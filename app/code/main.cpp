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
#include "main.h"
#include "audio/audio.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <fstream>

#include "maths/maths.h"

using namespace put;
using namespace pen;

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

u32 get_tags(nlohmann::json& tags)
{
    u32 t = 0;
    for(size_t i = 0; i < PEN_ARRAY_SIZE(Tags::names); ++i)
    {
        auto& name = Tags::names[i];
            
        if(tags.contains(name))
        {
            if(tags[name])
            {
                t |= (1<<i);
            }
        }
    }
    
    return t;
}

Str get_tags_str(u32 tags)
{
    Str tag_str = "";
    bool first = true;
    for(size_t i = 0; i < PEN_ARRAY_SIZE(Tags::names); ++i)
    {
        if(tags & 1<<i)
        {
            if(!first)
            {
                tag_str.append(" / ");
            }
            
            tag_str.append(Tags::names[i]);
            first = false;
        }
    }
    
    return tag_str;
}

u32 tag_menu(u32 tags)
{
    for(size_t i = 0; i < PEN_ARRAY_SIZE(Tags::names); ++i)
    {
        bool selected = tags & 1<<i;
        ImGui::Checkbox(Tags::names[i], &selected);
        
        if(selected)
        {
            tags |= (1<<i);
        }
        else
        {
            tags &= ~(1<<i);
        }
    }
    
    return tags;
}

// TODO: make more user data centric
static nlohmann::json   s_likes;
static std::mutex       s_like_mutex;
static bool             s_likes_invalidated = false;

nlohmann::json get_likes()
{
    s_like_mutex.lock();
    auto cp = s_likes;
    s_like_mutex.unlock();
    return cp;
}

bool has_like(Str id)
{
    bool ret = false;
    s_like_mutex.lock();
    if(s_likes.contains(id.c_str()))
    {
        ret = s_likes[id.c_str()];
    }
    s_like_mutex.unlock();
    return ret;
}

void add_like(Str id)
{
    s_like_mutex.lock();
    s_likes[id.c_str()] = true;
    s_like_mutex.unlock();
    s_likes_invalidated = true;
}

void remove_like(Str id)
{
    s_like_mutex.lock();
    s_likes.erase(id.c_str());
    s_like_mutex.unlock();
    s_likes_invalidated = true;
}

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

void free_components(soa& s)
{
    size_t num = get_num_components(s);
    for (u32 i = 0; i < num; ++i)
    {
        generic_cmp_array& cmp = get_component_array(s, i);
        pen::memory_free(cmp.data);
    }
}

Str get_cache_path()
{
    Str dir = os_get_cache_data_directory();
    dir.appendf("/dig/cache/");
    return dir;
}

Str get_docs_path()
{
    Str dir = os_get_persistent_data_directory();
    dir.appendf("/dig/");
    return dir;
}

Str download_and_cache(const Str& url, Str releaseid)
{
    Str filepath = pen::str_replace_string(url, "https://", "");
    filepath = pen::str_replace_chars(filepath, '/', '_');
    
    Str dir = get_cache_path();
    dir.appendf("/%s", releaseid.c_str());
    
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
        auto db = new curl::DataBuffer;
        *db = curl::download(url.c_str());
        
        // stash
        FILE* fp = fopen(filepath.c_str(), "wb");
        fwrite(db->data, db->size, 1, fp);
        fclose(fp);
        
        // free
        free(db->data);
    }
    
    return filepath;
}

Str download_and_cache_named(const Str& url, const Str& filename)
{
    Str dir = os_get_persistent_data_directory();
    dir.appendf("/dig");
    
    // filepath
    Str path = dir;
    path.appendf("/%s", filename.c_str());
    Str filepath = path;
    
    // check if file already exists
    u32 mtime = 0;
    pen::filesystem_getmtime(filepath.c_str(), mtime);
    if(mtime == 0)
    {
        // mkdirs
        pen::os_create_directory(dir.c_str());
    }
        
    // download
    auto db = new curl::DataBuffer;
    *db = curl::download(url.c_str());
    
    // stash
    FILE* fp = fopen(filepath.c_str(), "wb");
    fwrite(db->data, db->size, 1, fp);
    fclose(fp);
    
    // free
    free(db->data);
    
    return filepath;
}

