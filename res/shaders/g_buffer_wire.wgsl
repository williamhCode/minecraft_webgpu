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
  @location(0) fragPos: vec3f,
  @location(1) uv: vec2f,
  @location(2) @interpolate(flat) data1: u32,
  @location(3) @interpolate(flat) data2: u32,
}

struct GBufferOutput {
  @location(0) position : vec4f,
  @location(1) normal : vec4f,
  @location(2) albedo : vec4f,
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
  out.fragPos = viewPos.xyz;
  out.uv = vec2f(f32((in.data1 >> 20u) & 0x01u), f32((in.data1 >> 21u) & 0x01u));
  out.data1 = in.data1;
  out.data2 = in.data2;

  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> GBufferOutput {
  let texLoc = vec2f(f32((in.data1 >> 22u) & 0x0Fu), f32((in.data1 >> 26u) & 0x0Fu));
  let uv = (in.uv + texLoc) / 16.0;
  let transparency = (in.data1 >> 30u) & 0x03u;
  var normal = vec3f(f32(in.data2 & 0x03u), f32((in.data2 >> 2u) & 0x03u), f32((in.data2 >> 4u) & 0x03u));
  normal -= 1.0;
  let lightColor = vec3f(f32((in.data2 >> 6u) & 0xFFu), f32((in.data2 >> 14u) & 0xFFu), f32((in.data2 >> 22u) & 0xFFu)) / 255.0;

  var out: GBufferOutput;
  var color: vec4f;

  if (texLoc.x == 1 && texLoc.y == 4) {
    color = vec4f(lightColor, 1.0);
  } else {
    // dont use mipmap if object is transparent 
    // this is to prevent messing up the alpha, since only translucent, not transparent objects are sorted
    if (transparency >= 2u) {
      color = textureSampleLevel(texture, textureSampler, uv, 0.0);
    } else @diagnostic(off,derivative_uniformity) {
      color = textureSample(texture, textureSampler, uv);
    }
  }

  out.albedo = color;

  // dont store position of fully transparent object
  if (transparency == 3u) {
    // set w to 0.5 for ssao uses
    out.position = vec4f(in.fragPos, 0.5);
    out.normal = vec4f(normal, 1.0);
    return out;
  }
  
  out.position = vec4f(in.fragPos, 1.0);
  out.normal = vec4f(normal, 1.0);

  return out;
}
