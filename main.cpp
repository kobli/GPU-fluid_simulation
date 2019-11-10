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

const float ParticleRad = 0.03;

struct Material {
	vec4 color;
	float ambientK;
	float diffuseK;
	float specularK;
	float shininess;
};

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

class SPH {
	public:
		SPH(unsigned n) {
			particlePos.resize(n);
			int i = 0;
			for(vec3& p : particlePos) {
				p = vec3(0.5, 1+i*ParticleRad*2, 0.5);
				++i;
			}
			initSphereMesh();
		}

		void initSphereMesh() {
			const int lats = 40,
						longs = 40;
			vector<GLfloat> vertices;
			vector<GLfloat> normals;
			vector<GLuint> indices;

			// sphere mesh generation code from https://stackoverflow.com/a/5989676/4120490
			float const R = 1./(float)(lats-1);
			float const S = 1./(float)(longs-1);
			int r, s;
			vertices.resize(lats*longs*3);
			normals.resize(lats*longs*3);
			std::vector<GLfloat>::iterator v = vertices.begin();
			std::vector<GLfloat>::iterator n = normals.begin();
			for(r = 0; r < lats; r++)
				for(s = 0; s < longs; s++) {
					float const y = sin( -M_PI_2 + M_PI * r * R );
					float const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
					float const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );

					*v++ = x;
					*v++ = y;
					*v++ = z;

					*n++ = x;
					*n++ = y;
					*n++ = z;
				}
			indices.resize(lats*longs*4);
			std::vector<GLuint>::iterator i = indices.begin();
			for(r = 0; r < lats; r++)
				for(s = 0; s < longs; s++) {
				*i++ = r*longs + s;
				*i++ = r*longs + (s+1);
				*i++ = (r+1)*longs + (s+1);
				*i++ = (r+1)*longs + s;
			}

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			glGenBuffers(1, &vboNormals);
			glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
			glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(1);

			glGenBuffers(1, &vboIndices);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			indexN = indices.size();

			glGenBuffers(1, &vboParticlePos);
			glBindBuffer(GL_ARRAY_BUFFER, vboParticlePos);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(2);
			glVertexAttribDivisor(2, 1);
		}

		void draw() {
			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vboParticlePos);
			glBufferData(GL_ARRAY_BUFFER, particlePos.size()*sizeof(vec3), particlePos.data(), GL_DYNAMIC_DRAW);

			GLuint program;
			glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
			mat4x4 transform = scale(identity<mat4x4>(), vec3(ParticleRad, ParticleRad, ParticleRad));
			setUniform(program, transform, "Model");
			setUniform(program, transpose(inverse(transform)), "ModelInvT");

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
			glDrawElementsInstanced(GL_QUADS, indexN, GL_UNSIGNED_INT, NULL, particlePos.size());
		}

	private:
		vector<vec3> particlePos;

		GLuint vao;
		GLuint vbo;
		GLuint vboNormals;
		GLuint vboIndices;
		GLuint vboParticlePos;
		unsigned indexN;
};

class Application {
	public:
		Application(): b{{}, {1,1,1}}, sph{100} {
			cameraPos = {-2,2,.5};
			mat4x4 cameraView = lookAt(cameraPos, {.5,.5,.5}, UP);
			mat4x4 projection = perspective(70., 1., 0.1, 1000.);
			camera = projection*cameraView;
			shader = loadShaderProgram({
					make_tuple(GL_VERTEX_SHADER, "shaders/pt.vert"),
					make_tuple(GL_FRAGMENT_SHADER, "shaders/cameraLight.frag"),
					});
			if(!shader)
				cerr << "Failed to load shader program\n";

			material.ambientK = .5;
			material.diffuseK = .5;
			material.color = {0.3,0.3,1,1};
		}

		void draw() {
			glClearColor( 0.0, 0.0, 0.0, 1.0 );
			glClear( GL_COLOR_BUFFER_BIT );

			glUseProgram(shader);
			setUniform(shader, camera, "ViewProject");
			GLint diffuseKLoc = glGetUniformLocation(shader, "Mat.diffuseK");
			glUniform1f(diffuseKLoc, material.diffuseK);
			GLint ambientKLoc = glGetUniformLocation(shader, "Mat.ambientK");
			glUniform1f(ambientKLoc, material.ambientK);
			GLint colorLoc = glGetUniformLocation(shader, "Mat.color");
			glUniform4fv(colorLoc, 1, material.color.data.data);
			GLint camPosLoc = glGetUniformLocation(shader, "CameraPos");
			glUniform3fv(camPosLoc, 1, cameraPos.data.data);

			glVertexAttrib3f(2, 0, 0, 0);

			b.draw();
			sph.draw();
		}

		void update() {
		}

		Bounds b;
		GLuint shader;
		vec3 cameraPos;
		mat4x4 camera;
		SPH sph;
		Material material;
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
