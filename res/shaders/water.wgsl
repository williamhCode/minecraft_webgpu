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

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let viewPos = view * vec4f(in.position, 1.0);

  var out: VertexOutput;
  out.position = projection * viewPos;
  out.fragPos = viewPos.xyz;
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

  let out = textureSample(texture, textureSampler, uv);

  return out;
}


