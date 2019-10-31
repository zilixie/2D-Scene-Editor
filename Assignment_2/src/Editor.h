#ifndef EDITOR_H
#define EDITOR_H

#include "Helpers.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // GLFW is necessary to handle the OpenGL context
#else
#include <GLFW/glfw3.h> // GLFW is necessary to handle the OpenGL context
#endif

#include <Eigen/Core>
#include <Eigen/Dense>
#include <chrono>
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <initializer_list>

#define INSERT_MODE 1
#define TRANSLATION_MODE 2
#define DELETE_MODE 3
#define COLORIZE_MODE 4
#define ANIMATION_MODE 5
#define BEZIER_CURVE_MODE 6
#define QUIT_MODE 7

class Editor {
	public:
		int mode;
		int triangle_count;    // Numbers of triangle that have been inserted.
		int insert_step;       // Indicate which step (1,2,3) is the program at of insertion. 
		int ith_triangle;      // Used by: Translate, Delete, Animation. Which triangle was clicked.
		bool triangle_clicked; // If the mouse now clicked on a triangle.
		int closest_vertex;    // Used by: Colorize, Bezier
		int bezier_step;       // Which step is the program at when editing bezier curve.
		float aspect_ratio;
		float width;
		float height;
		int animation_type;    // animation type: 1-7.
		int snap_num;          // screen shot counter. for different file names.

		Eigen::MatrixXf V;
		Eigen::Matrix4f view;
		Eigen::MatrixXf model;
		Eigen::MatrixXf translation;
		Eigen::MatrixXf rotation;
		Eigen::MatrixXf scaling;

		Vector2d p0; // previous cursor position
		Vector2d p1; // current cursor position

	void init(void);
	bool click_on_triangle(Eigen::Vector2d world_coord_2d);
	void find_closest_vertex(int from, int to, int nv);
	void model_matrix_expand(void);
	void rotate_by(double degree, int direction);
	void scale_by(double percentage, int up);
	void delete_at(int triangle_index);
	void switch_mode(int m);
	float bezier_curve(float V1, float V2, float V3, float V4, float t);
	void screenshot(const char* filename);
	Eigen::Vector2d pixel_to_world_coord(Eigen::Vector4f pixel, int width, int height);
	std::string color_to_hex(float c);
};

inline void Editor::switch_mode(int m){
	triangle_clicked = false;
	ith_triangle = -1;
	insert_step = 0;
	closest_vertex = -1;
	bezier_step = 0;
	mode = m;
	V.conservativeResize(4, triangle_count * 3);

	if (m == INSERT_MODE) { std::cout << "Insertion mode is on." << std::endl; }
	else if (m == TRANSLATION_MODE) { std::cout << "Translation mode is on." << std::endl; }
	else if (m == DELETE_MODE) { std::cout << "Deletion mode is on." << std::endl; }
	else if (m == COLORIZE_MODE) { std::cout << "Colorization mode is on." << std::endl; }
	else if (m == ANIMATION_MODE) { std::cout << "Animation mode is on." << std::endl; }
	else if (m == BEZIER_CURVE_MODE) { std::cout << "Bezier curve mode is on." << std::endl; }
	else if (m == QUIT_MODE) { std::cout << "Quit Application." << std::endl; }
}

inline void Editor::init(void) {
	mode = 0;
	insert_step = 0;
	triangle_count = 1;
	triangle_clicked = false;
	ith_triangle = -1;
	closest_vertex = -1;
	animation_type = 1;
	bezier_step = 0;

	view = MatrixXf::Identity(4, 4);
	model = MatrixXf::Identity(4, 4);
	translation = MatrixXf::Identity(4, 4);
	rotation = MatrixXf::Identity(4, 4);
	scaling = MatrixXf::Identity(4, 4);

	p0 = Vector2d(0,0);
	p1 = Vector2d(0,0);
	V.resize(4,3);
	V << 0.0, 0.3, -0.3, 0.3, -0.3, -0.3, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0;
}

