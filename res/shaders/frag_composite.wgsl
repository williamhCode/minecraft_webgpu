// @group(0) @binding(0) var<uniform> view: mat4x4f;
// @group(0) @binding(1) var<uniform> projection: mat4x4f;
@group(0) @binding(2) var<uniform> inverseView: mat4x4f;

@group(1) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(1) @binding(1) var gBufferNormal: texture_2d<f32>;
@group(1) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(1) @binding(3) var gBufferSampler: sampler;

@group(2) @binding(0) var<uniform> sunDir: vec3f;
@group(2) @binding(1) var<uniform> sunViewProj: mat4x4f;

@group(3) @binding(0) var ssaoTexture: texture_2d<f32>;
@group(3) @binding(1) var waterTexture: texture_2d<f32>;
@group(3) @binding(2) var shadowMap: texture_depth_2d;
@group(3) @binding(3) var shadowSampler: sampler_comparison;


fn Blend(src: vec4f, dst: vec4f) -> vec4f {
  let outAlpha = dst.a;
  let outColor = src.rgb * src.a + dst.rgb * (1.0 - src.a);

  return vec4f(outColor, outAlpha);
}

fn ShadowCalculation(lightSpacePos: vec4f, normal: vec3f) -> f32 {
  // map to texture coordinates
  var projCoords = vec3f(
    lightSpacePos.xy * vec2f(0.5, -0.5) + vec2f(0.5),
    lightSpacePos.z
  );

  if (   lightSpacePos.x > 1.0 || lightSpacePos.x < 0.0
      || lightSpacePos.y > 1.0 || lightSpacePos.y < 0.0
      || lightSpacePos.z > 1.0 || lightSpacePos.z < 0.0) {
    return 1.0;
  }

  // let bias = max(0.001 * (1.0 - dot(normal, sunDir)), 0.0001);
  let bias = 0.0005;
  let depth = textureSampleCompareLevel(shadowMap, shadowSampler, projCoords.xy, projCoords.z - bias);
  return depth;
}

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  let samplePos = textureSampleLevel(gBufferPosition, gBufferSampler, uv, 0.0);
  let position = (inverseView * samplePos).xyz;
  let sunSpacePos = sunViewProj * vec4f(position, 1.0);

  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  var albedo = textureSampleLevel(gBufferAlbedo, gBufferSampler, uv, 0.0).rgb;
  let ambientOcclusion = textureSampleLevel(ssaoTexture, gBufferSampler, uv, 0.0).r;
  albedo *= ambientOcclusion;

  var waterColor = textureSampleLevel(waterTexture, gBufferSampler, uv, 0.0);
    
  // lighting calculations
  let ambient = 0.4;
  var diffuse = 0.6;
  if (samplePos.w != 0.0) {
    diffuse *= max(0.0, dot(normal.xyz, normalize(sunDir)));
  }

  // let viewPos = inverseView[3].xyz;
  // let viewDir = normalize(viewPos - position);
  // let reflectDir = reflect(-sunDir, normal);
  // let specular = 0.25 * pow(max(dot(viewDir, reflectDir), 0.0), 100.0);
  let specular = 0.0;

  var shadow = 1.0;
  if (samplePos.w != 0.0) {
    shadow = ShadowCalculation(sunSpacePos, normal);
  }

  // if (shadow == 0.0) {
  //   return vec4f(ambientOcclusion, ambientOcclusion, ambientOcclusion, 1.0);
  // }

  let color = albedo * (ambient + diffuse * shadow);
  var finalColor = vec4f(color, 1.0);

  finalColor = Blend(waterColor, finalColor);
  // finalColor = vec4f(ambientOcclusion, ambientOcclusion, ambientOcclusion, 1.0);

  return finalColor;
}
