struct VertexInput {
  // 0  position (5 bits, 5 bits, 10 bits)
  // 20 uv (1 bit x 2)
  // 22 texLoc (4 bits x 2)
  // 30 transparency (2 bits)
  @location(0) data1: u32,
  // 0 normal (2 bit x 3)
  // 6 color (8 bit x 3)
  @location(1) data2: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
}

@group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

@group(2) @binding(0) var<uniform> worldOffset: vec3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let position = vec3f(f32(in.data1 & 0x1Fu), f32((in.data1 >> 5u) & 0x1Fu), f32((in.data1 >> 10u) & 0x3FFu));
  let viewPos = view * vec4f(position + worldOffset, 1.0);

  var out: VertexOutput;
  out.position = projection * viewPos;
  out.position.z += 0.0001;

  return out;
}