pen::texture_creation_params load_texture_from_disk(const Str& filepath)
{
    s32 w, h, c;
    stbi_uc* rgba = stbi_load(filepath.c_str(), &w, &h, &c, 4);
    
    c = 4;
    
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
    tcp.bind_flags = 0;
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

void* registry_loader(void* userdata)
{
    DataContext* ctx = (DataContext*)userdata;
    
    // construct registry path
    Str reg_path = get_docs_path();
    reg_path.append("registry.json");
    
    // first we can check if we have a cached registry
    ctx->cache_registry_status = DataStatus::e_loading;
        
    u32 mtime = 0;
    pen::filesystem_getmtime(reg_path.c_str(), mtime);
    if(mtime != 0)
    {
        ctx->registry_mutex.lock();
        try {
            ctx->registry = nlohmann::json::parse(std::ifstream(reg_path.c_str()));
            ctx->cache_registry_status = DataStatus::e_ready;
        } 
        catch(...) {
            ctx->cache_registry_status = DataStatus::e_loading;
        }
        ctx->registry_mutex.unlock();
    }
    
    for(;;)
    {
        // grab the latest
        ctx->latest_registry_status = DataStatus::e_loading;
        download_and_cache_named("https://raw.githubusercontent.com/polymonster/dig/main/registry/releases.json", "registry.json");
        
        nlohmann::json reg = nlohmann::json::parse(std::ifstream(reg_path.c_str()));
        ctx->latest_registry_status = DataStatus::e_ready;
        
        ctx->registry_mutex.lock();
        ctx->registry = reg;
        ctx->cache_registry_status = DataStatus::e_ready;
        ctx->registry_mutex.unlock();

        while(ctx->latest_registry_status == DataStatus::e_ready)
        {
            // wait for a request
            pen::thread_sleep_ms(66);
        }
        
        PEN_LOG("fetch new reg");
    }
};

void* user_data_thread(void* userdata)
{
    // get user data dir
    Str user_data_dir = os_get_persistent_data_directory();
    user_data_dir.append("/dig/user_data/");
    
    // create dir
    pen::os_create_directory(user_data_dir);
    
    // grab likes
    u32 mtime  = 0;
    Str likes_filepath = user_data_dir;
    likes_filepath.appendf("likes.json");
    pen::filesystem_getmtime(likes_filepath.c_str(), mtime);
    if(mtime)
    {
        std::ifstream f(likes_filepath.c_str());
        s_likes = nlohmann::json::parse(f);;
    }

    for(;;)
    {
        if(s_likes_invalidated)
        {
            auto likes = get_likes();
            auto likes_str = likes.dump();
            
            FILE* fp = fopen(likes_filepath.c_str(), "w");
            fwrite(likes_str.c_str(), likes_str.length(), 1, fp);
            fclose(fp);
        }
        
        pen::thread_sleep_ms(66);
    }
}

void* info_loader(void* userdata)
{
    // get view from userdata
    ReleasesView* view = (ReleasesView*)userdata;
    
    // wait for a few seconds for a new registry
    u32 timestart = pen::get_time_ms();
    bool use_latest = true;
    
    if(view->reg_timeout > 1000)
    {
        view->data_ctx->latest_registry_status = DataStatus::e_loading;
    }
    
    // grab registry; either latest or cached
    nlohmann::json releases_registry;
    
    while(view->data_ctx->cache_registry_status != DataStatus::e_ready)
    {
        // need to wait on cached
        pen::thread_sleep_ms(16);
    }
    
    view->data_ctx->registry_mutex.lock();
    releases_registry = view->data_ctx->registry;
    view->data_ctx->registry_mutex.unlock();
    
    // grab items for this requested view from the registry
    std::vector<ChartItem> view_chart;
    std::string view_name = View::lookup_names[view->view];
    
    if(view->view == View::likes)
    {
        for(auto& like : s_likes.items())
        {
            if(releases_registry.contains(like.key()) && like.value())
            {
                view_chart.push_back({
                    like.key(),
                    0
                });
            }
        }
    }
    else
    {
        // populate view
        for(auto& item : releases_registry)
        {
            if(item.contains(view_name))
            {
                view_chart.push_back({
                    item["id"],
                    item[view_name]
                });
            }
        }
    }
    
    // sort the items
    std::sort(begin(view_chart),end(view_chart),[](ChartItem a, ChartItem b) {return a.pos < b.pos; });
    
    // make space
    resize_components(view->releases, view_chart.size());
    
    for(auto& entry : view_chart)
    {
        u32 ri = (u32)view->releases.available_entries;
        auto release = releases_registry[entry.index];
        
        // grab tags
        /*
        if(release.contains("tags"))
        {
            u32 tags = get_tags(release["tags"]);
            if(!(tags & view->tags))
            {
                continue;
            }
        }
        */
        
        // simple info
        view->releases.artist[ri] = release["artist"];
        view->releases.title[ri] = release["title"];
        view->releases.link[ri] = release["link"];
        view->releases.label[ri] = release["label"];
        view->releases.cat[ri] = release["cat"];
        
        // clear
        view->releases.artwork_filepath[ri] = "";
        view->releases.artwork_texture[ri] = 0;
        view->releases.flags[ri] = 0;
        view->releases.track_name_count[ri] = 0;
        view->releases.track_names[ri] = nullptr;
        view->releases.track_url_count[ri] = 0;
        view->releases.track_urls[ri] = nullptr;
        view->releases.track_filepath_count[ri] = 0;
        view->releases.select_track[ri] = 0; // reset
        memset(&view->releases.artwork_tcp[ri], 0x0, sizeof(pen::texture_creation_params));
        
        std::string id = release["id"];
        view->releases.id[ri] = id.c_str();

        // assign artwork url
        if(release["artworks"].size() > 1)
        {
            view->releases.artwork_url[ri] = release["artworks"][1];
        }
        else
        {
            view->releases.artwork_url[ri] = "";
        }

        // track names
        u32 name_count = (u32)release["track_names"].size();
        if(name_count > 0)
        {
            view->releases.track_names[ri] = new Str[name_count];
            for(u32 t = 0; t < release["track_names"].size(); ++t)
            {
                view->releases.track_names[ri][t] = release["track_names"][t];
            }
            
            std::atomic_thread_fence(std::memory_order_release);
            view->releases.track_name_count[ri] = name_count;
        }

        // track urls
        u32 url_count = (u32)release["track_urls"].size();
        if(url_count > 0)
        {
            view->releases.track_urls[ri] = new Str[url_count];
            for(u32 t = 0; t < release["track_urls"].size(); ++t)
            {
                view->releases.track_urls[ri][t] = release["track_urls"][t];
            }
            
            std::atomic_thread_fence(std::memory_order_release);
            view->releases.track_url_count[ri] = url_count;
        }
        
        // check likes
        if(has_like(view->releases.id[ri]))
        {
            view->releases.flags[ri] |= EntryFlags::liked;
        }
        
        // store tags
        if(release.contains("store_tags"))
        {
            for(u32 t = 0; t < PEN_ARRAY_SIZE(StoreTags::names); ++t) {
                if(release["store_tags"].contains(StoreTags::names[t]) && release["store_tags"][StoreTags::names[t]])
                {
                    view->releases.store_tags[ri] |= (1<<t);
                }
            }
        }

        pen::thread_sleep_ms(1);
        view->releases.available_entries++;
    }
    
    view->threads_terminated++;
    return nullptr;
}

size_t get_folder_size_recursive(const pen::fs_tree_node& dir, const c8* root)
{
    size_t size = 0;
    for(u32 i = 0; i < dir.num_children; ++i)
    {
        Str path = "";
        path.appendf("%s/%s", root, dir.children[i].name);
        
        if(dir.children[i].num_children > 0)
        {
            size += get_folder_size_recursive(dir.children[i], path.c_str());
        }
        else
        {
            size += filesystem_getsize(path.c_str());
        }
    }
    
    return size;
}

void* data_cacher(void* userdata)
{
    // get view from userdata
    ReleasesView* view = (ReleasesView*)userdata;
    
    Str cache_dir = get_cache_path();
    
    // enum cache stats
    pen::fs_tree_node dir;
    pen::filesystem_enum_directory(cache_dir.c_str(), dir, 1, "**/*.*");
    view->data_ctx->cached_release_folders = 0;
    view->data_ctx->cached_release_bytes = 0;
    for(u32 i = 0; i < dir.num_children; ++i)
    {
        Str path = cache_dir;
        path.append(dir.children[i].name);
        
        if(dir.children[i].num_children > 0)
        {
            view->data_ctx->cached_release_folders++;
            view->data_ctx->cached_release_bytes += get_folder_size_recursive(dir.children[i], cache_dir.c_str());
        }
        else
        {
            pen::fs_tree_node release_dir;
            pen::filesystem_enum_directory(path.c_str(), release_dir);
            view->data_ctx->cached_release_folders++;
            view->data_ctx->cached_release_bytes += get_folder_size_recursive(release_dir, path.c_str());
        }
    }
        
    for(;;)
    {
        if(view->terminate) {
            break;
        }
        
        // waits on info loader thread
        for(size_t i = 0; i < view->releases.available_entries; ++i)
        {
            if(!(view->releases.flags[i] & EntryFlags::cache_url_requested)) {
                continue;
            }
            
            // cache art
            if(!view->releases.artwork_url[i].empty())
            {
                if(view->releases.artwork_filepath[i].empty())
                {
                    view->releases.artwork_filepath[i] = download_and_cache(view->releases.artwork_url[i], view->releases.id[i]);
                    view->releases.flags[i] |= EntryFlags::artwork_cached;
                }
            }
            
            // cache tracks
            if(!(view->releases.flags[i] & EntryFlags::tracks_cached))
            {
                if(view->releases.track_url_count[i] > 0 && view->releases.track_filepaths[i] == nullptr)
                {
                    view->releases.track_filepaths[i] = new Str[view->releases.track_url_count[i]];
                    
                    for(u32 t = 0; t < view->releases.track_url_count[i]; ++t)
                    {
                        view->releases.track_filepaths[i][t] = "";
                        Str fp = download_and_cache(view->releases.track_urls[i][t], view->releases.id[i]);
                        view->releases.track_filepaths[i][t] = fp;
                    }
                    
                    std::atomic_thread_fence(std::memory_order_release);
                    view->releases.flags[i] |= EntryFlags::tracks_cached;
                    view->releases.track_filepath_count[i] = view->releases.track_url_count[i];
                }
            }
        }
        
        pen::thread_sleep_ms(16);
    }
    
    // flag terminated
    view->threads_terminated++;
    return nullptr;
}

void* data_loader(void* userdata)
{
    // get view from userdata
    ReleasesView* view = (ReleasesView*)userdata;
    
    for(;;)
    {
        if(view->terminate) {
            break;
        }
        
        for(size_t i = 0; i < view->releases.available_entries; ++i)
        {
            // load art if cached and not loaded
            std::atomic_thread_fence(std::memory_order_acquire);
            
            if((view->releases.flags[i] & EntryFlags::artwork_cached) &&
               !(view->releases.flags[i] & EntryFlags::artwork_loaded) &&
               (view->releases.flags[i] & EntryFlags::artwork_requested))
            {
                view->releases.artwork_tcp[i] = load_texture_from_disk(view->releases.artwork_filepath[i]);
                
                std::atomic_thread_fence(std::memory_order_release);
                view->releases.flags[i] |= EntryFlags::artwork_loaded;
            }
        }
        
        pen::thread_sleep_ms(16);
    }
    
    view->threads_terminated++;
    return nullptr;
}

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
    u32         clear_screen;
    AppContext  ctx;

    ReleasesView* new_view(View_t new_view, Tags_t new_tags, u32 reg_timeout)
    {
        ReleasesView* view = new ReleasesView;
        view->data_ctx = &ctx.data_ctx;
        view->tags = new_tags;
        view->view = new_view;
        view->reg_timeout = reg_timeout;
        view->scroll = vec2f(0.0f, ctx.w);
        
        // workers per view
        pen::thread_create(info_loader, 10 * 1024 * 1024, view, pen::e_thread_start_flags::detached);
        pen::thread_create(data_cacher, 10 * 1024 * 1024, view, pen::e_thread_start_flags::detached);
        pen::thread_create(data_loader, 10 * 1024 * 1024, view, pen::e_thread_start_flags::detached);
        
        return view;
    }

    void change_view(View_t view, Tags_t tags, u32 reg_timeout = 1000)
    {
        // prevent entering same view twice
        if(ctx.view) {
            if(ctx.view->view == view && ctx.view->tags == tags) {
                return;
            }
        }
        
        // first we add the current view into background views
        if(ctx.view && ctx.view->view < View::likes)
        {
            ctx.back_view = ctx.view;
            ctx.background_views.insert(ctx.view);
        }
            
        // kick off a new view
        ctx.view = new_view(view, tags, reg_timeout);
    }

    void cleanup_views()
    {
        std::vector<ReleasesView*> to_remove;
        
        for(auto& view : ctx.background_views)
        {
            if(view != ctx.back_view && view != ctx.view)
            {
                view->terminate = 1;
                if(view->threads_terminated == 3)
                {
                    auto& releases = view->releases;
                    for(size_t i = 0; i < releases.available_entries; ++i) {
                        // unload textures
                        if (releases.flags[i] & EntryFlags::artwork_loaded) {
                            if(releases.artwork_texture[i] != 0) {
                                // textures themseleves
                                pen::renderer_release_texture(releases.artwork_texture[i]);
                                releases.artwork_texture[i] = 0;
                            }
                            else
                            {
                                // TODO: free
                                // texture preloaded from disk
                                //free(releases.artwork_tcp[i].data);
                            }
                            memset(&releases.artwork_tcp, 0x0, sizeof(texture_creation_params));
                            releases.flags[i] &= ~EntryFlags::artwork_loaded;
                            releases.flags[i] &= ~EntryFlags::artwork_requested;
                        }
                        
                        // unload strings
                        if(view->releases.track_filepaths[i]) {
                            delete[] view->releases.track_filepaths[i];
                        }
                        
                        if(view->releases.track_names.data && view->releases.track_names[i]) {
                            delete[] view->releases.track_names[i];
                        }
                        
                        if(view->releases.track_urls[i]) {
                            delete[] view->releases.track_urls[i];
                        }
                    }
                    
                    // cleanup memory from the soa itself
                    free_components(view->releases);
                    
                    // add to remove list to preserve the set iterator
                    to_remove.push_back(view);
                }
            }
        }
        
        // erase view
        for(auto& rm : to_remove) {
            PEN_LOG("erasing view");
            ctx.background_views.erase(ctx.background_views.find(rm));
        }
    }

    void view_reload()
    {
        f32 reloady = (f32)ctx.w / k_top_pull_reload;
        
        // reload on drag
        static bool debounce = false;
        if(debounce)
        {
            if(!pen::input_is_mouse_down(PEN_MOUSE_L))
            {
                debounce = false;
            }
        }
        else
        {
            if(pen::input_is_mouse_down(PEN_MOUSE_L))
            {
                // check threshold
                if(ctx.view->scroll.y < reloady)
                {
                    if(ctx.reload_view == nullptr && ctx.view->view != View::likes)
                    {
                        // spawn reload view
                        ctx.reload_view = new_view(ctx.view->view, ctx.view->tags, 5000);
                        debounce = true; // wait for debounce;
                    }
                }
            }
        }

        // reload anim if we have an empty view or are relaoding
        if(ctx.reload_view || ctx.view->releases.available_entries == 0)
        {
            ImGui::SetWindowFontScale(2.0f);
            
            // small padding
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.75f));
            
            // spinner
            auto ww = ImGui::GetWindowSize().x;
            ImGui::SetCursorPosX((ww * 0.5f));
            ImGui::Text("%s", ICON_FA_SPINNER);
            ImGui::SetWindowFontScale(1.0f);
        }
        
        // if reloading wait until the new view has entries and then swap
        if(ctx.reload_view)
        {
            if(ctx.reload_view->releases.available_entries > 0)
            {
                // swap existing view with the new one and reset
                ctx.background_views.insert(ctx.view);
                ctx.view = ctx.reload_view;
                ctx.reload_view = nullptr;
            }
        }
    }

    void view_menu()
    {
        // view info
        View_t cur_view = ctx.view->view;
        Tags_t cur_tags = ctx.view->tags;
        
        if(cur_view != View::likes)
        {
            // store page
            ImGui::SetWindowFontScale(k_text_size_h2);
            
            constexpr const char* k_modes[] = {
                "Latest",
                "Weekly Chart",
                "Monthly Chart",
                "Likes"
            };
            
            ImGui::Dummy(ImVec2(k_indent1, 0.0f));
            ImGui::SameLine();
            ImGui::Text("%s", k_modes[cur_view]);
            ImVec2 view_menu_pos = ImGui::GetItemRectMin();
            view_menu_pos.y = ImGui::GetItemRectMax().y;
            if(ImGui::IsItemClicked()) {
                ImGui::OpenPopup("View Select");
            }
            
            ImGui::SetNextWindowPos(view_menu_pos);
            if(ImGui::BeginPopup("View Select"))
            {
                if(ImGui::MenuItem("Latest"))
                {
                    change_view(View::latest, ctx.view->tags);
                }
                if(ImGui::MenuItem("Weekly Chart"))
                {
                    change_view(View::weekly_chart, ctx.view->tags);
                }
                if(ImGui::MenuItem("Monthly Chart"))
                {
                    change_view(View::monthly_chart, ctx.view->tags);
                }
                ImGui::EndPopup();
            }
            
            ImGui::SetWindowFontScale(k_text_size_body);
            
            // tags
            ImGui::Dummy(ImVec2(k_indent1, 0.0f));
            ImGui::SameLine();
                    
            ImGui::Text("%s", get_tags_str(ctx.view->tags).c_str());
            ImVec2 tag_menu_pos = ImGui::GetItemRectMin();
            tag_menu_pos.y = ImGui::GetItemRectMax().y;
            
            if(ImGui::IsItemClicked()) {
                ImGui::OpenPopup("Tag Select");
            }
            
            ImGui::SetNextWindowPos(tag_menu_pos);
            if(ImGui::BeginPopup("Tag Select"))
            {
                ImGui::SetWindowFontScale(k_text_size_h2);
                u32 new_tags = tag_menu(ctx.view->tags);
                if(new_tags != cur_tags)
                {
                    change_view(ctx.view->view, new_tags);
                }
                ImGui::EndPopup();
                ImGui::SetWindowFontScale(k_text_size_body);
            }
        }
        else
        {
            // likes page
            ImGui::SetWindowFontScale(k_text_size_h2);
            ImGui::Dummy(ImVec2(k_indent1, 0.0f));
            ImGui::SameLine();
            ImGui::Text("%s", "Likes");
            ImGui::SetWindowFontScale(k_text_size_body);
        }
        
        // cleanup memory on old views
        cleanup_views();
    }

    bool lenient_button_click(f32 padding, bool& debounce)
    {
        auto& ms = pen::input_get_mouse_state();
        ImVec2 bbmin = ImGui::GetItemRectMin();
        ImVec2 bbmax = ImGui::GetItemRectMax();
        
        // need to wait on debounce
        if(debounce)
        {
            if(!ms.buttons[PEN_MOUSE_L])
            {
                debounce = false;
            }
            
            return false;
        }
        else
        {
            if(ms.buttons[PEN_MOUSE_L])
            {
                f32 d = maths::point_aabb_distance(vec2f(ms.x, ms.y), vec2f(bbmin.x, bbmin.y), vec2f(bbmax.x, bbmax.y));
                if(d < padding)
                {
                    debounce = true;
                    return true;
                }
            }
        }
        
        return false;
    }

    void header_menu()
    {
        ImGui::SetWindowFontScale(k_text_size_h1);
        ImGui::Dummy(ImVec2(k_indent1, 0.0f));
        ImGui::SameLine();
        
        if(ctx.view->view == View::likes || ctx.view->view == View::settings)
        {
            ImGui::Text("%s", ICON_FA_CHEVRON_LEFT);
            
            static bool back_debounce = false;
            if(lenient_button_click(80.0f, back_debounce))
            {
                ctx.view = ctx.back_view;
            }
        }
        else
        {
            ImGui::Text("Dig");
        }
        
        // likes button on same line
        ImGui::SameLine();
        f32 offset = ImGui::CalcTextSize("%s", ICON_FA_HEART_O).y;
        ImGui::SetCursorPosX(ctx.w - offset * 2.5f);
        ImGui::Text("%s", ctx.view->view == View::likes ? ICON_FA_HEART : ICON_FA_HEART_O);
        
        static bool heart_debounce = false;
        if(lenient_button_click(20.0f, heart_debounce))
        {
            change_view(View::likes, Tags::all);
        }
        
        ImGui::SameLine();
        ImGui::Text("%s", ICON_FA_ELLIPSIS_H);
        
        static bool ellipsis_debounce = false;
        if(lenient_button_click(64.0f, ellipsis_debounce))
        {
            change_view(View::settings, Tags::all);
        }
        
        ImGui::SetWindowFontScale(k_text_size_body);
    }

    void release_feed()
    {
        f32 w = ctx.w;
        f32 h = ctx.h;
        
        // get latest releases
        auto& releases = ctx.view->releases;
        
        // releases
        ImGui::BeginChildEx("releases", 1, ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        auto current_window = ImGui::GetCurrentWindow();
        ctx.releases_window = current_window;
        
        // Add an empty dummy at the top
        ImGui::Dummy(ImVec2(w, w));
        
        ctx.top = -1;
        for(u32 r = 0; r < releases.available_entries; ++r)
        {
            auto title = releases.title[r];
            auto artist = releases.artist[r];
            
            // apply loads
            std::atomic_thread_fence(std::memory_order_acquire);
            if(releases.flags[r] & EntryFlags::artwork_loaded)
            {
                if(releases.artwork_texture[r] == 0)
                {
                    if(releases.artwork_tcp[r].data)
                    {
                        releases.artwork_texture[r] = pen::renderer_create_texture(releases.artwork_tcp[r]);
                        pen::memory_free(releases.artwork_tcp[r].data); // data is copied for the render thread. safe to delete
                        releases.artwork_tcp[r].data = nullptr;
                    }
                }
            }
            
            // select primary
            ImGui::Spacing();
            f32 y = ImGui::GetCursorPos().y - ImGui::GetScrollY();
            if(y < (f32)h - ((f32)w * 1.1f))
            {
                if(y > -w * 0.33f)
                {
                    if(ctx.top == -1)
                    {
                        ctx.top = r;
                    }
                }
            }
            
            // label and catalogue
            ImGui::SetWindowFontScale(k_text_size_h3);
            
            ImGui::Dummy(ImVec2(k_indent1, 0.0f));
            ImGui::SameLine();
            ImGui::TextWrapped("%s: %s", releases.label[r].c_str(), releases.cat[r].c_str());
            
            ImGui::SetWindowFontScale(k_text_size_body);
            
            // ..
            f32 scaled_vel = ctx.scroll_delta.x;
            
            // images
            if(releases.artwork_texture[r])
            {
                f32 h = (f32)w * ((f32)releases.artwork_tcp[r].height / (f32)releases.artwork_tcp[r].width);
                f32 spacing = 20.0f;
                                
                ImGui::BeginChildEx("rel", r+1, ImVec2((f32)w, h + 10.0f), false, 0);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.0));
                
                u32 num_images = std::max<u32>(1, releases.track_url_count[r]);
                f32 imgw = w + spacing;
                
                f32 max_scroll = (num_images * imgw) - imgw;
                for(u32 i = 0; i < num_images; ++i)
                {
                    if(i > 0)
                    {
                        ImGui::SameLine();
                    }
                    
                    ImGui::Image(IMG(releases.artwork_texture[r]), ImVec2((f32)w, h));
                    
                    if(ImGui::IsItemHovered() && pen::input_is_mouse_down(PEN_MOUSE_L))
                    {
                        if(!ctx.scroll_lock_y)
                        {
                            if(abs(ctx.scroll_delta.x) > k_drag_threshold && ctx.side_drag)
                            {
                                releases.flags[r] |= EntryFlags::dragging;
                                ctx.scroll_lock_x = true;
                            }
                            
                            releases.flags[r] |= EntryFlags::hovered;
                        }
                    }
                }
                
                if(!pen::input_is_mouse_down(PEN_MOUSE_L))
                {
                    releases.flags[r] &= ~EntryFlags::hovered;
                    if(abs(scaled_vel) < 1.0)
                    {
                        releases.flags[r] &= ~EntryFlags::dragging;
                    }
                }
                
                // stop drags if we no longer hover
                if(!(releases.flags[r] & EntryFlags::hovered))
                {
                    if(releases.flags[r] & EntryFlags::dragging)
                    {
                        ctx.scroll_delta.y = 0.0f;
                        releases.scrollx[r] -= scaled_vel;
                    }
                    
                    f32 target = releases.select_track[r] * imgw;
                    f32 ssx = releases.scrollx[r];
                    
                    if(!(releases.flags[r] & EntryFlags::transitioning))
                    {
                        if(ssx > target + (imgw/2) && releases.select_track[r]+1 < num_images)
                        {
                            ctx.scroll_delta.x = 0.0;
                            releases.select_track[r] += 1;
                            releases.flags[r] |= EntryFlags::transitioning;
                        }
                        else if(ssx < target - (imgw/2) && (ssize_t)releases.select_track[r]-1 >= 0)
                        {
                            ctx.scroll_delta.x = 0.0;
                            releases.select_track[r] -= 1;
                            releases.flags[r] |= EntryFlags::transitioning;
                        }
                        else
                        {
                            if(abs(scaled_vel) < 5.0)
                            {
                                releases.flags[r] |= EntryFlags::transitioning;
                            }
                        }
                    }
                    else
                    {
                        if(abs(ssx - target) < k_inertia_cutoff)
                        {
                            releases.scrollx[r] = target;
                            releases.flags[r] &= ~EntryFlags::transitioning;
                        }
                        else
                        {
                            if(abs(scaled_vel) < 5.0)
                            {
                                releases.scrollx[r] = lerp(ssx, target, k_snap_lerp);
                            }
                        }
                    }
                }
                else if (releases.flags[r] & EntryFlags::dragging)
                {
                    releases.scrollx[r] -= ctx.scroll_delta.x;
                    releases.scrollx[r] = std::max(releases.scrollx[r], 0.0f);
                    releases.scrollx[r] = std::min(releases.scrollx[r], max_scroll);
                    releases.flags[r] &= ~EntryFlags::transitioning;
                }
                
                ImGui::SetScrollX(releases.scrollx[r]);
                
                ImGui::PopStyleVar();
                ImGui::EndChild();
            }
            else
            {
                // TODO: this is not scrollable
                ImGui::Dummy(ImVec2(w, w));
            }
            
            // tracks
            ImGui::SetWindowFontScale(k_text_size_dots);
            s32 tc = releases.track_filepath_count[r];
            if(tc != 0)
            {
                auto ww = ImGui::GetWindowSize().x;
                auto tw = ImGui::CalcTextSize(ICON_FA_STOP_CIRCLE).x * releases.track_url_count[r] * 1.5f;
                ImGui::SetCursorPosX((ww - tw) * 0.5f);
                
                // dots
                for(u32 i = 0; i < releases.track_url_count[r]; ++i)
                {
                    if(i > 0)
                    {
                        ImGui::SameLine();
                    }
                    
                    u32 sel = releases.select_track[r];
                    if(i == sel)
                    {
                        if(ctx.top == r)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 0.0f, 1.0f));
                            ImGui::Text("%s", ICON_FA_PLAY);
                            ImGui::PopStyleColor();
                            
                            // load up the track
                            if(!(ctx.play_track_filepath == releases.track_filepaths[r][sel]))
                            {
                                ctx.play_track_filepath = releases.track_filepaths[r][sel];
                                ctx.invalidate_track = true;
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
            else
            {
                // no audio
                auto ww = ImGui::GetWindowSize().x;
                ImGui::SetCursorPosX(ww * 0.5f);
                ImGui::Text("%s", ICON_FA_TIMES_CIRCLE);
            }
            ImGui::SetWindowFontScale(k_text_size_body);
            
            ImGui::Spacing();
            ImGui::Indent();
            
            ImGui::SetWindowFontScale(k_text_size_h2);
                        
            ImGui::PushID("like");
            static bool debounce = false;
            if(releases.flags[r] & EntryFlags::liked)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(225.0f/255.0f, 48.0f/255.0f, 108.0f/255.0f, 1.0f));
                ImGui::Text("%s", ICON_FA_HEART);
                if(lenient_button_click(64.0f, debounce) && !ctx.scroll_lock_x && !ctx.scroll_lock_y)
                {
                    remove_like(releases.id[r]);
                    releases.flags[r] &= ~EntryFlags::liked;
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("%s", ICON_FA_HEART_O);
                if(lenient_button_click(64.0f, debounce) && !ctx.scroll_lock_x && !ctx.scroll_lock_y)
                {
                    add_like(releases.id[r]);
                    releases.flags[r] |= EntryFlags::liked;
                }
            }
            ImGui::PopID();
            
            ImGui::SameLine();
            ImGui::PushID("buy");
            if(releases.store_tags[r] & StoreTags::preorder)
            {
                ImGui::Text("%s", ICON_FA_CALENDAR_PLUS_O);
            }
            else
            {
                ImGui::Text("%s", ICON_FA_CART_PLUS);
            }
            
            if(lenient_button_click(64.0f, debounce) && !ctx.scroll_lock_x && !ctx.scroll_lock_y)
            {
                ctx.open_url_request = releases.link[r];
            }
            ImGui::PopID();
            
            if(releases.store_tags[r] & StoreTags::out_of_stock)
            {
                ImGui::SameLine();
                ImGui::Text("%s", ICON_FA_EXCLAMATION_TRIANGLE);
            }
            
            ImGui::SetWindowFontScale(k_text_size_body);
            
            if(releases.store_tags[r] & StoreTags::has_charted)
            {
                ImGui::SameLine();
                ImGui::Text("%s", ICON_FA_FIRE);
            }
            
            if(!(releases.store_tags[r] & StoreTags::out_of_stock))
            {
                if(releases.store_tags[r] & StoreTags::has_been_out_of_stock)
                {
                    ImGui::SameLine();
                    ImGui::Text("%s", ICON_FA_EXCLAMATION);
                }
            }
            
            // release info
            ImGui::TextWrapped("%s", artist.c_str());
            ImGui::TextWrapped("%s", title.c_str());
            
            // track name
            ImGui::SetWindowFontScale(k_text_size_track);
            u32 sel = releases.select_track[r];
            if(releases.track_name_count[r] > releases.select_track[r])
            {
                ImGui::TextWrapped("%s", releases.track_names[r][sel].c_str());
            }
            ImGui::SetWindowFontScale(k_text_size_body);
            
            ImGui::Unindent();
            
            ImGui::Spacing();
        }
        
        // couple of empty ones so we can reach the end of the feed
        ImGui::Dummy(ImVec2(w, w));
        ImGui::Dummy(ImVec2(w, w));
        
        // get scroll limit
        ctx.releases_scroll_maxy = ImGui::GetScrollMaxY() - w;
    }

    void audio_player()
    {
        auto& releases = ctx.view->releases;
        
        // audio player
        if(!ctx.mute)
        {
            static u32 si = -1;
            static u32 ci = -1;
            static u32 gi = -1;
            static bool started = false;
            
            if(ctx.top == -1)
            {
                // stop existing
                if(is_valid(si))
                {
                    // release existing
                    put::audio_channel_stop(ci);
                    put::audio_release_resource(si);
                    put::audio_release_resource(ci);
                    put::audio_release_resource(gi);
                    si = -1;
                    ci = -1;
                    gi = -1;
                    started = false;
                    ctx.play_track_filepath = "";
                }
            }
            
            if(ctx.play_track_filepath.length() > 0 && ctx.invalidate_track)
            {
                // stop existing
                if(is_valid(si))
                {
                    // release existing
                    put::audio_channel_stop(ci);
                    put::audio_release_resource(si);
                    put::audio_release_resource(ci);
                    put::audio_release_resource(gi);
                    si = -1;
                    ci = -1;
                    gi = -1;
                    started = false;
                }
                
                si = put::audio_create_stream(ctx.play_track_filepath.c_str());
                ci = put::audio_create_channel_for_sound(si);
                gi = put::audio_create_channel_group();
                
                put::audio_add_channel_to_group(ci, gi);
                put::audio_group_set_volume(gi, 1.0f);

                ctx.invalidate_track = false;
                started = false;
            }
            
            // playing
            if(is_valid(ci))
            {
                put::audio_group_state gstate;
                memset(&gstate, 0x0, sizeof(put::audio_group_state));
                put::audio_group_get_state(gi, &gstate);
                
                if(started && gstate.play_state == put::e_audio_play_state::not_playing)
                {
                    //
                    put::audio_channel_stop(ci);
                    put::audio_release_resource(si);
                    put::audio_release_resource(ci);
                    put::audio_release_resource(gi);
                    si = -1;
                    ci = -1;
                    gi = -1;
                    
                    // move to next
                    u32 next = releases.select_track[ctx.top] + 1;
                    if(next < releases.track_filepath_count[ctx.top])
                    {
                        ctx.scroll_delta.x = 0.0;
                        releases.select_track[ctx.top] += 1;
                        releases.flags[ctx.top] |= EntryFlags::transitioning;
                    }
                }
                else if(gstate.play_state == put::e_audio_play_state::playing)
                {
                    started = true;
                }
            }
        }
    }

    void issue_data_requests()
    {
        auto& releases = ctx.view->releases;
        
        // TODO: make the ranges data driven
        // make requests for data
        if(ctx.top != -1)
        {
            s32 range_start = max(ctx.top - 10, 0);
            s32 range_end = min<s32>(ctx.top + 10, (s32)releases.available_entries);
            
            for(size_t i = 0; i < releases.available_entries; ++i)
            {
                if(i >= range_start && i <= range_end) {
                    if(releases.artwork_texture[i] == 0) {
                        releases.flags[i] |= EntryFlags::artwork_requested;
                    }
                }
                else if (releases.flags[i] & EntryFlags::artwork_loaded){
                    if(releases.artwork_texture[i] != 0) {
                        // proper release
                        pen::renderer_release_texture(releases.artwork_texture[i]);
                        releases.artwork_texture[i] = 0;
                        releases.flags[i] &= ~EntryFlags::artwork_loaded;
                        releases.flags[i] &= ~EntryFlags::artwork_requested;
                    }
                }
            }
            std::atomic_thread_fence(std::memory_order_release);
        }
        
        // make requests for cache
        if(ctx.top != -1)
        {
            s32 range_start = max(ctx.top - 100, 0);
            s32 range_end = min<s32>(ctx.top + 100, (s32)releases.available_entries);
            
            for(size_t i = 0; i < releases.available_entries; ++i)
            {
                if(i >= range_start && i <= range_end) {
                    releases.flags[i] |= EntryFlags::cache_url_requested;
                }
                else {
                    releases.flags[i] &= ~EntryFlags::cache_url_requested;
                    
                    // TODO: art preloads
                }
            }
        }
    }

    void issue_open_url_requests()
    {
        // apply request for open url and handle it to ignore clicks that became drags
        if(!ctx.open_url_request.empty())
        {
            // open url if we hacent scrolled within 5 frames
            if(ctx.open_url_counter > 5)
            {
                pen::os_open_url(ctx.open_url_request);
                ctx.open_url_request = "";
                ctx.open_url_counter = 0;
            }
            else
            {
                ctx.open_url_counter++;
            }
                        
            // disable url opening if we began a scroll
            if(ctx.scroll_lock_x || ctx.scroll_lock_y)
            {
                ctx.open_url_request = "";
                ctx.open_url_counter = 0;
            }
        }
    }

    void apply_drags()
    {
        f32 w = ctx.w;
        
        f32 miny = (f32)w / k_top_pull_pad;
        
        // dragging and scrolling
        vec2f cur_scroll_delta = touch_screen_mouse_wheel();
        if(pen::input_is_mouse_down(PEN_MOUSE_L))
        {
            ctx.scroll_delta = cur_scroll_delta;
        }
        else
        {
            // apply inertia
            ctx.scroll_delta *= k_inertia;
            if(mag(ctx.scroll_delta) < k_inertia_cutoff) {
                ctx.scroll_delta = vec2f::zero();
            }
            
            // snap back to min top
            if(ctx.view->scroll.y < w) {
                ctx.view->scroll.y = lerp(ctx.view->scroll.y, (f32)w, 0.5f);
            }
            
            // release locks
            ctx.scroll_lock_x = false;
            ctx.scroll_lock_y = false;
        }
        
        // clamp to top
        if(ctx.view->scroll.y <= miny) {
            ctx.view->scroll.y = miny;
            ctx.scroll_delta = vec2f::zero();
        }
        
        f32 dx = abs(dot(ctx.scroll_delta, vec2f::unit_x()));
        f32 dy = abs(dot(ctx.scroll_delta, vec2f::unit_y()));
        
        ctx.side_drag = false;
        if(dx > dy)
        {
            ctx.side_drag = true;
        }
        
        if(!ctx.side_drag && abs(ctx.scroll_delta.y) > k_drag_threshold)
        {
            ctx.scroll_lock_y = true;
        }
        
        // only not if already scrolling x
        if(!ctx.scroll_lock_x)
        {
            ctx.view->scroll.y -= ctx.scroll_delta.y;
            ImGui::SetScrollY((ImGuiWindow*)ctx.releases_window, ctx.view->scroll.y);
        }
        
        // clamp to bottom
        if(ctx.view->scroll.y > ctx.releases_scroll_maxy) {
            ctx.view->scroll.y = std::min(ctx.view->scroll.y, ctx.releases_scroll_maxy);
        }
    }

    void settings_menu()
    {
        ImGui::SetWindowFontScale(k_text_size_h2);
        
        if(ImGui::CollapsingHeader("Help"))
        {
            ImGui::Text("%s - Released / Buy Link", ICON_FA_CART_PLUS);
            ImGui::Text("%s - Preorder / Buy Link", ICON_FA_CALENDAR_PLUS_O);
            ImGui::Text("%s - Sold Out", ICON_FA_EXCLAMATION_TRIANGLE);
            ImGui::Text("%s - Has Charted", ICON_FA_FIRE);
            ImGui::Text("%s - Has Previously Sold Out", ICON_FA_EXCLAMATION);
        }
        
        if(ImGui::CollapsingHeader("Contact / Invites"))
        {
            
        }
        
        if(ImGui::CollapsingHeader("Cache"))
        {
            float mb = (((float)ctx.data_ctx.cached_release_bytes.load()) / 1024.0 / 1024.0);
            ImGui::Text("Cached Releases: %i", ctx.data_ctx.cached_release_folders.load());
            ImGui::Text("Cached Data: %f(mb)", mb);
        }
        
        ImGui::SetWindowFontScale(k_text_size_body);
    }

    void main_window()
    {
        s32 w, h;
        pen::window_get_size(w, h);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((f32)w, (f32)h));
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
        
        ImGui::Dummy(ImVec2(0.0f, ctx.status_bar_height));
        
        // ui menus
        header_menu();
        
        if(ctx.view->view == View::settings)
        {
            settings_menu();
        }
        else
        {
            view_menu();
            view_reload();
            release_feed();
        }
        
        ImGui::EndChild();
        ImGui::PopStyleVar(4);
        ImGui::End();
    }

    void main_update()
    {
        apply_drags();
        audio_player();
        issue_data_requests();
        issue_open_url_requests();
    }

    loop_t user_update()
    {
        pen::timer_start(frame_timer);
        pen::renderer_new_frame();
        
        // clear backbuffer
        pen::renderer_set_targets(PEN_BACK_BUFFER_COLOUR, PEN_BACK_BUFFER_DEPTH);
        pen::renderer_clear(clear_screen);

        put::dev_ui::new_frame();
        
        // main code entry
        main_window();
        main_update();
        
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

    void* user_setup(void* params)
    {
        // unpack the params passed to the thread and signal to the engine it ok to proceed
        pen::job_thread_params* job_params = (pen::job_thread_params*)params;
        s_thread_info = job_params->job_info;
        pen::semaphore_post(s_thread_info->p_sem_continue, 1);

        // force audio in silent mode
        pen::os_ignore_slient();
        
        // get window size
        pen::window_get_size(ctx.w, ctx.h);
        
        // base ratio taken from iphone11 max dimension
        f32 font_ratio = 42.0f / 1125.0f;
        f32 font_pixel_size = ctx.w * font_ratio;
        
        // intialise pmtech systems
        pen::jobs_create_job(put::audio_thread_function, 1024 * 10, nullptr, pen::e_thread_start_flags::detached);
        dev_ui::init(dev_ui::default_pmtech_style(), font_pixel_size);
        
        curl::init();
                
        // init context
        ctx.status_bar_height = pen::os_get_status_bar_portrait_height();

        // permanent workers
        pen::thread_create(registry_loader, 10 * 1024 * 1024, &ctx.data_ctx, pen::e_thread_start_flags::detached);
        pen::thread_create(user_data_thread, 10 * 1024 * 1024, ctx.view, pen::e_thread_start_flags::detached);

        // enter initial view, the inputs can be serialised
        change_view(View::latest, Tags::all);

        // timer
        frame_timer = pen::timer_create();
        pen::timer_start(frame_timer);
        
        // for clearing the backbuffer
        clear_state cs;
        cs.r = 1.0f;
        cs.g = 1.0f;
        cs.b = 1.0f;
        cs.a = 1.0f;
        cs.depth = 0.0f;
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
} // namespace

void* pen::user_entry( void* params )
{
    return PEN_THREAD_OK;
}
