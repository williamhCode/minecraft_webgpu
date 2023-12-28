struct VertexInput {
  // 0  position (5 bits, 5 bits, 10 bits)
  // 20 uv (1 bit x 2)
  // 22 texLoc (4 bits x 2)
  // 30 transparency (2 bits)
  @location(0) data1: u32,
  // 0 normal (2 bit x 3)
  @location(1) data2: u32,
}

@group(0) @binding(0) var<uniform> index: u32;

// @group(1) @binding(0) var<uniform> sunDir: vec3f;
@group(1) @binding(1) var<storage> sunViewProjs: array<mat4x4f>;
// @group(1) @binding(2) var<uniform> numCascades: u32;

@group(2) @binding(0) var<uniform> worldOffset: vec3f;


@vertex
fn vs_main(in: VertexInput) -> @builtin(position) vec4f {
  let position = vec3f(f32(in.data1 & 0x1Fu), f32((in.data1 >> 5u) & 0x1Fu), f32((in.data1 >> 10u) & 0x3FFu));
  return sunViewProjs[index] * vec4f(position + worldOffset, 1.0f);
}

