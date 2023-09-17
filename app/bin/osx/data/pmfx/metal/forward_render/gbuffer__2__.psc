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
struct c_material_data
{
    float4 m_albedo;
    float2 m_uv_scale;
    float m_roughness;
    float m_reflectivity;
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
struct ps_output_multi
{
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 world_pos [[color(2)]];
};
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
fragment ps_output_multi ps_main(vs_output input [[stage_in]]
, texture_2d( diffuse_texture, 0 )
, texture_2d( normal_texture, 1 )
, texture_2d( specular_texture, 2 )
, constant c_material_data &material_data [[buffer(11)]])
{
    constant float& m_reflectivity = material_data.m_reflectivity;
    ps_output_multi output;
    float4 albedo = sample_texture(diffuse_texture, input.texcoord.xy);
    float4 metalness = sample_texture(specular_texture, input.texcoord.xy);
    float3 normal_sample = sample_texture( normal_texture, input.texcoord.xy ).rgb;
    normal_sample = normal_sample * 2.0 - 1.0;
    float4 ro_sample = sample_texture( specular_texture, input.texcoord.xy );
    float3 n = transform_ts_normal(
    input.tangent,
    input.bitangent,
    input.normal,
    normal_sample );
    float roughness = ro_sample.x;
    float reflectivity = m_reflectivity;
    output.albedo = float4(albedo.rgb * input.colour.rgb, roughness);
    output.normal = float4(n, reflectivity);
    output.world_pos = float4(input.world_pos.xyz, metalness.r);
    return output;
}
