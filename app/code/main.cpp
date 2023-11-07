#include "renderer.h"
#include "timer.h"
#include "file_system.h"
#include "pen_string.h"
#include "loader.h"
#include "dev_ui.h"
#include "pen_json.h"
#include "hash.h"
#include "str_utilities.h"
#include "input.h"
#include "data_struct.h"
#include "ecs/ecs_scene.h"

#include "maths/maths.h"
#include "audio/audio.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace put;
using namespace pen;
using namespace ecs;

// curl api

// curls include
#define CURL_STATICLIB
#include "curl/curl.h"

namespace curl
{
    constexpr size_t k_min_alloc = 1024;

    struct DataBuffer {
        u8*    data = nullptr;
        size_t size = 0;
        size_t alloc_size = 0;
    };

    void init()
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    size_t write_function(void *ptr, size_t size, size_t nmemb, DataBuffer* db)
    {
        size_t required_size = db->size + size * nmemb;
        size_t prev_pos = db->size;
        
        // allocate. with a min alloc amount to avoid excessive small allocs
        if(required_size >= db->alloc_size)
        {
            size_t new_alloc_size = std::max(required_size+1, k_min_alloc+1); // alloc with +1 space for null term
            db->data = (u8*)realloc(db->data, new_alloc_size);
            db->size = required_size;
            db->alloc_size = new_alloc_size;
            PEN_ASSERT(db->data);
        }
        
        memcpy(db->data + prev_pos, ptr, size * nmemb);
        db->data[required_size] = '\0'; // null term
        return size * nmemb;
    }

    DataBuffer download(const c8* url)
    {
        CURL *curl;
        CURLcode res;
        DataBuffer db = {};
        
        curl = curl_easy_init();
        
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &db);
        
            res = curl_easy_perform(curl);

            if(res != CURLE_OK)
            {
                PEN_LOG("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            curl_easy_cleanup(curl);
        }
        
        return db;
    }
}

namespace EntryFlags
{
    enum EntryFlags
    {
        allocated = 1<<0,
        artwork_cached = 1<<1,
        tracks_cached = 1<<2,
        artwork_loaded = 1<<3,
        tracks_loaded = 1<<4,
        transitioning = 1<<5,
        dragging = 1<<6
    };
};

struct soa
{
    cmp_array<u64>                          id;
    cmp_array<u64>                          flags;
    cmp_array<Str>                          artist;
    cmp_array<Str>                          title;
    cmp_array<Str>                          label;
    cmp_array<Str>                          cat;
    cmp_array<Str>                          link;
    cmp_array<Str>                          artwork_url;
    cmp_array<Str>                          artwork_filepath;
    cmp_array<u32>                          artwork_texture;
    cmp_array<pen::texture_creation_params> artwork_tcp;
    cmp_array<u32>                          track_name_count;
    cmp_array<Str*>                         track_names;
    cmp_array<u32>                          track_url_count;
    cmp_array<Str*>                         track_urls;
    cmp_array<Str*>                         track_filepaths;
    cmp_array<u32>                          select_track;
    cmp_array<f32>                          scrollx;
    
    std::atomic<size_t>                     available_entries = {0};
    std::atomic<size_t>                     soa_size = {0};
};
static soa s_releases;

generic_cmp_array& get_component_array(soa& s, size_t i)
{
    generic_cmp_array* begin = (generic_cmp_array*)&s;
    return begin[i];
}

size_t get_num_components(soa& s)
{
    generic_cmp_array* begin = (generic_cmp_array*)&s;
    generic_cmp_array* end = (generic_cmp_array*)&s.available_entries;
    return (size_t)(end - begin);
}

void resize_components(soa& s, size_t size)
{
    size_t new_size = s.soa_size + size;

    size_t num = get_num_components(s);
    for (u32 i = 0; i < num; ++i)
    {
        generic_cmp_array& cmp = get_component_array(s, i);
        size_t alloc_size = cmp.size * new_size;

        if (cmp.data)
        {
            // realloc
            cmp.data = pen::memory_realloc(cmp.data, alloc_size);

            // zero new mem
            size_t prev_size = s.soa_size * cmp.size;
            u8* new_offset = (u8*)cmp.data + prev_size;
            size_t zero_size = alloc_size - prev_size;
            pen::memory_zero(new_offset, zero_size);

            continue;
        }

        // alloc and zero
        cmp.data = pen::memory_alloc(alloc_size);
        pen::memory_zero(cmp.data, alloc_size);
    }

    s.soa_size = new_size;
}

