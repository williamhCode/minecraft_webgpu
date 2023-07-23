// @group(0) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(0) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(0) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(0) @binding(3) var gBufferSampler: sampler;

@group(1) @binding(0) var ssaoTexture: texture_2d<f32>;
@group(1) @binding(1) var ssaoSampler: sampler;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  let color = textureSampleLevel(gBufferAlbedo, gBufferSampler, uv, 0.0).xyz;
  let ambientOcclusion = textureSampleLevel(ssaoTexture, ssaoSampler, uv, 0.0).r;

  // let ambient = vec3f(ambientOcclusion, ambientOcclusion, ambientOcclusion);
  let ambient = vec3f(color * ambientOcclusion);

  if (all(normal == vec3f(0.0, 0.0, 0.0))) {
    return vec4f(ambient, 1.0);
  }

  var finalColor = ambient * max(0.6, dot(normal, normalize(vec3f(1.0, 2.0, 3.0))));
  // var finalColor = ambient;

  return vec4f(finalColor, 1.0);
}
