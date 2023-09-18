struct VertexInput {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) extraData: u32,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) fragPos: vec3f,
  @location(1) normal: vec3f,
  @location(2) uv: vec2f,
  @location(3) @interpolate(flat) extraData: u32,
};

@group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;
@group(0) @binding(2) var<uniform> inverseView: mat4x4f;

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

@group(2) @binding(0) var<uniform> sunDir: vec3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  // var position = in.position;
  // let scale = 0.3;
  // position += vec3f(0.0, 0.0, (sin(position.x * scale) + sin(position.y * scale)) * 1.0);

  var out: VertexOutput;
  out.position = projection * view * vec4f(in.position, 1.0);;
  out.fragPos = in.position;
  out.normal = in.normal;
  out.uv = in.uv;
  out.extraData = in.extraData;

  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  // get last byte (2x 4 bits), put in vec2f
  let texLoc = vec2f(f32(in.extraData & 0x0Fu), f32((in.extraData >> 4u) & 0x0Fu));
  let uv = (in.uv + texLoc) / 16.0;

  let color = textureSample(texture, textureSampler, uv);

  // let diffuse = max(dot(in.normal, sunDir), 0.0);
  let diffuse = 1.0;

  let viewPos = inverseView[3].xyz;
  let viewDir = normalize(viewPos - in.fragPos);
  let reflectDir = reflect(-sunDir, in.normal);
  let specular = 0.4 * pow(max(dot(viewDir, reflectDir), 0.0), 100.0);

  var out = color * diffuse + vec4f(1.0, 1.0, 1.0, 0.4) * specular;
  // let out = specular * vec4f(1.0, 0.0, 0.0, 1.0);

  return out;
}


