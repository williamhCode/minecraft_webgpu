struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
};

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    time: f32,
};

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

const pi = 3.14159266;


/**
 * Option A: Rebuild the matrices for each vertex
 * (not recommended)
 */
fn vs_main_optionA(in: VertexInput) -> VertexOutput {
	// Scale the object
	let S = transpose(mat4x4f(
		0.3,  0.0, 0.0, 0.0,
		0.0,  0.3, 0.0, 0.0,
		0.0,  0.0, 0.3, 0.0,
		0.0,  0.0, 0.0, 1.0,
	));

	// Translate the object
	let T1 = transpose(mat4x4f(
		1.0,  0.0, 0.0, 0.5,
		0.0,  1.0, 0.0,  0.0,
		0.0,  0.0, 1.0,  0.0,
		0.0,  0.0, 0.0,  1.0,
	));

	// Rotate the object
	let angle1 = uMyUniforms.time;
	// let angle1 = 0.0;
	let c1 = cos(angle1);
	let s1 = sin(angle1);
	let R1 = transpose(mat4x4f(
		 c1,  s1, 0.0, 0.0,
		-s1,  c1, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	));

	// Change the view angle
	let angle2 = 3.0 * pi / 4.0;
	let c2 = cos(angle2);
	let s2 = sin(angle2);
	let R2 = transpose(mat4x4f(
		1.0, 0.0, 0.0, 0.0,
		0.0,  c2,  s2, 0.0,
		0.0, -s2,  c2, 0.0,
		0.0, 0.0, 0.0, 1.0,
	));

	// Move the view point
	let focalPoint = vec3f(0.0, 0.0, -2.0);
	let T2 = transpose(mat4x4f(
		1.0,  0.0, 0.0, -focalPoint.x,
		0.0,  1.0, 0.0, -focalPoint.y,
		0.0,  0.0, 1.0, -focalPoint.z,
		0.0,  0.0, 0.0,     1.0,
	));

	let focalLength = 2.0;
	let near = 0.01;
	let far = 100.0;
	let ratio = 640.0 / 480.0;
	// (no need for a scale parameter now that we have focalLength)
	let divider = 1.0 / (focalLength * (far - near));
	let P = transpose(mat4x4f(
		1.0,  0.0,        0.0,                  0.0,
		0.0, ratio,       0.0,                  0.0,
		0.0,  0.0,    far * divider,   -far * near * divider,
		0.0,  0.0,  1.0 / focalLength,          0.0,
	));

	var out: VertexOutput;
	var homogeneous_position = vec4f(in.position, 1.0);
	out.position = P * T2 * R2 * R1 * T1 * S * homogeneous_position;
	out.color = in.color;
	return out;
}

/**
 * Option B: Use matrices that have been precomputed and stored in the uniform buffer
 * (recommended)
 */
fn vs_main_optionB(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
	out.color = in.color;
	return out;
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	// return vs_main_optionA(in);
	return vs_main_optionB(in);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let color = in.color * uMyUniforms.color.rgb;
	// Gamma-correction
	let corrected_color = pow(color, vec3f(2.2));
	return vec4f(corrected_color, uMyUniforms.color.a);
}
