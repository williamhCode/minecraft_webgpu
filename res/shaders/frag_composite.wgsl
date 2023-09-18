@group(0) @binding(0) var<uniform> view: mat4x4f;
// @group(0) @binding(1) var<uniform> projection: mat4x4f;
@group(0) @binding(2) var<uniform> inverseView: mat4x4f;

@group(1) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(1) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(1) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(1) @binding(3) var gBufferSampler: sampler;

@group(2) @binding(0) var ssaoTexture: texture_2d<f32>;
@group(2) @binding(1) var waterTexture: texture_2d<f32>;

@group(3) @binding(0) var<uniform> sunDir: vec3f;

fn blend(src: vec4f, dst: vec4f) -> vec4f {
  let outAlpha = dst.a;
  let outColor = src.rgb * src.a + dst.rgb * (1.0 - src.a);

  return vec4f(outColor, outAlpha);
}

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  let position = (inverseView * textureSampleLevel(gBufferPosition, gBufferSampler, uv, 0.0)).xyz;

  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  var albedo = textureSampleLevel(gBufferAlbedo, gBufferSampler, uv, 0.0).rgb;
  let ambientOcclusion = textureSampleLevel(ssaoTexture, gBufferSampler, uv, 0.0).r;

  var waterColor = textureSampleLevel(waterTexture, gBufferSampler, uv, 0.0);

  albedo = vec3f(albedo * ambientOcclusion);

  let ambient = 0.7;

  let diffuse = 0.3 * dot(normal.xyz, normalize(sunDir));

  // let viewPos = inverseView[3].xyz;
  // let viewDir = normalize(viewPos - position);
  // let reflectDir = reflect(-sunDir, normal);
  // let specular = 0.25 * pow(max(dot(viewDir, reflectDir), 0.0), 100.0);
  let specular = 0.0;

  var color = albedo * (ambient + diffuse) + vec3f(1.0) * specular;

  var finalColor = vec4f(color, 1.0);
  finalColor = blend(waterColor, finalColor);

  return finalColor;
}
