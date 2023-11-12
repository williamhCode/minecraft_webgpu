@group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;
@group(0) @binding(2) var<uniform> inverseView: mat4x4f;

@group(1) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(1) @binding(1) var gBufferNormal: texture_2d<f32>;
// @group(1) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(1) @binding(3) var gBufferSampler: sampler;

@group(2) @binding(0) var<uniform> samples: array<vec4f, 64>;
@group(2) @binding(1) var noiseTexture: texture_2d<f32>;
@group(2) @binding(2) var noiseSampler: sampler;
@group(2) @binding(3) var<uniform> opts: Options;
struct Options {
  enabled: i32,
  sampleSize: i32,
  radius: f32,
  bias: f32,
}

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) f32 {
  if (opts.enabled == 0) {
    return 1.0;
  }

  // get input for SSAO algorithm
  let fragViewPos = textureSampleLevel(gBufferPosition, gBufferSampler, uv, 0.0);
  if (fragViewPos.w == 0.0) {
    return 1.0;
  }
  let fragWorldPos = (inverseView * fragViewPos).xyz;
  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  let noiseScale = vec2f(textureDimensions(gBufferPosition)) / 4.0;
  let randomVec = textureSampleLevel(noiseTexture, noiseSampler, uv * noiseScale, 0.0).xyz;

  // create TBN change-of-basis matrix: from tangent-space to view-space
  let tangent = normalize(randomVec - dot(randomVec, normal) * normal);
  let bitangent = cross(normal, tangent);
  let TBN = mat3x3f(tangent, bitangent, normal);
  // iterate over the sample kernel and calculate occlusion factor
  var occlusion = 0.0;
  for (var i = 0; i < opts.sampleSize; i++) {
    // get sample position
    var samplePos = TBN * samples[i].xyz;
    samplePos = fragWorldPos + samplePos * opts.radius; 
    samplePos = (view * vec4f(samplePos, 1.0)).xyz;

    // project sample position (to sample texture) (to get position on screen/texture)
    var clipOffset = projection * vec4f(samplePos, 1.0);
    clipOffset.y = -clipOffset.y;
    let screenOffset = (clipOffset.xy / clipOffset.w) * 0.5 + 0.5;

    if (textureSampleLevel(gBufferNormal, gBufferSampler, screenOffset, 0.0).w == 1.0) { continue; }
    let sampleDepth = textureSampleLevel(gBufferPosition, gBufferSampler, screenOffset, 0.0).z;
     // range check & accumulate
    let rangeCheck = smoothstep(0.0, 1.0, opts.radius / abs(fragViewPos.z - sampleDepth));
    occlusion += select(0.0, 1.0, sampleDepth >= samplePos.z + opts.bias) * rangeCheck;
  }
  occlusion = 1.0 - (occlusion / f32(opts.sampleSize));

  return occlusion;
}