Str download_and_cache(const Str& url, u64 releaseid)
{
    Str filepath = pen::str_replace_string(url, "https://", "");
    filepath = pen::str_replace_chars(filepath, '/', '_');
    
    Str dir = os_get_persistent_data_directory(); // "/Users/alex.dixon/dev/dig/cache/"; // TODO get user dir
    dir.appendf("/%llu", releaseid);
    
    // filepath
    Str path = dir;
    path.appendf("/%s", filepath.c_str());
    filepath = path;
    
    // check if file already exists
    u32 mtime = 0;
    pen::filesystem_getmtime(filepath.c_str(), mtime);
    if(mtime == 0)
    {
        // mkdirs
        pen::os_create_directory(dir.c_str());
        
        // download
        //PEN_LOG("downloading: %s\n", url.c_str());
        auto db = new curl::DataBuffer;
        *db = curl::download(url.c_str());
        
        // stash
        //PEN_LOG("caching: %s\n", filepath.c_str());
        FILE* fp = fopen(filepath.c_str(), "wb");
        fwrite(db->data, db->size, 1, fp);
        fclose(fp);
    }
    else
    {
        // PEN_LOG("cache hit: %s", filepath.c_str());
    }
    
    return filepath;
}

pen::texture_creation_params load_texture_from_disk(const Str& filepath)
{
    s32 w, h, c;
    stbi_uc* rgba = stbi_load(filepath.c_str(), &w, &h, &c, 4);
    
    pen::texture_creation_params tcp;
    tcp.width = w;
    tcp.height = h;
    tcp.format = PEN_TEX_FORMAT_RGBA8_UNORM;
    tcp.data = rgba;
    tcp.sample_count = 1;
    tcp.sample_quality = 0;
    tcp.num_arrays = 1;
    tcp.num_mips = 1;
    tcp.collection_type = 0;
    tcp.bind_flags =
    tcp.usage = PEN_USAGE_DEFAULT;
    tcp.bind_flags = PEN_BIND_SHADER_RESOURCE;
    tcp.cpu_access_flags = 0;
    tcp.flags = 0;
    tcp.block_size = 4;
    tcp.pixels_per_block = 1;
    tcp.collection_type = pen::TEXTURE_COLLECTION_NONE;
    tcp.data = rgba;
    tcp.data_size = w * h * c;
    
    return tcp;
}

void* info_loader(void* userdata)
{
    // load registry
    auto registry_data = curl::download("https://raw.githubusercontent.com/polymonster/dig/main/registry/releases.json");
    //PEN_LOG("%s\n", registry_data.data);
    pen::json releases_registry = pen::json::load((const c8*)registry_data.data);
    
    // get the count
    size_t count = releases_registry.size();
    
    // allocate mem
    resize_components(s_releases, count);
    
    // crawl releases
    for(u32 i = 0; i < count; ++i)
    {
        u32 ri = (u32)s_releases.available_entries;
        auto release = releases_registry[i];
        
        // simple info
        s_releases.id[ri] = release["id"].as_u64();
        s_releases.artist[ri] = release["artist"].as_str();
        s_releases.title[ri] = release["title"].as_str();
        s_releases.link[ri] = release["link"].as_str();
        s_releases.label[ri] = release["label"].as_str();
        s_releases.cat[ri] = release["cat"].as_str();
        
        // assign artwork url
        if(release["artworks"].size() > 1)
        {
            s_releases.artwork_url[ri] = release["artworks"][1].as_str();
        }
        else
        {
            s_releases.artwork_url[ri] = "";
        }
        
        // clear the artwork filepath
        s_releases.artwork_filepath[ri] = "";
        
        // track names
        s_releases.track_name_count[ri] = release["track_names"].size();
        if(s_releases.track_name_count[ri] > 0)
        {
            s_releases.track_names[ri] = new Str[s_releases.track_name_count[ri]];
            for(u32 t = 0; t < release["track_names"].size(); ++t)
            {
                s_releases.track_names[ri][t] = release["track_names"][t].as_str();
            }
        }
        
        // track urls
        s_releases.track_url_count[ri] = release["track_urls"].size();
        if(s_releases.track_url_count[ri] > 0)
        {
            s_releases.track_urls[ri] = new Str[s_releases.track_url_count[ri]];
            for(u32 t = 0; t < release["track_urls"].size(); ++t)
            {
                s_releases.track_urls[ri][t] = release["track_urls"][t].as_str();
            }
        }
        
        s_releases.available_entries++;
        pen::thread_sleep_ms(3);
    }
    
    return nullptr;
}

