struct VertexInput {
	@location(0) position: vec2f,
	@location(1) uv: vec2f,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
	@location(0) uv: vec2f,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  out.position = vec4f(in.position, 0.0, 1.0);
  out.uv = in.uv;

  return out;
}
