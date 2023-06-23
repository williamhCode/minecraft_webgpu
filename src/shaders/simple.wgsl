struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) normal: vec3f,
	@location(1) uv: vec2f,
	@location(2) org_position: vec3f,
};

@group(0) @binding(0) var<uniform> viewProjMatrix: mat4x4f;

@group(1) @binding(0) var<uniform> modelMatrix: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
  out.org_position = in.position;
	out.position = viewProjMatrix * modelMatrix * vec4f(in.position, 1.0);
  out.normal = in.normal;
  out.uv = in.uv;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  // let texelCoords = vec2i(in.uv * vec2f(textureDimensions(gradientTexture)));
  // let color = textureLoad(gradientTexture, texelCoords, 0).rgb;
  let color = vec3f(in.uv, 0.5);
	return vec4f(color, 1.0);
}