inline bool Editor::click_on_triangle(Eigen::Vector2d world_coord_2d) {
	for (int i = 0; i < V.cols(); i = i+3) {
		int j = V.cols() - i - 3;

		Eigen::Vector4f v1(V(0,j), V(1,j), 0, 1);
		Eigen::Vector4f v2(V(0,j+1), V(1,j+1), 0, 1);
		Eigen::Vector4f v3(V(0,j+2), V(1,j+2), 0, 1);
		// world coordinates
		v1 = model.block(0, floor(j/3) * 4, 4, 4) * v1;
		v2 = model.block(0, floor(j/3) * 4, 4, 4) * v2;
		v3 = model.block(0, floor(j/3) * 4, 4, 4) * v3;

		MatrixXf m(3,2);
		m << (RowVector2f)(v1.head(2)), (RowVector2f)(v2.head(2)), (RowVector2f)(v3.head(2));
		m.transposeInPlace();

		Matrix3f A;
		Vector3f b;
		A << m, 1, 1, 1;
		b << world_coord_2d(0), world_coord_2d(1), 1;
		Vector3f x = A.colPivHouseholderQr().solve(b);

		if (x(0) > 0 && x(1) > 0 && x(2) > 0) {
			ith_triangle = floor(j/3);
			triangle_clicked = true;
			return true;
		} else {
			ith_triangle = -1;
			triangle_clicked = false;
		}
	}
	return false;
}

inline Eigen::Vector2d Editor::pixel_to_world_coord(Eigen::Vector4f pixel, int width, int height) {
	Eigen::Vector4f canonical_coord((pixel(0)/width)*2-1, (pixel(1)/height)*2-1, 0, 1);
	Eigen::Vector4f world_coord = view.inverse() * canonical_coord; // Homogeneious
	Eigen::Vector2d world_coord_2d (world_coord(0), world_coord(1));
	return world_coord_2d;
}

inline void Editor::rotate_by(double degree, int direction) {
	if (mode == TRANSLATION_MODE && ith_triangle != -1) {
		double theta;
		if (direction == 0) {theta = (-1) * degree * (M_PI / 180);} //std::cout << "Rotate the primitive clockwise by 10 degree." << std::endl;
		else {theta = degree * (M_PI / 180);} //std::cout << "Rotate the primitive counter-clockwise by 10 degree." << std::endl;

		int i = ith_triangle * 3;
		Eigen::Vector4f v1(V(0,i), V(1,i), 0, 1);
		Eigen::Vector4f v2(V(0,i+1), V(1,i+1), 0, 1);
		Eigen::Vector4f v3(V(0,i+2), V(1,i+2), 0, 1);
		Vector2f barycenter ((v1(0) + v2(0) + v3(0))/3, (v1(1) + v2(1) + v3(1))/3);

		Eigen::Matrix4f m0 = MatrixXf::Identity(4, 4);
		Eigen::Matrix4f m1 = MatrixXf::Identity(4, 4);
		m0(0, 3) = -barycenter(0);
		m0(1, 3) = -barycenter(1);
		m1(0, 3) = barycenter(0);
		m1(1, 3) = barycenter(1);

		Matrix4f r = MatrixXf::Identity(4, 4);
		r.topLeftCorner(2,2) << cos(theta), -sin(theta), sin(theta), cos(theta);

		rotation.block(0, ith_triangle * 4, 4, 4) = m1 * r * m0 * rotation.block(0, ith_triangle * 4, 4, 4);
	}
}

