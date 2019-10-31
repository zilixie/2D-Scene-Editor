// This example is heavily based on the tutorial at https://open.gl

// OpenGL Helpers to reduce the clutter
#include "Helpers.h"

using namespace std;
using namespace Eigen;
using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

#include "Editor.h"

// Global Variables
VertexBufferObject VBO; // VertexBufferObject wrapper
Editor e;

// Callback Functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
	// Get the size of the window.
	int width, height;
	glfwGetCursorPos(window, &xpos, &ypos);
	glfwGetWindowSize(window, &width, &height);

	Eigen::Vector4f pixel(xpos, height-1-ypos, 0, 1);
	// Convert screen position to world coordinates.
	Eigen::Vector2d world_coord_2d = e.pixel_to_world_coord(pixel, width, height);
	// Track the mouse positions
	if (e.p0 != Vector2d(0,0) && e.p1 != Vector2d(0,0)) {
		e.p0 = e.p1;
		e.p1 = world_coord_2d;
	} else { e.p0 = e.p1 = world_coord_2d; }
	// The last column of V stores position value of cursor.
	if (e.mode == INSERT_MODE && (e.insert_step == 1 || e.insert_step == 2)) {
		e.V.col(e.V.cols()-1) << e.p1(0), e.p1(1), e.V(2, e.V.cols()-1), e.V(3, e.V.cols()-1);
		VBO.update(e.V);
	} // Implement the drag effect below
	if (e.mode == TRANSLATION_MODE && e.ith_triangle != -1 && e.triangle_clicked) {
		e.translation(0, e.ith_triangle * 4 + 3) += (e.p1(0) - e.p0(0));
		e.translation(1, e.ith_triangle * 4 + 3) += (e.p1(1) - e.p0(1));
	} // Special case for handling bezier curve.
	if (e.mode == BEZIER_CURVE_MODE && (e.bezier_step == 1 || e.bezier_step == 2 || e.bezier_step == 3)) {
		e.V.col(e.V.cols()-1) << e.p1(0), e.p1(1), e.V(2, e.V.cols()-1), e.V(3, e.V.cols()-1);
		VBO.update(e.V);
	}
	if (e.mode == BEZIER_CURVE_MODE && (e.bezier_step == 5)) {
		e.V(0,e.closest_vertex) += (e.p1(0) - e.p0(0));
		e.V(1,e.closest_vertex) += (e.p1(1) - e.p0(1));
		VBO.update(e.V);
	}
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods) {
	if (e.mode == INSERT_MODE && action == GLFW_PRESS) {
		e.insert_step ++; //increment insert step
		if (e.insert_step == 1) { // first click for insert
			e.V.conservativeResize(4, e.triangle_count * 3 + 2);
			e.V.col(e.triangle_count * 3) << e.p1(0), e.p1(1), 0.0, 0.0;
			e.V.col(e.triangle_count * 3 + 1) << e.p1(0), e.p1(1), 0.0, 0.0;
		}
    	else if (e.insert_step == 2) {
    		e.V.conservativeResize(4, e.triangle_count * 3 + 3);
    		e.V.col(e.triangle_count * 3 + 2) << e.p1(0), e.p1(1), 0.0, 0.0;
    	}
    	else if (e.insert_step == 3) {
			for (int j = 0; j < 3; j++) {
				e.V(2,e.triangle_count * 3 + j) = -1.0;
				e.V(3,e.triangle_count * 3 + j) = 0.0;
			}
			e.insert_step = 0; // After one insert is finished, reset insert_step to be 0.
			e.model_matrix_expand(); // Initialize matrix for this triangle.
    	}
    } 
    else if (e.mode == TRANSLATION_MODE) {	
		if (e.click_on_triangle(e.p1) && action == GLFW_RELEASE) { e.triangle_clicked = false; }
    } 
    else if (e.mode == DELETE_MODE) {
		if (e.click_on_triangle(e.p1) && action == GLFW_PRESS) { // down edge: delete triangle
			e.delete_at(e.ith_triangle);
			e.ith_triangle = -1;
			e.triangle_clicked = false;
		}
		if (action == GLFW_RELEASE) { // up edge: unclicked
			e.ith_triangle = -1;
		}
    }
    else if (e.mode == COLORIZE_MODE) {
    	e.find_closest_vertex(0, (int)e.V.cols(), 3);
    }
	else if (e.mode == ANIMATION_MODE) {
		if (e.ith_triangle != -1 && action == GLFW_PRESS) { e.ith_triangle = -1; }
		else if (e.click_on_triangle(e.p1) && action == GLFW_RELEASE) { e.triangle_clicked = false; }

		if (e.ith_triangle != -1) {
			e.V(3,e.ith_triangle * 3) = e.animation_type;
			e.V(3,e.ith_triangle * 3+1) = e.animation_type;
			e.V(3,e.ith_triangle * 3+2) = e.animation_type;
			VBO.update(e.V);
		}
	}
	else if (e.mode == BEZIER_CURVE_MODE) {
		if (action == GLFW_PRESS) {
			e.bezier_step ++;
			if (e.bezier_step == 1) {
				e.V.conservativeResize(4, e.triangle_count * 3 + 2);
				e.V.col(e.triangle_count * 3) << e.p1(0), e.p1(1), 0.0, 0.0;
				e.V.col(e.triangle_count * 3 + 1) << e.p1(0), e.p1(1), 0.0, 0.0;
			}
			else if (e.bezier_step == 2 || e.bezier_step == 3) {
				e.V.conservativeResize(4, e.triangle_count * 3 + e.bezier_step + 1);
				e.V.col(e.triangle_count * 3 + e.bezier_step) << e.p1(0), e.p1(1), 0.0, 0.0;
			}
			else if (e.bezier_step == 4) {
				e.V.conservativeResize(4, e.triangle_count * 3 + 4 + 100);
				e.V.rightCols(100) = MatrixXf::Zero(4,100);
			}
			else if (e.bezier_step == 5) {
				e.find_closest_vertex(e.triangle_count * 3, e.triangle_count * 3 + 4, 4);
			}
    	}
    	else if (action == GLFW_RELEASE && e.bezier_step == 5) {
    		e.bezier_step = 4;
    	}
	}
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        e.V.col(0) << e.p1(0), e.p1(1), e.V(2, 0), e.V(3, 0);     // Update the position of the first vertex if the left button is pressed
    }
    VBO.update(e.V); // Upload the change to the GPU
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_Z && action == GLFW_RELEASE) { std::cout << "view:\n" << e.view << "\n" << std::endl; }
	if (key == GLFW_KEY_X && action == GLFW_RELEASE) { std::cout << "model:\n" << e.model << "\n" << std::endl; }
	if (key == GLFW_KEY_V && action == GLFW_RELEASE) { std::cout << "Vertex:\n" << e.V << "\n" << std::endl; }
	if (key == GLFW_KEY_N && action == GLFW_RELEASE) { std::cout << "triangle_clicked: " << e.triangle_clicked << "  ith_triangle: " << e.ith_triangle << "\n" << std::endl; }
	if (key == GLFW_KEY_M && action == GLFW_RELEASE) { std::cout << "closest_vertex:\n" << e.closest_vertex << "\n" << std::endl; }

	if (key == GLFW_KEY_I && action == GLFW_RELEASE) { e.switch_mode(INSERT_MODE); }
	else if (key == GLFW_KEY_O && action == GLFW_RELEASE) { e.switch_mode(TRANSLATION_MODE); }
	else if (key == GLFW_KEY_P && action == GLFW_RELEASE) { e.switch_mode(DELETE_MODE); }
	else if (key == GLFW_KEY_C && action == GLFW_RELEASE) { e.switch_mode(COLORIZE_MODE); }
	else if (key == GLFW_KEY_U && action == GLFW_RELEASE) { e.switch_mode(ANIMATION_MODE); }
	else if (key == GLFW_KEY_Y && action == GLFW_RELEASE) { e.switch_mode(BEZIER_CURVE_MODE); }

	else if (key == GLFW_KEY_H && action == GLFW_RELEASE) { e.rotate_by(10,1); }
	else if (key == GLFW_KEY_J && action == GLFW_RELEASE) { e.rotate_by(10,0); }
	else if (key == GLFW_KEY_K && action == GLFW_RELEASE) { e.scale_by(0.25,1); }
	else if (key == GLFW_KEY_L && action == GLFW_RELEASE) { e.scale_by(0.25,0); }

	else if (key >= 49 && key <= 57 && e.mode == COLORIZE_MODE) {
		e.V(2, e.closest_vertex) = float(key - 48); 
	}
	else if (key >= 49 && key <= 55 && e.mode == ANIMATION_MODE) {
		e.animation_type = key - 48;
		e.V(3,e.ith_triangle * 3) = e.animation_type;
		e.V(3,e.ith_triangle * 3+1) = e.animation_type;
		e.V(3,e.ith_triangle * 3+2) = e.animation_type;
	}
	else if ((key == GLFW_KEY_MINUS || key == GLFW_KEY_EQUAL) && action == GLFW_RELEASE) {
		if (key == GLFW_KEY_MINUS) {e.view.topLeftCorner(2,2) = e.view.topLeftCorner(2,2) * 0.8;}
		if (key == GLFW_KEY_EQUAL) {e.view.topLeftCorner(2,2) = e.view.topLeftCorner(2,2) * 1.2;}
	}
	else if ((key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D) && action == GLFW_RELEASE) {
		Vector4f p_world = e.view.inverse() * Vector4f(1,1,0,0); //don't set the last column be 0.
		//p_world is just half of world distance of screen.
		if (key == GLFW_KEY_W) { e.view(1,3) += 0.4 * p_world(1) * e.view(1,1); }
		if (key == GLFW_KEY_A) { e.view(0,3) -= 0.4 * p_world(0) * e.aspect_ratio * e.view(1,1); }
		if (key == GLFW_KEY_S) { e.view(1,3) -= 0.4 * p_world(1) * e.view(1,1); }
		if (key == GLFW_KEY_D) { e.view(0,3) += 0.4 * p_world(0) * e.aspect_ratio * e.view(1,1); }
	}
	else if (key == GLFW_KEY_E && action == GLFW_RELEASE) {
		e.switch_mode(0);
	}
	else if (key == GLFW_KEY_Q && action == GLFW_RELEASE) { e.switch_mode(QUIT_MODE); }
	else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
		char filename[100];
		sprintf(filename, "snap%d.svg", e.snap_num);
		e.screenshot(filename);
		e.snap_num ++;
	}
    VBO.update(e.V); // Upload the change to the GPU
}

