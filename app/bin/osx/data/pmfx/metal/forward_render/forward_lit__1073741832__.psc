#include <metal_stdlib>
using namespace metal;
#define BUF_OFFSET 4
// compute texture
#define texture_2d_rw( name, index ) texture2d<float, access::read_write> name [[texture(index)]]
#define texture_2d_r( name, index ) texture2d<float, access::read> name [[texture(index)]]
#define texture_2d_w( name, index ) texture2d<float, access::write> name [[texture(index)]]
#define texture_3d_rw( name, index ) texture3d<float, access::read_write> name [[texture(index)]]
#define texture_3d_r( name, index ) texture3d<float, access::read> name [[texture(index)]]
#define texture_3d_w( name, index ) texture3d<float, access::write> name [[texture(index)]]
#define texture_2d_array_rw( name, index ) texture2d_array<float, access::read_write> name [[texture(index)]]
#define texture_2d_array_r( name, index ) texture2d_array<float, access::read> name [[texture(index)]]
#define texture_2d_array_w( name, index ) texture2d_array<float, access::write> name [[texture(index)]]
#define read_texture( name, gid ) name.read(gid)
#define write_texture( name, val, gid ) name.write(val, gid)
#define read_texture_array( name, gid, slice ) name.read(gid, uint(slice))
#define write_texture_array( name, val, gid, slice ) name.write(val, gid, uint(slice))
// standard texture
#define texture_2d( name, sampler_index ) texture2d<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_3d( name, sampler_index ) texture3d<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_2dms( type, samples, name, sampler_index ) texture2d_ms<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_cube( name, sampler_index ) texturecube<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_2d_array( name, sampler_index ) texture2d_array<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_cube_array( name, sampler_index ) texturecube_array<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define texture_2d_external( name, sampler_index ) texture_2d( name, sampler_index )
// depth texture
#define depth_2d( name, sampler_index ) depth2d<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define depth_2d_array( name, sampler_index ) depth2d_array<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define depth_cube( name, sampler_index ) depthcube<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
#define depth_cube_array( name, sampler_index ) depthcube_array<float> name [[texture(sampler_index)]], sampler sampler_##name [[sampler(sampler_index)]]
// _arg macros are used to pass textures through functions from main
#define texture_2d_arg( name ) thread texture2d<float>& name, thread sampler& sampler_##name
#define texture_2d_external_arg( name ) texture_2d_arg(name)
#define texture_3d_arg( name ) thread texture3d<float>& name, thread sampler& sampler_##name
#define texture_2dms_arg( name ) thread texture2d_ms<float>& name, thread sampler& sampler_##name
#define texture_cube_arg( name ) thread texturecube<float>& name, thread sampler& sampler_##name
#define texture_2d_array_arg( name ) thread texture2d_array<float>& name, thread sampler& sampler_##name
#define texture_cube_array_arg( name ) thread texturecube_array<float>& name, thread sampler& sampler_##name
#define depth_2d_arg( name ) thread depth2d<float>& name, thread sampler& sampler_##name
#define depth_2d_array_arg( name ) thread depth2d_array<float>& name, thread sampler& sampler_##name
#define depth_cube_arg( name ) thread depthcube<float>& name, thread sampler& sampler_##name
#define depth_cube_array_arg( name ) thread depthcube_array<float>& name, thread sampler& sampler_##name
// structured buffers
#define structured_buffer_rw( type, name, index ) device type* name [[buffer(index+BUF_OFFSET)]]
#define structured_buffer_rw_arg( type, name, index ) device type* name [[buffer(index+BUF_OFFSET)]]
#define structured_buffer( type, name, index ) constant type* name [[buffer(index+BUF_OFFSET)]]
#define structured_buffer_arg( type, name, index ) constant type* name [[buffer(index+BUF_OFFSET)]]
// sampler
#define sample_texture( name, tc ) name.sample(sampler_##name, tc)
#define sample_texture_2dms( name, x, y, fragment ) name.read(uint2(x, y), fragment)
#define sample_texture_level( name, tc, l ) name.sample(sampler_##name, tc, level(l))
#define sample_texture_grad( name, tc, vddx, vddy ) name.sample(sampler_##name, tc, gradient3d(vddx, vddy))
#define sample_texture_array( name, tc, a ) name.sample(sampler_##name, tc, uint(a))
#define sample_texture_array_level( name, tc, a, l ) name.sample(sampler_##name, tc, uint(a), level(l))
#define sample_texture_cube_array( name, tc, a ) sample_texture_array(name, tc, a)
#define sample_texture_cube_array_level( name, tc, a, l ) sample_texture_array_level(name, tc, a, l)
// sampler compare / gather
#define sample_depth_compare( name, tc, compare_value ) name.sample_compare(sampler_##name, tc, compare_value, min_lod_clamp(0))
#define sample_depth_compare_array( name, tc, a, compare_value ) name.sample_compare(sampler_##name, tc, uint(a), compare_value, min_lod_clamp(0))
#define sample_depth_compare_cube( name, tc, compare_value ) name.sample_compare(sampler_##name, tc, compare_value, min_lod_clamp(0))
#define sample_depth_compare_cube_array( name, tc, a, compare_value ) name.sample_compare(sampler_##name, tc, uint(a), compare_value, min_lod_clamp(0))
// matrix
#define to_3x3( M4 ) float3x3(M4[0].xyz, M4[1].xyz, M4[2].xyz)
#define from_columns_2x2(A, B) (transpose(float2x2(A, B)))
#define from_rows_2x2(A, B) (float2x2(A, B))
#define from_columns_3x3(A, B, C) (transpose(float3x3(A, B, C)))
#define from_rows_3x3(A, B, C) (float3x3(A, B, C))
#define from_columns_4x4(A, B, C, D) (transpose(float4x4(A, B, C, D)))
#define from_rows_4x4(A, B, C, D) (float4x4(A, B, C, D))
#define mul( A, B ) ((A) * (B))
#define mul_tbn( A, B ) ((B) * (A))
#define unpack_vb_instance_mat( mat, r0, r1, r2, r3 ) mat[0] = r0; mat[1] = r1; mat[2] = r2; mat[3] = r3;
#define to_data_matrix(mat) mat
// clip
#define remap_z_clip_space( d ) (d = d * 0.5 + 0.5)
#define remap_ndc_ray( r ) float2(r.x, r.y)
#define remap_depth( d ) (d)
// defs
#define ddx dfdx
#define ddy dfdy
#define discard discard_fragment()
#define lerp mix
#define frac fract
#define mod(x, y) (x - y * floor(x/y))
#define _pmfx_unroll
#define _pmfx_loop
#define	read3 uint3
#define read2 uint2
// atomics
#define atomic_counter(name, index) structured_buffer_rw(atomic_uint, name, index)
#define atomic_load(atomic, original) original = atomic_load_explicit(&atomic, memory_order_relaxed)
#define atomic_store(atomic, value) atomic_store_explicit(&atomic, value, memory_order_relaxed)
#define atomic_increment(atomic, original) original = atomic_fetch_add_explicit(&atomic, 1, memory_order_relaxed)
#define atomic_decrement(atomic, original) original = atomic_fetch_sub_explicit(&atomic, 1, memory_order_relaxed)
#define atomic_add(atomic, value, original) original = atomic_fetch_add_explicit(&atomic, value, memory_order_relaxed)
#define atomic_subtract(atomic, value, original) original = atomic_fetch_sub_explicit(&atomic, value, memory_order_relaxed)
#define atomic_min(atomic, value, original) original = atomic_fetch_min_explicit(&atomic, value, memory_order_relaxed)
#define atomic_max(atomic, value, original) original = atomic_fetch_max_explicit(&atomic, value, memory_order_relaxed)
#define atomic_and(atomic, value, original) original = atomic_fetch_and_explicit(&atomic, value, memory_order_relaxed)
#define atomic_or(atomic, value, original) original = atomic_fetch_or_explicit(&atomic, value, memory_order_relaxed)
#define atomic_xor(atomic, value, original) original = atomic_fetch_xor_explicit(&atomic, value, memory_order_relaxed)
#define atomic_exchange(atomic, value, original) original = atomic_exchange_explicit(&atomic, value, memory_order_relaxed)
#define threadgroup_barrier() threadgroup_barrier(mem_flags::mem_threadgroup)
#define device_barrier() threadgroup_barrier(mem_flags::mem_device)
#define chebyshev_normalize( V ) (V.xyz / max( max(abs(V.x), abs(V.y)), abs(V.z) ))
#define max3(v) max(max(v.x, v.y),v.z)
#define max4(v) max(max(max(v.x, v.y),v.z), v.w)
#define PI 3.14159265358979323846264
struct light_data
{
    float4 pos_radius;
    float4 dir_cutoff;
    float4 colour;
    float4 data;
};
struct distance_field_shadow
{
    float4x4 world_matrix;
    float4x4 world_matrix_inv;
};
struct area_light_data
{
    float4 corners[4];
    float4 colour;
};
struct c_per_pass_view
{
    float4x4 vp_matrix;
    float4x4 view_matrix;
    float4x4 vp_matrix_inverse;
    float4x4 view_matrix_inverse;
    float4 camera_view_pos;
    float4 camera_view_dir;
};
struct c_per_draw_call
{
    float4x4 world_matrix;
    float4 user_data;
    float4 user_data2;
    float4x4 world_matrix_inv_transpose;
};
struct c_per_pass_lights
{
    float4 light_info;
    light_data lights[100];
};
struct c_per_pass_shadow
{
    float4x4 shadow_matrix[100];
};
struct c_per_pass_shadow_distance_fields
{
    distance_field_shadow sdf_shadow;
};
struct c_per_pass_area_lights
{
    float4 area_light_info;
    area_light_data area_lights[10];
};
struct c_material_data
{
    float4 m_albedo;
    float m_roughness;
    float m_reflectivity;
    float m_surface_offset;
    float m_padding;
};
struct vs_output
{
    float4 position;
    float4 world_pos;
    float3 normal;
    float3 tangent;
    float3 bitangent;
    float4 texcoord;
    float4 colour;
};
struct ps_output
{
    float4 colour [[color(0)]];
};
float3 cook_torrence(
float4 light_pos_radius,
float3 light_colour,
float3 n,
float3 world_pos,
float3 view_pos,
float3 albedo,
float3 metalness,
float roughness,
float reflectivity
)
{
    float3 l = normalize( light_pos_radius.xyz - world_pos.xyz );
    float n_dot_l = dot( n, l );
    if( n_dot_l > 0.0f )
    {
        float roughness_sq = roughness * roughness;
        float k = reflectivity;
        float3 v_view = normalize( (view_pos.xyz - world_pos.xyz) );
        float3 hv = normalize( v_view + l );
        float n_dot_v = dot( n, v_view );
        float n_dot_h = dot( n, hv );
        float v_dot_h = dot( v_view, hv );
        float n_dot_h_2 = 2.0f * n_dot_h;
        float g1 = (n_dot_h_2 * n_dot_v) / v_dot_h;
        float g2 = (n_dot_h_2 * n_dot_l) / v_dot_h;
        float geom_atten = min(1.0, min(g1, g2));
        float r1 = 1.0f / ( 4.0f * roughness_sq * pow(n_dot_h, 4.0f));
        float r2 = (n_dot_h * n_dot_h - 1.0) / (roughness_sq * n_dot_h * n_dot_h);
        float roughness_atten = r1 * exp(r2);
        float fresnel = pow(1.0 - v_dot_h, 5.0);
        fresnel *= roughness;
        fresnel += reflectivity;
        float specular = (fresnel * geom_atten * roughness_atten) / (n_dot_v * n_dot_l * PI);
        float3 lit_colour = light_colour * specular * n_dot_l;
        return saturate(lit_colour);
    }
    return float3( 0.0, 0.0, 0.0 );
}
float3 oren_nayar(
float4 light_pos_radius,
float3 light_colour,
float3 n,
float3 world_pos,
float3 view_pos,
float roughness,
float3 albedo)
{
    float3 v = normalize(view_pos-world_pos);
    float3 l = normalize(light_pos_radius.xyz-world_pos);
    float l_dot_v = dot(l, v);
    float n_dot_l = dot(n, l);
    float n_dot_v = dot(n, v);
    float s = l_dot_v - n_dot_l * n_dot_v;
    float t = lerp(1.0, max(n_dot_l, n_dot_v), step(0.0, s));
    float lum = length( albedo );
    float sigma2 = roughness * roughness;
    float A = 1.0 + sigma2 * (lum / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);
    return ( albedo * light_colour * max(0.0, n_dot_l) * (A + B * s / t) / 3.14159265 );
}
float spot_light_attenuation(
float4 light_pos_radius,
float4 light_dir_cutoff,
float falloff,
float3 world_pos)
{
    float co = light_dir_cutoff.w;
    float3 vl = normalize(world_pos.xyz - light_pos_radius.xyz);
    float3 sd = normalize(light_dir_cutoff.xyz);
    float dp = (1.0 - dot(vl, sd));
    return smoothstep(co, co - falloff, dp);
}
float point_light_attenuation_cutoff(
float4 light_pos_radius,
float3 world_pos)
{
    float r = light_pos_radius.w;
    float d = length(world_pos.xyz - light_pos_radius.xyz);
    d = max(d - r, 0.0);
    float denom = d/r + 1.0;
    float attenuation = 1.0 / (denom*denom);
    float cutoff = 0.2;
    attenuation = (attenuation - cutoff) / (1.0 - cutoff);
    attenuation = max(attenuation, 0.0);
    return attenuation;
}
float sample_shadow_array_pcf_9(
depth_2d_array_arg(shadowmap_texture),
float array_slice,
float3 sp)
{
    float2 samples[9];
    float2 inv_sm_size = float2(1.0/2048.0, 1.0/2048.0);
    samples[0] = float2(-1.0, -1.0) * inv_sm_size;
    samples[1] = float2(-1.0, 0.0) * inv_sm_size;
    samples[2] = float2(-1.0, 1.0) * inv_sm_size;
    samples[3] = float2(0.0, -1.0) * inv_sm_size;
    samples[4] = float2(0.0, 0.0) * inv_sm_size;
    samples[5] = float2(0.0, 1.0) * inv_sm_size;
    samples[6] = float2(1.0, -1.0) * inv_sm_size;
    samples[7] = float2(1.0, 0.0) * inv_sm_size;
    samples[8] = float2(1.0, 1.0) * inv_sm_size;
    float shadow = 0.0;
    for(int j = 0; j < 9; ++j)
    {
        shadow += sample_depth_compare_array(shadowmap_texture, sp.xy + samples[j], array_slice, sp.z);
    }
    shadow /= 9.0;
    return shadow;
}
bool ray_vs_aabb(float3 emin,
float3 emax,
float3 r1,
float3 rv,
thread float3& intersection)
{
    float3 dirfrac = float3(1.0, 1.0, 1.0) / rv;
    float t1 = (emin.x - r1.x)*dirfrac.x;
    float t2 = (emax.x - r1.x)*dirfrac.x;
    float t3 = (emin.y - r1.y)*dirfrac.y;
    float t4 = (emax.y - r1.y)*dirfrac.y;
    float t5 = (emin.z - r1.z)*dirfrac.z;
    float t6 = (emax.z - r1.z)*dirfrac.z;
    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));
    float t = 0.0;
    if (tmax < 0.0)
    {
        t = tmax;
        return false;
    }
    if (tmin > tmax)
    {
        t = tmax;
        return false;
    }
    t = tmin;
    intersection = r1 + rv * t;
    return true;
}
float sdf_shadow_trace(
texture_3d_arg(sdf_volume),
float max_samples,
float3 light_pos,
float3 world_pos,
float3 scale,
float3 ray_origin,
float4x4 inv_mat,
float3x3 inv_rot)
{
    float3 ray_dir = normalize(light_pos - world_pos);
    ray_dir = normalize( mul( ray_dir, inv_rot ) );
    float closest = 1.0;
    float3 uvw = ray_origin;
    if(abs(uvw.x) >= 1.0 || abs(uvw.y) >= 1.0 || abs(uvw.z) >= 1.0)
    {
        float3 emin = float3(-1.0, -1.0, -1.0);
        float3 emax = float3(1.0, 1.0, 1.0);
        float3 ip = float3(0.0, 0.0, 0.0);
        bool hit = ray_vs_aabb( emin, emax, uvw, ray_dir, ip);
        uvw = ip;
        if(!hit)
        {
            return closest;
        }
    }
    float3 light_uvw = mul( float4(light_pos, 1.0), inv_mat ).xyz * 0.5 + 0.5;
    uvw = uvw * 0.5 + 0.5;
    float3 v1 = normalize(light_uvw - uvw);
    for( int s = 0; s < int(max_samples); ++s )
    {
        float d = sample_texture_level( sdf_volume, uvw, 0.0 ).r;
        closest = min(d, closest);
        ray_dir = normalize(light_uvw - uvw);
        float3 step = ray_dir.xyz * float3(d, d, d) / scale * 0.7;
        uvw += step;
        if( d <= 0.0 )
        {
            closest = max( d, 0.0 );
            break;
        }
        if(uvw.x >= 1.0 || uvw.x < 0.0)
        break;
        if(uvw.y >= 1.0 || uvw.y < 0.0)
        break;
        if(uvw.z >= 1.0 || uvw.z < 0.0)
        break;
    }
    return closest;
}
float integrate_edge(float3 v1,
float3 v2)
{
    float cos_theta = dot(v1, v2);
    float theta = acos(cos_theta);
    float res = cross(v1, v2).z * ((theta > 0.001) ? theta/sin(theta) : 1.0);
    return res;
}
void clip_quad_to_horizon(thread float3* L,
thread int& n)
{
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;
    n = 0;
    if (config == 0)
    {
    }
    else if (config == 1)
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2)
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3)
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4)
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5)
    {
        n = 0;
    }
    else if (config == 6)
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7)
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8)
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9)
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10)
    {
        n = 0;
    }
    else if (config == 11)
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12)
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13)
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14)
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15)
    {
        n = 4;
    }
    if (n == 3)
    L[3] = L[0];
    if (n == 4)
    L[4] = L[0];
}
float3 ltc_uv_coord(float3 p[4])
{
    float3 v1 = p[1] - p[0];
    float3 v2 = p[3] - p[0];
    float3 plane_ortho = (cross(v1, v2));
    float plane_area_squared = dot(plane_ortho, plane_ortho);
    float plane_distx_plane_area = dot(plane_ortho, p[0]);
    float3 pp = plane_distx_plane_area * plane_ortho / plane_area_squared - p[0];
    float v1_dot_v2 = dot(v1, v2);
    float inv_v1_dot_v1 = 1.0 / dot(v1, v1);
    float3 vv2 = v2 - v1 * v1_dot_v2 * inv_v1_dot_v1;
    float2 puv;
    puv.y = dot(vv2, pp) / dot(vv2, vv2);
    puv.x = dot(v1, pp) * inv_v1_dot_v1 - v1_dot_v2 * inv_v1_dot_v1 * puv.y;
    float d = abs(plane_distx_plane_area) / pow(plane_area_squared, 0.75);
    return float3(puv, d);
}
float4 ltc_evaluate(
float3 n,
float3 v,
float3 p,
float3x3 minv,
float3 points[4],
bool two_sided)
{
    float3 t1, t2;
    t1 = normalize(v - n * dot(v, n));
    t2 = cross(n, t1);
    float3x3 ttn = from_columns_3x3(t1, t2, n);
    minv = mul(minv, ttn);
    float3 l[5];
    l[0] = mul(minv, points[0] - p);
    l[1] = mul(minv, points[1] - p);
    l[2] = mul(minv, points[2] - p);
    l[3] = mul(minv, points[3] - p);
    l[4] = l[3];
    float3 ll[4];
    ll[0] = l[0];
    ll[1] = l[1];
    ll[2] = l[2];
    ll[3] = l[3];
    float3 uvl = ltc_uv_coord(ll);
    int nc;
    clip_quad_to_horizon(l, nc);
    if (nc == 0)
    return float4(0, 0, 0, 0.0);
    l[0] = normalize(l[0]);
    l[1] = normalize(l[1]);
    l[2] = normalize(l[2]);
    l[3] = normalize(l[3]);
    l[4] = normalize(l[4]);
    float sum = 0.0;
    sum += integrate_edge(l[0], l[1]);
    sum += integrate_edge(l[1], l[2]);
    sum += integrate_edge(l[2], l[3]);
    if (nc >= 4)
    sum += integrate_edge(l[3], l[4]);
    if (nc == 5)
    sum += integrate_edge(l[4], l[0]);
    sum = two_sided ? abs(sum) : max(0.0, sum);
    float3 lo_i = float3(sum, sum, sum);
    return float4(uvl.x, uvl.y, uvl.z, sum);
}
float ltc_evaluate_cc(
float3 n,
float3 v,
float3 p,
float3x3 minv,
float3 points[4],
bool two_sided)
{
    float3 t1, t2;
    t1 = normalize(v - n * dot(v, n));
    t2 = cross(n, t1);
    float3x3 ttn = from_columns_3x3(t1, t2, n);
    minv = mul(minv, ttn);
    float3 l[5];
    for(int i = 0; i < 4; ++i)
    l[i] = mul(minv, points[i] - p);
    l[4] = l[3];
    int nc;
    clip_quad_to_horizon(l, nc);
    if (nc == 0)
    return 0.0;
    for(int i = 0; i < 5; ++i)
    l[i] = normalize(l[i]);
    float sum = 0.0;
    sum += integrate_edge(l[0], l[1]);
    sum += integrate_edge(l[1], l[2]);
    sum += integrate_edge(l[2], l[3]);
    if (nc >= 4)
    sum += integrate_edge(l[3], l[4]);
    if (nc == 5)
    sum += integrate_edge(l[4], l[0]);
    sum = two_sided ? abs(sum) : max(0.0, sum);
    return sum;
}
float4 area_light_specular_uv(
texture_2d_arg(ltc_mat),
texture_2d_arg(ltc_mag),
float3 points[4],
float3 pos,
float roughness,
float3 n,
float3 v)
{
    float pi = 3.14159265359;
    float lut_size = 64.0;
    float lut_scale = (lut_size - 1.0)/lut_size;
    float lut_bias = 0.5/lut_size;
    float theta = acos(dot(n, v));
    float2 uv = float2(roughness, theta / (0.5 * pi));
    uv = uv * lut_scale + lut_bias;
    float4 mat = sample_texture(ltc_mat, uv);
    float mag = sample_texture(ltc_mag, uv).w;
    float3x3 minv = from_rows_3x3(
    float3(1.0, 0.0, mat.y),
    float3(0.0, mat.z, 0.0),
    float3(mat.w, 0.0, mat.x)
    );
    float4 spec = ltc_evaluate(n, v, pos, minv, points, true);
    return spec;
}
float area_light_specular(
texture_2d_arg(ltc_mat),
texture_2d_arg(ltc_mag),
float3 points[4],
float3 pos,
float roughness,
float3 n,
float3 v)
{
    float pi = 3.14159265359;
    float lut_size = 64.0;
    float lut_scale = (lut_size - 1.0)/lut_size;
    float lut_bias = 0.5/lut_size;
    float theta = acos(dot(n, v));
    float2 uv = float2(roughness, theta / (0.5 * pi));
    uv = uv * lut_scale + lut_bias;
    float4 mat = sample_texture(ltc_mat, uv);
    float mag = sample_texture(ltc_mag, uv).w;
    float3x3 minv = from_rows_3x3(
    float3(1.0, 0.0, mat.y),
    float3(0.0, mat.z, 0.0),
    float3(mat.w, 0.0, mat.x)
    );
    float spec = ltc_evaluate_cc(n, v, pos, minv, points, true);
    return spec;
}
float4 area_light_diffuse_uv(
float3 points[4],
float3 pos,
float3 n,
float3 v)
{
    float3x3 difv = float3x3(
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
    );
    float4 diff = ltc_evaluate(n, v, pos, difv, points, true);
    return diff;
}
float area_light_diffuse(
float3 points[4],
float3 pos,
float3 n,
float3 v)
{
    float3x3 difv = float3x3(
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
    );
    float diff = ltc_evaluate_cc(n, v, pos, difv, points, true);
    return diff;
}
float3 transform_ts_normal( float3 t,
float3 b,
float3 n,
float3 ts_normal )
{
    float3x3 tbn;
    tbn[0] = float3(t.x, b.x, n.x);
    tbn[1] = float3(t.y, b.y, n.y);
    tbn[2] = float3(t.z, b.z, n.z);
    return normalize( mul_tbn( tbn, ts_normal ) );
}
fragment ps_output ps_main(vs_output input [[stage_in]]
, texture_2d( diffuse_texture, 0 )
, texture_2d( normal_texture, 1 )
, texture_2d( specular_texture, 2 )
, texture_3d( sdf_volume, 14 )
, texture_2d( ltc_mat, 13 )
, texture_2d( ltc_mag, 12 )
, depth_cube_array( omni_shadow_texture, 10 )
, texture_2d_array( area_light_textures, 11 )
, depth_2d_array( shadowmap_texture, 15 )
, constant c_per_pass_view &per_pass_view [[buffer(4)]]
, constant c_per_draw_call &per_draw_call [[buffer(5)]]
, constant c_per_pass_lights &per_pass_lights [[buffer(7)]]
, constant c_per_pass_shadow &per_pass_shadow [[buffer(8)]]
, constant c_per_pass_shadow_distance_fields &per_pass_shadow_distance_fields [[buffer(9)]]
, constant c_per_pass_area_lights &per_pass_area_lights [[buffer(10)]]
, constant c_material_data &material_data [[buffer(11)]])
{
    constant float4& camera_view_pos = per_pass_view.camera_view_pos;
    constant float4x4& world_matrix = per_draw_call.world_matrix;
    constant float4& user_data = per_draw_call.user_data;
    constant float4& light_info = per_pass_lights.light_info;
    constant light_data* lights = &per_pass_lights.lights[0];
    constant float4x4* shadow_matrix = &per_pass_shadow.shadow_matrix[0];
    constant distance_field_shadow& sdf_shadow = per_pass_shadow_distance_fields.sdf_shadow;
    constant float4& area_light_info = per_pass_area_lights.area_light_info;
    constant area_light_data* area_lights = &per_pass_area_lights.area_lights[0];
    constant float& m_roughness = material_data.m_roughness;
    constant float& m_reflectivity = material_data.m_reflectivity;
    constant float& m_surface_offset = material_data.m_surface_offset;
    ps_output output;
    float4 albedo = sample_texture( diffuse_texture, input.texcoord.xy );
    float3 normal_sample = sample_texture( normal_texture, input.texcoord.xy ).rgb;
    float4 ro_sample = sample_texture( specular_texture, input.texcoord.xy );
    float4 specular_sample = float4(1.0, 1.0, 1.0, 1.0);
    normal_sample = normal_sample * 2.0 - 1.0;
    float3 n = transform_ts_normal(
    input.tangent,
    input.bitangent,
    input.normal,
    normal_sample );
    albedo *= input.colour;
    float4 metalness = float4(0.0, 0.0, 0.0, 1.0);
    float3 lit_colour = float3( 0.0, 0.0, 0.0 );
    float reflectivity = saturate(user_data.z);
    float roughness = saturate(user_data.y);
    reflectivity = m_reflectivity;
    roughness = ro_sample.r;
    roughness = input.colour.a;
    albedo.a = 1.0;
    n = input.normal.rgb;
    roughness = m_roughness;
    float max_samples = 128.0;
    float3x3 inv_rot = to_3x3(sdf_shadow.world_matrix_inv);
    float3 r1 = input.world_pos.xyz + input.normal.xyz * m_surface_offset;
    float3 tr1 = mul( float4(r1, 1.0), sdf_shadow.world_matrix_inv ).xyz;
    float3 scale = float3(length(sdf_shadow.world_matrix[0].xyz), length(sdf_shadow.world_matrix[1].xyz), length(sdf_shadow.world_matrix[2].xyz)) * 2.0;
    float3 vddx = ddx( r1 );
    float3 vddy = ddy( r1 );
    float t = 1.0;
    float3 lll = float3(0.0, 0.0, 0.0);
    int shadow_map_index = 0;
    _pmfx_loop
    for( int i = 0; i < int(light_info.x); ++i )
    {
        float3 light_col = float3( 0.0, 0.0, 0.0 );
        light_col += cook_torrence(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        albedo.rgb,
        metalness.rgb,
        roughness,
        reflectivity
        );
        light_col += oren_nayar(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        roughness,
        albedo.rgb
        );
        float s = sdf_shadow_trace(sdf_volume, sampler_sdf_volume, max_samples, lights[i].pos_radius.xyz, input.world_pos.xyz, scale, tr1, sdf_shadow.world_matrix_inv, inv_rot);
        light_col *= smoothstep( 0.0, 0.1, s);
        if( lights[i].colour.a == 0.0 )
        {
            lit_colour += light_col;
            continue;
        }
        else
        {
            float shadow = 1.0;
            float d = 1.0;
            float4 offset_pos = float4(input.world_pos.xyz + n.xyz * 0.01, 1.0);
            float4 sp = mul( offset_pos, shadow_matrix[i] );
            sp.xyz /= sp.w;
            sp.y *= -1.0;
            sp.xy = sp.xy * 0.5 + 0.5;
            sp.z = remap_depth(sp.z);
            shadow = sample_shadow_array_pcf_9(shadowmap_texture, sampler_shadowmap_texture, float(shadow_map_index), sp.xyz);
            lit_colour += light_col * shadow;
            ++shadow_map_index;
        }
    }
    int point_start = int(light_info.x);
    int point_end = int(light_info.x) + int(light_info.y);
    int omni_shadow_index = 0;
    _pmfx_loop
    for( int i = point_start; i < point_end; ++i )
    {
        float3 light_col = float3( 0.0, 0.0, 0.0 );
        light_col += cook_torrence(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        albedo.rgb,
        metalness.rgb,
        roughness,
        reflectivity
        );
        light_col += oren_nayar(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        roughness,
        albedo.rgb
        );
        float a = point_light_attenuation_cutoff( lights[i].pos_radius, input.world_pos.xyz );
        light_col *= a;
        float s = sdf_shadow_trace(sdf_volume, sampler_sdf_volume, max_samples, lights[i].pos_radius.xyz, input.world_pos.xyz, scale, tr1, sdf_shadow.world_matrix_inv, inv_rot);
        light_col *= smoothstep( 0.0, 0.1, s);
        if( lights[i].colour.a == 0.0)
        {
            lit_colour += light_col;
            continue;
        }
        else
        {
            float3 to_light = (input.world_pos.xyz - lights[i].pos_radius.xyz);
            float d = length(to_light) / 2.0;
            float3 cv = normalize(to_light) * float3(1.0, 1.0, -1.0);
            d /= lights[i].pos_radius.w;
            d -= 0.00025f;
            float ll = sample_depth_compare_cube_array(omni_shadow_texture, cv, float(omni_shadow_index), d);
            lit_colour += light_col * ll;
            ++omni_shadow_index;
        }
    }
    int spot_start = point_end;
    int spot_end = spot_start + int(light_info.z);
    _pmfx_loop
    for(int i = spot_start; i < spot_end; ++i )
    {
        float3 light_col = float3( 0.0, 0.0, 0.0 );
        light_col += cook_torrence(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        albedo.rgb,
        metalness.rgb,
        roughness,
        reflectivity
        );
        light_col += oren_nayar(
        lights[i].pos_radius,
        lights[i].colour.rgb,
        n,
        input.world_pos.xyz,
        camera_view_pos.xyz,
        roughness,
        albedo.rgb
        );
        float a = spot_light_attenuation(lights[i].pos_radius,
        lights[i].dir_cutoff,
        lights[i].data.x,
        input.world_pos.xyz );
        light_col *= a;
        float s = sdf_shadow_trace(sdf_volume, sampler_sdf_volume, max_samples, lights[i].pos_radius.xyz, input.world_pos.xyz, scale, tr1, sdf_shadow.world_matrix_inv, inv_rot);
        light_col *= smoothstep( 0.0, 0.1, s);
        if( lights[i].colour.a == 0.0 )
        {
            lit_colour += light_col;
            continue;
        }
        else
        {
            float shadow = 1.0;
            float d = 1.0;
            float4 offset_pos = float4(input.world_pos.xyz + n.xyz * 0.01, 1.0);
            float4 sp = mul( offset_pos, shadow_matrix[shadow_map_index] );
            sp.xyz /= sp.w;
            sp.y *= -1.0;
            sp.xy = sp.xy * 0.5 + 0.5;
            sp.z = remap_depth(sp.z);
            shadow = sample_shadow_array_pcf_9(shadowmap_texture, sampler_shadowmap_texture, float(shadow_map_index), sp.xyz);
            lit_colour += light_col * shadow;
            ++shadow_map_index;
        }
    }
    {
        float pi = 3.14159265359;
        int num_area_lights = int(area_light_info.x);
        for(int i = 0; i < num_area_lights; ++i)
        {
            float3 v = -normalize(input.world_pos.xyz - camera_view_pos.xyz);
            float3 pos = input.world_pos.xyz;
            float3 points[4];
            for(int j = 0; j < 4; ++j)
            points[j] = area_lights[i].corners[j].xyz;
            float diff_sum = area_light_diffuse(points, pos, n, v);
            float3 diff = area_lights[i].colour.rgb * diff_sum;
            float spec_sum = area_light_specular(ltc_mat, sampler_ltc_mat, ltc_mag, sampler_ltc_mag, points, pos, ro_sample.x, n, v);
            float3 spec = area_lights[i].colour.rgb * spec_sum;
            float3 light_col = (spec.rgb + diff.rgb) / (2.0 * pi);
            lit_colour += light_col;
        }
        int ts = num_area_lights;
        int num_area_lights_textured = int(area_light_info.y);
        for(int i = ts; i < ts + num_area_lights_textured; ++i)
        {
            float slice = area_lights[i].colour.w;
            float levels = 8.0;
            float2 inv_texel = float2(1.0/640.0, 1.0/480.0);
            float2 inv_texel_x = float2(1.0, 1.0) - inv_texel;
            float3 points[4];
            for(int j = 0; j < 4; ++j)
            points[j] = area_lights[i].corners[j].xyz;
            float3 v = -normalize(input.world_pos.xyz - camera_view_pos.xyz);
            float3 pos = input.world_pos.xyz;
            float4 diff_uv = area_light_diffuse_uv(points, pos, n, v);
            float2 duv = clamp(diff_uv.xy, inv_texel, inv_texel_x);
            float3 diff = sample_texture_array_level( area_light_textures, duv, slice, diff_uv.z * levels).rgb * diff_uv.w;
            float4 spec_uv = area_light_specular_uv(ltc_mat, sampler_ltc_mat, ltc_mag, sampler_ltc_mag, points, pos, ro_sample.x, n, v);
            float2 suv = clamp(spec_uv.xy, inv_texel, inv_texel_x);
            float3 spec = sample_texture_array_level(area_light_textures, suv, slice, spec_uv.z * levels).rgb * spec_uv.w;
            float3 light_col = (spec.rgb + diff.rgb) / (2.0 * pi);
            lit_colour += light_col;
        }
    }
    output.colour.rgb = lit_colour.rgb * albedo.a;
    output.colour.a = albedo.a;
    if(albedo.a <= 0.0)
    discard;
    return output;
}
