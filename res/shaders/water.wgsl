struct VertexInput {
  // 0  position (5 bits, 5 bits, 10 bits)
  // 20 uv (1 bit x 2)
  // 22 texLoc (4 bits x 2)
  // 30 transparency (2 bits)
  @location(0) data1: u32,
  // 0 normal (2 bit x 3)
  @location(1) data2: u32,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) fragPos: vec3f,
  @location(1) uv: vec2f,
  @location(2) @interpolate(flat) data1: u32,
  @location(3) @interpolate(flat) data2: u32,
}

@group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;
@group(0) @binding(2) var<uniform> inverseView: mat4x4f;

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

@group(2) @binding(0) var<uniform> sunDir: vec3f;
// @group(2) @binding(1) var<uniform> sunViewProj: mat4x4f;

@group(3) @binding(0) var<uniform> worldOffset: vec3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let position = vec3f(f32(in.data1 & 0x1Fu), f32((in.data1 >> 5u) & 0x1Fu), f32((in.data1 >> 10u) & 0x3FFu));
  let worldPos = vec4f(position + worldOffset, 1.0);

  var out: VertexOutput;
  out.position = projection * view * worldPos;
  out.fragPos = worldPos.xyz;
  out.uv = vec2f(f32((in.data1 >> 20u) & 0x01u), f32((in.data1 >> 21u) & 0x01u));
  out.data1 = in.data1;
  out.data2 = in.data2;

  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let texLoc = vec2f(f32((in.data1 >> 22u) & 0x0Fu), f32((in.data1 >> 26u) & 0x0Fu));
  let uv = (in.uv + texLoc) / 16.0;
  var normal = vec3f(f32(in.data2 & 0x03u), f32((in.data2 >> 2u) & 0x03u), f32((in.data2 >> 4u) & 0x03u));
  normal -= 1.0;

  let color = textureSample(texture, textureSampler, uv);

  // let diffuse = max(dot(in.normal, sunDir), 0.0);
  let diffuse = 1.0;

  let viewPos = inverseView[3].xyz;
  let viewDir = normalize(viewPos - in.fragPos);
  let reflectDir = reflect(-sunDir, normal);
  let specular = 0.4 * pow(max(dot(viewDir, reflectDir), 0.0), 100.0);

  var out = color * diffuse + vec4f(1.0, 1.0, 1.0, 0.4) * specular;
  // let out = specular * vec4f(1.0, 0.0, 0.0, 1.0);

  return out;
}


