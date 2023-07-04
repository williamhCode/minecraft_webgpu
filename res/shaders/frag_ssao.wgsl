@group(0) @binding(1) var<uniform> projection: mat4x4f;

@group(1) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(1) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(1) @binding(2) var gBufferAlbedo: texture_2d<f32>;
// @group(1) @binding(3) var gBufferSampler: sampler;

@group(2) @binding(0) var<uniform> samples: array<vec3f, 64>;

@fragment
fn main(@location(0) uv: vec2f) -> @location(0) vec4f {

  return vec4f();
}
