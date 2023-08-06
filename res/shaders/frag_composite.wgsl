// @group(0) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(0) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(0) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(0) @binding(3) var gBufferSampler: sampler;

@group(1) @binding(0) var ssaoTexture: texture_2d<f32>;
@group(1) @binding(1) var ssaoSampler: sampler;

@group(2) @binding(0) var waterTexture: texture_2d<f32>;

fn blend(src: vec4f, dst: vec4f) -> vec4f {
  let srcAlpha = src.a;
  let dstAlpha = dst.a;

  let outAlpha = dstAlpha;
  let outColor = src.rgb * srcAlpha + dst.rgb * (1.0 - srcAlpha);

  return vec4f(outColor, outAlpha);
}

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  let albedo = textureSampleLevel(gBufferAlbedo, gBufferSampler, uv, 0.0).rgb;
  let ambientOcclusion = textureSampleLevel(ssaoTexture, ssaoSampler, uv, 0.0).r;

  var waterColor = textureSampleLevel(waterTexture, gBufferSampler, uv, 0.0);

  // let ambient = vec3f(ambientOcclusion, ambientOcclusion, ambientOcclusion);
  let ambient = vec3f(albedo * ambientOcclusion);

  if (all(normal == vec3f(0.0, 0.0, 0.0))) {
    return vec4f(ambient, 1.0);
  }

  var color = ambient * max(0.6, dot(normal, normalize(vec3f(1.0, 2.0, 3.0))));
  // var color = ambient;
  var finalColor = vec4f(color, 1.0);
  finalColor = blend(waterColor, finalColor);
  // finalColor = waterColor;

  return finalColor;
}
