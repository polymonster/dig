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
struct vs_output
{
    float4 position;
    float4 screen_coord;
    float4 light_pos_radius;
    float4 light_dir_cutoff;
    float4 light_colour;
    float4 light_data;
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
fragment ps_output ps_main(vs_output input [[stage_in]]
, texture_2d( gbuffer_albedo, 0 )
, texture_2d( gbuffer_normals, 1 )
, texture_2d( gbuffer_world_pos, 2 )
, constant c_per_pass_view &per_pass_view [[buffer(4)]])
{
    constant float4& camera_view_pos = per_pass_view.camera_view_pos;
    ps_output output;
    float2 sc = input.screen_coord.xy;
    float3 final_light_col = float3(0.0, 0.0, 0.0);
    int samples = 1;
    _pmfx_unroll
    for(int i = 0; i < samples; ++i)
    {
        float4 g_albedo = sample_texture(gbuffer_albedo, sc);
        float4 g_normals = sample_texture(gbuffer_normals, sc);
        float4 g_world_pos = sample_texture(gbuffer_world_pos, sc);
        float3 albedo = g_albedo.rgb;
        float3 n = normalize(g_normals.rgb);
        float3 world_pos = g_world_pos.rgb;
        float metalness = g_world_pos.a;
        float roughness = g_albedo.a;
        float reflectivity = g_normals.a;
        float3 light_col = cook_torrence(
        input.light_pos_radius,
        input.light_colour.rgb,
        n,
        world_pos,
        camera_view_pos.xyz,
        albedo,
        float3(0.5, 0.5, 0.5),
        roughness,
        reflectivity);
        light_col += oren_nayar(
        input.light_pos_radius,
        input.light_colour.rgb,
        n,
        world_pos,
        camera_view_pos.xyz,
        roughness,
        albedo.rgb
        );
        final_light_col += light_col;
    }
    output.colour.rgb = final_light_col / float(samples);
    return output;
}