inline void Editor::scale_by(double percentage, int up) {
	if (mode == TRANSLATION_MODE && ith_triangle != -1) {
		Matrix4f s = MatrixXf::Identity(4, 4);
		if (up) {s.topLeftCorner(2,2) << (1 - percentage), 0, 0, (1 - percentage);} //std::cout << "Scale the primitive up by 25%." << std::endl;
		else {s.topLeftCorner(2,2) << (1 + percentage), 0, 0, (1 + percentage);} //std::cout << "Scale the primitive down by 25%." << std::endl;

		int i = ith_triangle * 3;
		Eigen::Vector4f v1(V(0,i), V(1,i), 0, 1);
		Eigen::Vector4f v2(V(0,i+1), V(1,i+1), 0, 1);
		Eigen::Vector4f v3(V(0,i+2), V(1,i+2), 0, 1);
		Vector2f barycenter ((v1(0) + v2(0) + v3(0))/3, (v1(1) + v2(1) + v3(1))/3);

		Eigen::Matrix4f m0 = MatrixXf::Identity(4, 4);
		Eigen::Matrix4f m1 = MatrixXf::Identity(4, 4);
		m0(0, 3) = -barycenter(0);
		m0(1, 3) = -barycenter(1);
		m1(0, 3) = barycenter(0);
		m1(1, 3) = barycenter(1);

		scaling.block(0, ith_triangle * 4, 4, 4) = m1 * s * m0 * scaling.block(0, ith_triangle * 4, 4, 4);
	}
}

inline void Editor::delete_at(int triangle_index) {
	int i = triangle_index * 3;
	for(int j = 0; j < 3; j ++) {
		V.col(i + j) = V.col(triangle_count * 3 - 3 + j);
	}
	for(int j = 0; j < 4; j ++) {
		model.col(floor(i/3) * 4 + j) = model.col(triangle_count * 4 - 4 + j);
		translation.col(floor(i/3) * 4 + j) = translation.col(triangle_count * 4 - 4 + j);
		rotation.col(floor(i/3) * 4 + j) = rotation.col(triangle_count * 4 - 4 + j);
		scaling.col(floor(i/3) * 4 + j) = scaling.col(triangle_count * 4 - 4 + j);
	}
	triangle_count --;
	V.conservativeResize(4, triangle_count * 3);
	model.conservativeResize(4, triangle_count * 4);
	translation.conservativeResize(4, triangle_count * 4);
	rotation.conservativeResize(4, triangle_count * 4);
	scaling.conservativeResize(4, triangle_count * 4);
}

inline void Editor::find_closest_vertex(int from, int to, int nv) {
	closest_vertex = -1;
	double dist = 10.0;

	for (int i = from; i < to; i++) {
		Eigen::Vector4f v(V(0, i), V(1, i), 0, 1);
		if (nv == 3) { v = model.block(0, floor(i/3) * 4, 4, 4) * v; }
		if (nv == 4) { v = MatrixXf::Identity(4, 4) * v; }
		Eigen::Vector2d v_2d (v(0), v(1));

		double d = (p1 - v_2d).norm();
		if (d < dist) {
			dist = d;
			closest_vertex = i;
		}
	}
}

inline void Editor::model_matrix_expand(void) {
	triangle_count ++;

	model.conservativeResize(4, triangle_count * 4);
	translation.conservativeResize(4, triangle_count * 4);
	rotation.conservativeResize(4, triangle_count * 4);
	scaling.conservativeResize(4, triangle_count * 4);

	model.topRightCorner(4,4).setIdentity();
	translation.topRightCorner(4,4).setIdentity();
	rotation.topRightCorner(4,4).setIdentity();
	scaling.topRightCorner(4,4).setIdentity();
}

inline float Editor::bezier_curve(float V1, float V2, float V3, float V4, float t) {
    float s = 1-t;
    float T1 = (V1*s + V2*t) * s + (V2*s + V3*t) * t;
    float T2 = (V2*s + V3*t) * s + (V3*s + V4*t) * t;
    return T1*s + T2*t;
}

inline std::string Editor::color_to_hex(float c) {
	std::string color;
	if (c == -1.0) { color = "#BF332E";}
	if (c == 0.0) { color = "#000000"; }
	if (c == 1.0) { color = "#F08080";} //vec3(240,128,128) /255.0
	if (c == 2.0) { color = "#FFA500";} //vec3(255,165,0) /255.0; 
	if (c == 3.0) { color = "#F0E68C";} //vec3(240,230,140) /255.0; 
	if (c == 4.0) { color = "#90EE90";}//vec3(144,238,144) /255.0;
	if (c == 5.0) { color = "#66FAAA";} //vec3(102,205,170) /255.0;
	if (c == 6.0) { color = "#20B2AA";} //vec3(32,178,170) /255.0; 
	if (c == 7.0) { color = "#4169E1";} //vec3(65,105,225) /255.0; 
	if (c == 8.0) { color = "#7B68EE";} //vec3(123,104,238) /255.0;
	if (c == 9.0) { color = "#FFB6C1";} //vec3(255,182,193) /255.0;
	return color;
}