// Main
int main(void) {
    GLFWwindow* window;
    if (!glfwInit()) { return -1; }     // Initialize the library
    glfwWindowHint(GLFW_SAMPLES, 8);    // Activate supersampling
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
	#ifdef __APPLE__
    	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
	#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, "Assignment 2", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Make the window's context current

    #ifndef __APPLE__
		glewExperimental = true;
		GLenum err = glewInit();
		if(GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		}
		glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    e.init();
    							// Initialize the VAO
    VertexArrayObject VAO;		// A Vertex Array Object (or VAO) is an object that describes how the vertex
    VAO.init();					// attributes are stored in a Vertex Buffer Object (or VBO). This means that
    VAO.bind();					// the VAO is not the actual object storing the vertex data,
    							// but the descriptor of the vertex data.

    VBO.init();           // Initialize the VBO with the vertices data
    VBO.update(e.V);      // A VBO is a data container that lives in the GPU memory

    				  	// Initialize the OpenGL Program
    Program program; 	// A program controls the OpenGL pipeline and it must contains
    					// at least a vertex shader and a fragment shader to be valid

    program.init("../src/vertex_shader.glsl","../src/fragment_shader.glsl","outColor"); // Compile the two shaders and upload the binary to the GPU
	program.bind();          // Note that we have to explicitly specify that the output "slot" called outColor
	                         // is the one that we want in the fragment buffer (and thus on screen)

    glfwSetKeyCallback(window, key_callback);                 // Register the keyboard callback
    glfwSetMouseButtonCallback(window, mouse_click_callback); // Register the mouse callback
    glfwSetCursorPosCallback(window, mouse_move_callback);    // Register the cursor move callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // Update viewport

	TimePoint t_start = std::chrono::high_resolution_clock::now();
    // Loop until the user closes the window

    while (!glfwWindowShouldClose(window) && e.mode != QUIT_MODE) {
        VAO.bind();    // Bind your VAO (not necessary if you have only one)
		program.init("../src/vertex_shader.glsl","../src/fragment_shader.glsl","outColor");
		program.bind();

		// The following line connects the VBO we defined above with the position "slot" in the vertex shader
		program.bindVertexAttribArray("position",VBO); // The vertex shader wants the position of the vertices as an input.

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		e.aspect_ratio = float(height)/float(width);
		e.width = float(width);
		e.height = float(height);
		if (e.view(0,0)/e.view(1,1) != e.aspect_ratio) { e.view(0,0) = e.aspect_ratio * e.view(1,1); }

        // Clear the framebuffer
        glClearColor(1.0f, 1.0f, 1.0f, 0.4f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniformMatrix4fv(program.uniform("view"), 1, GL_FALSE, e.view.data());
        glUniform1i(program.uniform("animated"), e.mode == ANIMATION_MODE);

        if (e.mode != BEZIER_CURVE_MODE) {
			if (e.mode == INSERT_MODE && e.insert_step >= 1) {
				Eigen::Matrix4f identity = MatrixXf::Identity(4, 4);
				if (e.insert_step == 1){
					glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
					glDrawArrays(GL_LINES, (e.triangle_count * 3), 2);
				} 
				else if (e.insert_step ==  2){ //Display 3 lines
					glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
					glDrawArrays(GL_LINE_LOOP, (e.triangle_count * 3), 3);
				}
			}
			if (e.mode == TRANSLATION_MODE && e.ith_triangle != -1) {
				Eigen::Matrix4f t_m = e.translation.block(0, e.ith_triangle * 4, 4, 4);
				Eigen::Matrix4f r_m = e.rotation.block(0, e.ith_triangle * 4, 4, 4);
				Eigen::Matrix4f s_m = e.scaling.block(0, e.ith_triangle * 4, 4, 4);
				e.model.block(0, e.ith_triangle * 4, 4, 4) = t_m * r_m * s_m;
			}
			// Draw triangles
			for (int i = 0; i < (e.triangle_count * 3); i += 3) {
				if ((int)floor(i/3) == e.ith_triangle && e.ith_triangle != -1 && e.triangle_clicked) {
					glUniform1i(program.uniform("click"), 1);
				}  else { glUniform1i(program.uniform("click"), 0); }

				glUniform1i(program.uniform("is_ith_triangle"), (int)floor(i/3) == e.ith_triangle);

				Vector2f v1(e.V.col(i)(0), e.V.col(i)(1));
				Vector2f v2(e.V.col(i+1)(0), e.V.col(i+1)(1));
				Vector2f v3(e.V.col(i+2)(0), e.V.col(i+2)(1));
				Vector2f barycenter ((v1(0)+v2(0)+v3(0))/3.0, (v1(1)+v2(1)+v3(1))/3.0) ;
				glUniform2f(program.uniform("barycenter"), barycenter(0), barycenter(1));

				auto t_now = std::chrono::high_resolution_clock::now();
				float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
				glUniform1f(program.uniform("time"), time + floor(v1(0)*1000));

				glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, &e.model(0, (int)floor(i/3) * 4));
				glDrawArrays(GL_TRIANGLES, i, 3);
			}
        }
        else if (e.mode == BEZIER_CURVE_MODE) {
        	Eigen::Matrix4f identity = MatrixXf::Identity(4, 4);
        	if (e.bezier_step == 1){
				glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
				glDrawArrays(GL_LINES, (e.triangle_count * 3), 2);
        	}
        	else if (e.bezier_step ==  2){ //Display 3 lines
				glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
				glDrawArrays(GL_LINE_STRIP, (e.triangle_count * 3), 3);
			}
			else if (e.bezier_step ==  3){ //Display 4 lines
				glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
				glDrawArrays(GL_LINE_STRIP, (e.triangle_count * 3), 4);
			}
			else if (e.bezier_step >=  4) {
				glUniformMatrix4fv(program.uniform("model"), 1, GL_FALSE, identity.data());
				glDrawArrays(GL_LINE_STRIP, (e.triangle_count * 3), 4);

				int i = e.triangle_count * 3;
				Vector2f v1(e.V.col(i)(0), e.V.col(i)(1));
				Vector2f v2(e.V.col(i+1)(0), e.V.col(i+1)(1));
				Vector2f v3(e.V.col(i+2)(0), e.V.col(i+2)(1));
				Vector2f v4(e.V.col(i+3)(0), e.V.col(i+3)(1));
				for (int j=0; j < 100; j ++) {
					float t = float(j)/100;
					float bezier_x = e.bezier_curve(v1(0), v2(0), v3(0), v4(0), t);
					float bezier_y = e.bezier_curve(v1(1), v2(1), v3(1), v4(1), t);

					e.V.col(e.triangle_count * 3 + 4 + j) << bezier_x, bezier_y, 0.0;
				}
				glDrawArrays(GL_LINE_STRIP, (e.triangle_count * 3 + 4), 100);
			}
        }
		glfwSwapBuffers(window); // Swap front and back buffers
		glfwPollEvents(); // Poll for and process events
    }
    // Deallocate opengl memory
    program.free();
    VAO.free();
    VBO.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}
