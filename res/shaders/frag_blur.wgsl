@group(0) @binding(0) var ssaoTexture: texture_2d<f32>;
@group(0) @binding(1) var ssaoSampler: sampler;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) f32 {
  let texelSize = 1.0 / vec2f(textureDimensions(ssaoTexture));
  var result = 0.0;
  for (var x = -2; x < 2; x++) 
  {
      for (var y = -2; y < 2; y++) 
      {
          let offset = vec2f(f32(x), f32(y)) * texelSize;
          result += textureSampleLevel(
             ssaoTexture, ssaoSampler, uv + offset, 0.0
          ).r;
      }
  }

  // let color = result / (4.0 * 4.0);
  // return vec4f(color, color, color, 1.0);
  return result / (4.0 * 4.0);
}
