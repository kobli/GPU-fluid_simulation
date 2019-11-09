#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include "utils.hpp"

using namespace std;
using namespace glm;

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 1024

class Bounds {
	public:
		Bounds(vec3 _center, vec3 _size): center{_center} {
			transform = scale(identity<mat4x4>(), _size);
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			vector<vec3> vertices = {
				{0,0,0},
				{0,0,1},
				{1,0,1},
				{1,0,0},
				{0,1,0},
				{0,1,1},
				{1,1,1},
				{1,1,0},
			};
			glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);

			glGenBuffers(1, &vboLineIndices);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboLineIndices);
			vector<GLuint> lineIndices = {
				0,1,
				1,2,
				2,3,
				3,0,

				4,5,
				5,6,
				6,7,
				7,4,

				0,4,
				1,5,
				2,6,
				3,7,
			};
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size()*sizeof(GLuint), lineIndices.data(), GL_STATIC_DRAW);
		}

		void draw() {
			glBindVertexArray(vao);
			GLuint program;
			glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
			setUniform(program, transform, "Model");
			glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
		}

		vec3 center;
		mat4x4 transform;
		GLuint vao;
		GLuint vbo;
		GLuint vboLineIndices;
};

class Application {
	public:
		Application(): b{{}, {1,1,1}} {
			mat4x4 view = lookAt({-2,2,.5}, {.5,.5,.5}, UP);
			mat4x4 projection = perspective(70., 1., 0.1, 1000.);
			camera = projection*view;
			shader = loadShaderProgram({
					make_tuple(GL_VERTEX_SHADER, "shaders/pt.vert"),
					make_tuple(GL_FRAGMENT_SHADER, "shaders/cameraLight.frag"),
					});
			if(!shader)
				cerr << "Failed to load shader program\n";
		}

		void draw() {
			glClearColor( 0.0, 0.0, 0.0, 1.0 );
			glClear( GL_COLOR_BUFFER_BIT );
			glUseProgram(shader);
			setUniform(shader, camera, "ViewProject");
			b.draw();
		}

		void update() {
		}

		Bounds b;
		GLuint shader;
		mat4x4 camera;
};
unique_ptr<Application> app;

void DrawImage( void ) {
	app->draw();
  glutSwapBuffers();
}
static void HandleKeys(unsigned char key, int x, int y) {
  switch (key) {
    case 27:	// ESC
      exit(0);
  }
}
void idleFunc() {
	app->update();
  glutPostRedisplay();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitWindowSize(IMAGE_WIDTH, IMAGE_HEIGHT);
  glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
  glutCreateWindow("SPH demo");

	if(glewInit()) {
		cerr << "Cannot initialize GLEW\n";
		exit(EXIT_FAILURE);
	}
	if(glDebugMessageCallback){
		cout << "Register OpenGL debug callback " << endl;
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglCallbackFunction, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);
	}
	else
		cout << "glDebugMessageCallback not available" << endl;

  glutDisplayFunc(DrawImage);
  glutKeyboardFunc(HandleKeys);
  glutIdleFunc(idleFunc);

	app.reset(new Application);
  glutMainLoop();
  return 0;
}