inline void Editor::screenshot(const char* filename) {
	char buff[1000];
	snprintf(buff, sizeof(buff), 
		"<svg xmlns='http://www.w3.org/2000/svg' version='1.1' width='%f' height='%f'>"
		"<g transform='matrix(1 0 0 -1 0 %f)'>"
		"<rect x='0' y='0' width='%f' height='%f' fill='white'/>\n",width, height, height, width, height);
	std::string input = buff;
	for (int i = 0; i < (triangle_count * 3); i += 3) {
		Vector4f v1(V.col(i)(0), V.col(i)(1), 0,1);
		Vector4f v2(V.col(i+1)(0), V.col(i+1)(1), 0,1);
		Vector4f v3(V.col(i+2)(0), V.col(i+2)(1), 0,1);

		Matrix4f viewport;
		viewport << (width/2.0)*aspect_ratio,0,0,(width-1)/2.0,  0, height/2.0,0,(height-1)/2.0,  0,0,1,0,  0,0,0,1;
		Vector4f v1_ = viewport * model.block(0, floor(i/3) * 4, 4, 4) * v1;
		Vector4f v2_ = viewport * model.block(0, floor(i/3) * 4, 4, 4) * v2;
		Vector4f v3_ = viewport * model.block(0, floor(i/3) * 4, 4, 4) * v3;

		float max_x = std::max({v1_(0), v2_(0), v3_(0)});
		float min_x = std::min({v1_(0), v2_(0), v3_(0)});
		float max_y = std::max({v1_(1), v2_(1), v3_(1)});
		float min_y = std::min({v1_(1), v2_(1), v3_(1)});
		float triangle_width = max_x - min_x;
		float triangle_height = max_y - min_y;
		Vector2f normal_v1((v1_(0)-min_x)/triangle_width , (v1_(1)-min_y)/triangle_height);
		Vector2f normal_v2((v2_(0)-min_x)/triangle_width , (v2_(1)-min_y)/triangle_height);
		Vector2f normal_v3((v3_(0)-min_x)/triangle_width , (v3_(1)-min_y)/triangle_height);
		Vector2f midpoint = (normal_v2 + normal_v3)/2;

		std::string c1,c2,c3;
		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff),
			"<linearGradient id='c%d' gradientUnits='objectBoundingBox' x1='%f' y1='%f' x2='%f' y2='%f'>"
			"<stop offset='0%%' stop-color='%s'/><stop offset='100%%' stop-color='%s'/></linearGradient>\n"
			"<linearGradient id='c%d' gradientUnits='objectBoundingBox' x1='%f' y1='%f' x2='%f' y2='%f'>"
			"<stop offset='0%%' stop-color='%s'/><stop offset='100%%' stop-color='%s' stop-opacity='0'/></linearGradient>\n"
			"<path d='M %f,%f  L %f,%f  %f,%f Z' fill='url(#c%d)'/><path d='M %f,%f  L %f,%f  %f,%f Z' fill='url(#c%d)'/>\n", 
			i, normal_v2(0),normal_v2(1), normal_v3(0), normal_v3(1), color_to_hex(V(2,i+1)).c_str(), color_to_hex(V(2,i+2)).c_str(),
			i+1, normal_v1(0), normal_v1(1), midpoint(0), midpoint(1), color_to_hex(V(2,i)).c_str(), color_to_hex(V(2,i)).c_str(),
			v1_(0),v1_(1), v2_(0),v2_(1), v3_(0),v3_(1), i,
			v1_(0),v1_(1), v2_(0),v2_(1), v3_(0),v3_(1), i+1);
		std::string tri_str = buff;
		input += tri_str;
	}
	input += "</g></svg>";
    std::ofstream file(filename);
	file << input;
	file.close();
}

#endif

