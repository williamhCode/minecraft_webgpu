// @group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;

@group(1) @binding(0) var gBufferPosition: texture_2d<f32>;
@group(1) @binding(1) var gBufferNormal: texture_2d<f32>;
// @group(1) @binding(2) var gBufferAlbedo: texture_2d<f32>;
@group(1) @binding(3) var gBufferSampler: sampler;

@group(2) @binding(0) var<uniform> samples: array<vec4f, 45>;
@group(2) @binding(1) var noiseTexture: texture_2d<f32>;
@group(2) @binding(2) var noiseSampler: sampler;

const kernelSize = 45;
const radius = 0.25f;
const bias = 0.01f;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4f {
  // get input for SSAO algorithm
  let fragPos = textureSampleLevel(gBufferPosition, gBufferSampler, uv, 0.0).xyz;
  if (fragPos.x == 0.0) {
    return vec4f(1.0, 1.0, 1.0, 1.0);
  }
  let normal = textureSampleLevel(gBufferNormal, gBufferSampler, uv, 0.0).xyz;
  let noiseScale = vec2f(textureDimensions(gBufferPosition)) / 4.0;
  let randomVec = textureSampleLevel(noiseTexture, noiseSampler, uv * noiseScale, 0.0).xyz;

  // // create TBN change-of-basis matrix: from tangent-space to view-space
  let tangent = normalize(randomVec - normal * dot(randomVec, normal));
  let bitangent = cross(normal, tangent);
  let TBN = mat3x3f(tangent, bitangent, normal);
  // // iterate over the sample kernel and calculate occlusion factor
  var occlusion = 0.0;
  for (var i = 0; i < kernelSize; i++) {
    // get sample position
    var samplePos = TBN * samples[i].xyz; // from tangent to view-space
    samplePos = fragPos + samplePos * radius; 

    // project sample position (to sample texture) (to get position on screen/texture)
    var clipOffset = projection * vec4f(samplePos, 1.0);
    clipOffset.y = -clipOffset.y;
    let screenOffset = (clipOffset.xy / clipOffset.w) * 0.5 + 0.5;

    let sampleDepth = textureSampleLevel(gBufferPosition, gBufferSampler, screenOffset, 0.0).z;
     // range check & accumulate
    let rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
    occlusion += select(0.0, 1.0, sampleDepth >= samplePos.z + bias) * rangeCheck;
  }
  occlusion = 1.0 - (occlusion / f32(kernelSize));

  return vec4f(occlusion, occlusion, occlusion, 1.0);
}