void* data_cacher(void* userdata)
{
    for(;;)
    {
        // waits on info loader thread
        for(size_t i = 0; i < s_releases.available_entries; ++i)
        {
            // cache art
            if(!s_releases.artwork_url[i].empty())
            {
                if(s_releases.artwork_filepath[i].empty())
                {
                    s_releases.artwork_filepath[i] = download_and_cache(s_releases.artwork_url[i], s_releases.id[i]);
                    pen::thread_sleep_ms(16);
                    
                    s_releases.flags[i] |= EntryFlags::artwork_cached;
                }
            }
            
            // load art if cached and not loaded
            if((s_releases.flags[i] & EntryFlags::artwork_cached) && !(s_releases.flags[i] & EntryFlags::artwork_loaded))
            {
                s_releases.artwork_tcp[i] = load_texture_from_disk(s_releases.artwork_filepath[i]);
                pen::thread_sleep_ms(16);
                
                std::atomic_thread_fence(std::memory_order_release);
                s_releases.flags[i] |= EntryFlags::artwork_loaded;
            }
            
            // cache tracks
            if(s_releases.track_url_count[i] > 0 && s_releases.track_filepaths[i] == nullptr)
            {
                s_releases.track_filepaths[i] = new Str[s_releases.track_url_count[i]];
                
                for(u32 t = 0; t < s_releases.track_url_count[i]; ++t)
                {
                    s_releases.track_filepaths[i][t] = download_and_cache(s_releases.track_urls[i][t], s_releases.id[i]);
                    pen::thread_sleep_ms(16);
                }
                
                std::atomic_thread_fence(std::memory_order_release);
                s_releases.flags[i] |= EntryFlags::tracks_cached;

            }
        }
        
        pen::thread_sleep_ms(16);
    }
    
    return nullptr;
}

namespace
{
    void*  user_setup(void* params);
    loop_t user_update();
    void   user_shutdown();
} // namespace

namespace pen
{
    pen_creation_params pen_entry(int argc, char** argv)
    {
        pen::pen_creation_params p;
        p.window_width = 1125 / 3;
        p.window_height = 2436 / 3;
        p.window_title = "dig";
        p.window_sample_count = 4;
        p.user_thread_function = user_setup;
        p.flags = pen::e_pen_create_flags::renderer;
        return p;
    }
} // namespace pen

vec2f touch_screen_mouse_wheel()
{
    const pen::mouse_state& ms = pen::input_get_mouse_state();
    vec2f cur = vec2f((f32)ms.x, (f32)ms.y);
    static vec2f prev = cur;
    
    static bool prevdown = false;
    if(!prevdown) {
        prev = cur;
    }
    
    vec2f delta = (cur - prev);
    prev = cur;
    
    if(ms.buttons[PEN_MOUSE_L])
    {
        prevdown = true;
        return delta;
    }
    
    prevdown = false;
    return 0.0f;
}

namespace
{
    pen::job*   s_thread_info = nullptr;
    pen::timer* frame_timer;
    pen::json   releases_registry;
    u32         clear_screen;

    void* user_setup(void* params)
    {
        //unpack the params passed to the thread and signal to the engine it ok to proceed
        pen::job_thread_params* job_params = (pen::job_thread_params*)params;
        s_thread_info = job_params->job_info;
        pen::semaphore_post(s_thread_info->p_sem_continue, 1);

        // intialise pmtech systems
        pen::jobs_create_job(put::audio_thread_function, 1024 * 10, nullptr, pen::e_thread_start_flags::detached);
        dev_ui::init();
        curl::init();
        
        pen::thread_create(info_loader, 10 * 1024 * 1024, nullptr, pen::e_thread_start_flags::detached);
        pen::thread_create(data_cacher, 10 * 1024 * 1024, nullptr, pen::e_thread_start_flags::detached);

        // timer
        frame_timer = pen::timer_create();
        pen::timer_start(frame_timer);
        
        // for clearing the backbuffer
        clear_state cs;
        cs.r = 1.0f;
        cs.g = 1.0f;
        cs.b = 1.0f;
        cs.a = 1.0f;
        cs.num_colour_targets = 1;
        clear_screen = pen::renderer_create_clear_state(cs);
        
        // white
        ImGui::StyleColorsLight();

        pen_main_loop(user_update);
        return PEN_THREAD_OK;
    }

