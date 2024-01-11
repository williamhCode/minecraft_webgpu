struct VertexInput {
  // 0  position (5 bits, 5 bits, 10 bits)
  // 20 uv (1 bit x 2)
  // 22 texLoc (4 bits x 2)
  // 30 transparency (2 bits)
  @location(0) data1: u32,
  // 0 normal (2 bit x 3)
  @location(1) data2: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) uv: vec2f,
  @location(1) @interpolate(flat) data1: u32,
}

@group(0) @binding(0) var<uniform> index: u32;

// @group(1) @binding(0) var<uniform> sunDir: vec3f;
@group(1) @binding(1) var<storage> sunViewProjs: array<mat4x4f>;
// @group(1) @binding(2) var<uniform> numCascades: i32;

@group(2) @binding(0) var texture: texture_2d<f32>;
@group(2) @binding(1) var textureSampler: sampler;

@group(3) @binding(0) var<uniform> worldOffset: vec3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let position = vec3f(f32(in.data1 & 0x1Fu), f32((in.data1 >> 5u) & 0x1Fu), f32((in.data1 >> 10u) & 0x3FFu));

  var out: VertexOutput;
  out.position = sunViewProjs[index] * vec4f(position + worldOffset, 1.0f);
  out.uv = vec2f(f32((in.data1 >> 20u) & 0x01u), f32((in.data1 >> 21u) & 0x01u));
  out.data1 = in.data1;

  return out;
}

@fragment
fn fs_main(in: VertexOutput) {
  let texLoc = vec2f(f32((in.data1 >> 22u) & 0x0Fu), f32((in.data1 >> 26u) & 0x0Fu));
  let uv = (in.uv + texLoc) / 16.0;
  let transparency = (in.data1 >> 30u) & 0x03u;

  var color: vec4f;
  if (transparency >= 2u) {
    color = textureSampleLevel(texture, textureSampler, uv, 0.0);
    if (color.a < 0.01) {
        discard;
    }
  }  
}
