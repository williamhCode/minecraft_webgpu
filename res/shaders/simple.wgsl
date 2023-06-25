struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
	@location(3) texLoc: vec2f
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) normal: vec3f,
	@location(1) uv: vec2f,
	@location(2) texLoc: vec2f,
};

@group(0) @binding(0) var<uniform> viewProjMatrix: mat4x4f;
@group(1) @binding(0) var texture: texture_2d<f32>;
@group(1) @binding(1) var textureSampler: sampler;
@group(2) @binding(0) var<uniform> modelMatrix: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = viewProjMatrix * modelMatrix * vec4f(in.position, 1.0);
  out.normal = in.normal;
  out.uv = in.uv;
  out.texLoc = in.texLoc;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let uv = (in.uv + in.texLoc) / 16.0;
  let color = textureSample(texture, textureSampler, uv).rgb;
	return vec4f(color, 1.0);
}
