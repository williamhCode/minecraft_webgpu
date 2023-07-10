struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
	@location(3) texLoc: vec2f
};

struct VertexOutput {
	@builtin(position) position: vec4f,
  @location(0) fragPos: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
  @location(3) texLoc: vec2f,
};

struct GBufferOutput {
  @location(0) position : vec4f,
  @location(1) normal : vec4f,
  @location(2) albedo : vec4f,
}

@group(0) @binding(0) var<uniform> view: mat4x4f;
@group(0) @binding(1) var<uniform> projection: mat4x4f;

@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;

@group(2) @binding(0) var<uniform> offset: vec3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let viewPos = view * vec4f(in.position + offset, 1.0);

  var out: VertexOutput;
  out.position = projection * viewPos;
  out.fragPos = viewPos.xyz;
  out.normal = in.normal;
  out.uv = in.uv;
  out.texLoc = in.texLoc;

	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> GBufferOutput {
  let uv = (in.uv + in.texLoc) / 16.0;

  var out: GBufferOutput;
  out.position = vec4f(in.fragPos, 1.0);
  out.normal = vec4f(normalize(in.normal), 1.0);
  out.albedo = vec4f(textureSample(texture, textureSampler, uv).rgb, 1.0);

  return out;
}

