#version 150 core
#define PI 3.1415926538

in vec4 position;
out vec3 f_color;

uniform mat4 view;
uniform mat4 model;

uniform vec2 barycenter;
uniform float time;
uniform int animated;
uniform int is_ith_triangle;

vec3 compute_color(int is_ith_triangle, float s, vec3 origin_color)
{
	vec3 comp_color = vec3(0.0,0.0,0.0);
	if (is_ith_triangle == 1) {
		comp_color = origin_color * (1 - 0.4 * s);
	} else {comp_color = origin_color;}
	return comp_color;
}

void main()
{
	float theta = PI * time;
	float c = cos(0.5 * theta);
	float s = sin(0.5 * theta);

	vec3 color = vec3(0.0,0.0,0.0);
	if (position[2] == -1.0) {
		color = compute_color(is_ith_triangle, s, vec3(0.75,0.2,0.18));
	}
	if (position[2] == 0.0) {
		color = vec3(0,0,0);
	}
	if (position[2] == 1.0) {
		color = compute_color(is_ith_triangle, s, vec3(240,128,128)/255.0);
	}
	if (position[2] == 2.0) {
		color = compute_color(is_ith_triangle, s, vec3(255,165,0)/255.0);
	}
	if (position[2] == 3.0) {
		color = compute_color(is_ith_triangle, s, vec3(240,230,140)/255.0);
	}
	if (position[2] == 4.0) {
		color = compute_color(is_ith_triangle, s, vec3(144,238,144)/255.0);
	}
	if (position[2] == 5.0) {
		color = compute_color(is_ith_triangle, s, vec3(102,205,170)/255.0);
	}
	if (position[2] == 6.0) {
		color = compute_color(is_ith_triangle, s, vec3(32,178,170)/255.0);
	}
	if (position[2] == 7.0) {
		color = compute_color(is_ith_triangle, s, vec3(65,105,225)/255.0);
	}
	if (position[2] == 8.0) {
		color = compute_color(is_ith_triangle, s, vec3(123,104,238)/255.0);
	}
	if (position[2] == 9.0) {
		color = compute_color(is_ith_triangle, s, vec3(255,182,193)/255.0);
	}

	if ((position[3] == 0) || (animated == 0)) {
		gl_Position = view * model * vec4(position[0], position[1], 0.0, 1.0);
		f_color = color;
	}
	else {
		vec4 b0 = model * vec4(barycenter, 0.0, 1.0);
		vec2 b1 = barycenter;
		mat4 r_m;

		// in glsl the matrix is in the transposed form
		// put-back matrixs
		mat4 m0 = mat4(
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			-b0[0],-b0[1],0,1);
		mat4 m1 = mat4(
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			b0[0],b0[1],0,1);
		mat4 m2 = mat4(
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			-b1[0],-b1[1],0,1);
		mat4 m3 = mat4(
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			b1[0],b1[1],0,1);

		mat4 put;
		mat4 back;

		// animation matrix
		if (position[3] == 1) {
			r_m = mat4(
				c,-s,0,0,
				s,c,0,0,
				0,0,1,0,
				0,0,0,1);
		}
		else if (position[3] == 2) {
			r_m = mat4(
				c,0,-s,0,
				0,1,0,0,
				s,0,c,0,
				0,0,0,1);
		}
		else if (position[3] == 3) {
			r_m = mat4(
				1,0,0,0,
				0,c,-s,0,
				0,s,c,0,
				0,0,0,1);
		}
		else if (position[3] == 4) {
			r_m = mat4(
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				s,0,0,1);
		}
		else if (position[3] == 5) {
			r_m = mat4(
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,s,0,1);
		}
		else if (position[3] == 6) {
			r_m = mat4(
				1+s/2,0,0,0,
				0,1+s/2,0,0,
				0,0,1,0,
				0,0,0,1);
		}
		else if (position[3] == 7) {
			r_m = mat4(
				c,-s,0,0,
				s,c,0,0,
				0,0,1,0,
				0,0,0,1);
		}
		if (position[3] <= 6) {
			put = m0;
			back = m1;
		}
		else if (position[3] == 7) {
			put = m2;
			back = m3;
		}
		gl_Position = view * back * r_m * put * model * vec4(position[0], position[1], 0.0, 1.0);
		f_color = color;
	}

}