    void user_shutdown()
    {
        pen::renderer_new_frame();

        pen::renderer_present();
        pen::renderer_consume_cmd_buffer();

        // signal to the engine the thread has finished
        pen::semaphore_post(s_thread_info->p_sem_terminated, 1);
    }

    loop_t user_update()
    {
        //
        s32 w, h;
        pen::window_get_size(w, h);
        
        // state
        static vec2f    scroll = vec2f(0.0f, 0.0f);
        static Str      play_track_filepath = "";
        static bool     invalidate_track = false;
        static bool     mute = false;
        
        pen::timer_start(frame_timer);
        pen::renderer_new_frame();
        
        // clear backbuffer
        pen::renderer_set_targets(PEN_BACK_BUFFER_COLOUR, PEN_BACK_BUFFER_DEPTH);
        pen::renderer_clear(clear_screen);

        put::dev_ui::new_frame();
                

        ImGui::SetNextWindowPos(ImVec2(0.0, 0.0));
        ImGui::SetNextWindowSize(ImVec2((f32)w, (f32)h));
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0, 0.0));
        
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        auto current_window = ImGui::GetCurrentWindow();
        
        //
        vec2f scroll_delta = touch_screen_mouse_wheel();
        
        s32 top = -1;
        for(u32 r = 0; r < s_releases.available_entries; ++r)
        {
            if(r > 100)
            {
                continue;
            }
            
            auto title = s_releases.title[r];
            auto artist = s_releases.artist[r];
            
            // apply loads
            std::atomic_thread_fence(std::memory_order_acquire);
            if(s_releases.flags[r] & EntryFlags::artwork_loaded)
            {
                if(s_releases.artwork_texture[r] == 0)
                {
                    if(s_releases.artwork_tcp[r].data)
                    {
                        s_releases.artwork_texture[r] = pen::renderer_create_texture(s_releases.artwork_tcp[r]);
                    }
                }
            }
            
            // skip unloaded
            if(s_releases.artwork_texture[r] == 0)
            {
                continue;
            }
            
            // images
            if(s_releases.artwork_texture[r])
            {
                ImGui::BeginChildEx("rel", r+1, ImVec2(w, w), false, 0);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 0.0));
                
                u32 numImages = std::max<u32>(1, s_releases.track_url_count[r]);
                
                bool hovered = false;
                f32 max_scroll = (numImages * w) - w;
                for(u32 i = 0; i < numImages; ++i)
                {
                    if(i > 0)
                    {
                        ImGui::SameLine();
                    }
                    
                    ImGui::Image(IMG(s_releases.artwork_texture[r]), ImVec2(w, w));
                    
                    
                    if(ImGui::IsItemHovered() && pen::input_is_mouse_down(PEN_MOUSE_L))
                    {
                        if(abs(scroll_delta.x) > 5)
                        {
                            s_releases.flags[r] |= EntryFlags::dragging;
                        }
                        hovered = true;
                    }
                    
                    if(ImGui::IsItemVisible())
                    {
                        if(top == -1)
                        {
                            top = r;
                        }
                    }
                }
                
                // stop drags if we no longer hover
                if(!hovered)
                {
                    if(s_releases.flags[r] & EntryFlags::dragging)
                    {
                        s_releases.flags[r] &= ~EntryFlags::dragging;
                    }
                }
                
                if(s_releases.flags[r] & EntryFlags::dragging)
                {
                    // scroll_delta.y = 0.0f;
                    s_releases.scrollx[r] -= scroll_delta.x;
                    s_releases.scrollx[r] = std::max(s_releases.scrollx[r], 0.0f);
                    s_releases.scrollx[r] = std::min(s_releases.scrollx[r], max_scroll);
                    s_releases.flags[r] &= ~EntryFlags::transitioning;
                }
                else
                {
                    f32 target = s_releases.select_track[r] * w;
                    f32 ssx = s_releases.scrollx[r];
                    
                    if(!(s_releases.flags[r] & EntryFlags::transitioning))
                    {
                        if(ssx > target + (w/2))
                        {
                            s_releases.select_track[r] += 1;
                            s_releases.flags[r] |= EntryFlags::transitioning;
                        }
                        else if(ssx < target - (w/2) )
                        {
                            s_releases.select_track[r] -= 1;
                            s_releases.flags[r] |= EntryFlags::transitioning;
                        }
                        else if(ssx != target)
                        {
                            s_releases.flags[r] |= EntryFlags::transitioning;
                        }
                    }
                    else
                    {
                        if(abs(ssx - target) < 3.33)
                        {
                            s_releases.scrollx[r] = target;
                            s_releases.flags[r] &= ~EntryFlags::transitioning;
                        }
                        else
                        {
                            s_releases.scrollx[r] = lerp(ssx, target, 0.33f);
                        }
                    }
                }
                
                ImGui::SetScrollX(s_releases.scrollx[r]);
                
                ImGui::PopStyleVar();
                ImGui::EndChild();
            }
            
            // tracks
            if(s_releases.track_url_count[r])
            {
                // offset to centre
                auto ww = ImGui::GetWindowSize().x;
                auto tw = ImGui::CalcTextSize(ICON_FA_STOP_CIRCLE).x * s_releases.track_url_count[r] * 1.5f;
                ImGui::SetCursorPosX((ww - tw) * 0.5f);
                
                // dots
                for(u32 i = 0; i < s_releases.track_url_count[r]; ++i)
                {
                    if(i > 0)
                    {
                        ImGui::SameLine();
                    }
                    
                    u32 sel = s_releases.select_track[r];
                    if(i == sel)
                    {
                        if(top == r)
                        {
                            ImGui::Text("%s", ICON_FA_PLAY);
                            
                            // load up the track
                            if(!(play_track_filepath == s_releases.track_filepaths[r][sel]))
                            {
                                play_track_filepath = s_releases.track_filepaths[r][sel];
                                invalidate_track = true;
                            }
                            
                        }
                        else
                        {
                            ImGui::Text("%s", ICON_FA_STOP_CIRCLE);
                        }
                    }
                    else
                    {
                        ImGui::Text("%s", ICON_FA_STOP_CIRCLE);
                    }
                }
            }
            
            // buttons
            ImGui::SetWindowFontScale(2.0f);
            ImGui::Text("%s", ICON_FA_HEART_O);
            ImGui::SameLine();
            ImGui::Text("%s", ICON_FA_SHOPPING_BASKET);
            ImGui::SetWindowFontScale(1.0f);
            
            // release info
            ImGui::TextWrapped("%s", artist.c_str());
            ImGui::TextWrapped("%s", title.c_str());
            
            // track name
            u32 sel = s_releases.select_track[r];
            if(s_releases.track_name_count[r] > s_releases.select_track[r])
            {
                ImGui::TextWrapped("%s", s_releases.track_names[r][sel].c_str());
            }
            
            ImGui::Spacing();
            ImGui::Spacing();
        }
        
        if(!mute)
        {
            if(play_track_filepath.length() > 0 && invalidate_track)
            {
                static u32 si = -1;
                static u32 ci = -1;
                static u32 gi = -1;
                
                // stop existing
                if(is_valid(si))
                {
                    // release existing
                    put::audio_channel_stop(ci);
                    put::audio_release_resource(si);
                    put::audio_release_resource(ci);
                    put::audio_release_resource(gi);
                }
                
                si = put::audio_create_sound(play_track_filepath.c_str());
                ci = put::audio_create_channel_for_sound(si);
                gi = put::audio_create_channel_group();
                
                put::audio_add_channel_to_group(ci, gi);
                put::audio_group_set_volume(gi, 1.0f);

                invalidate_track = false;
            }
        }

        ImGui::PopStyleVar(4);
        ImGui::End();
        
        // main window scroll
        {
            scroll.y -= scroll_delta.y;
            ImGui::SetScrollY(current_window, scroll.y);
        }
                
        // present
        put::dev_ui::render();
        pen::renderer_present();
        pen::renderer_consume_cmd_buffer();
        put::audio_consume_command_buffer();

        if (pen::semaphore_try_wait(s_thread_info->p_sem_exit))
        {
            user_shutdown();
            pen_main_loop_exit();
        }

        pen_main_loop_continue();
    }
} // namespace

void* pen::user_entry( void* params )
{
    return PEN_THREAD_OK;
}
