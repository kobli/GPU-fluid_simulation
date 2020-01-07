#define _USE_MATH_DEFINES
#include <memory>
#include <sstream>
#include <iomanip>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/freeglut.h>
#include "utils.hpp"
#include "application.hpp"
#include "sphGpu.hpp"
#include "sphCpu.hpp"

///////////////////////////// BEGINNING OF CONFIGURATION ////////////////////////////////

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 1024

const unsigned ParticleN = 1024*8; // must be power of two
const unsigned SubdivisionN = 8; // must be power of two
const vec3 BoxSize{2,2,2};

const float Step = 0.005; // [seconds]
const float H = 0.1;
const float M = 32;
const float Rho0 = 1;
const float K = 2.4;
const float Mu = 2048;

using SPHimpl = SPHgpu;
//using SPHimpl = SPHcpu;

///////////////////////////// END OF CONFIGURATION ////////////////////////////////
static_assert(ParticleN >= 1024, "ParticleN must be at least 1024 because of localGroupSize");
SPHconfig config(ParticleN, SubdivisionN);

using namespace std;

unique_ptr<Application> app;

void DrawImage( void ) {
	app->draw();
  glutSwapBuffers();
}

static void HandleKeys(unsigned char key, int /*x*/, int /*y*/) {
  switch (key) {
    case 27:	// ESC
      exit(0);
			break;
		case 'r':
			app->reset();
			break;
		case 's':
			config.Step /= 2;
			break;
		case 'S':
			config.Step *= 2;
			break;
		case 'h':
			config.H /= 2;
			break;
		case 'H':
			config.H *= 2;
			break;
		case 'm':
			config.M /= 2;
			break;
		case 'M':
			config.M *= 2;
			break;
		case 'k':
			config.K /= 2;
			break;
		case 'K':
			config.K *= 2;
			break;
		case 'u':
			config.Mu /= 2;
			break;
		case 'U':
			config.Mu *= 2;
			break;
		case 'o':
			config.Rho0 /= 2;
			break;
		case 'O':
			config.Rho0 *= 2;
			break;
  }
	cout << "step: " << Step << endl;
	cout << "h: " << H << endl;
	cout << "m: " << M << endl;
	cout << "k: " << K << endl;
	cout << "mu: " << Mu << endl;
	cout << "rho0: " << Rho0 << endl;
	cout << endl;
}
void idleFunc() {
	app->update();
	ostringstream title;
	title << "SPH demo - avg frame time: " << std::fixed << setw(8) << setprecision(2) << app->avgFrameTime << " [ms]";
	glutSetWindowTitle(title.str().c_str());
  glutPostRedisplay();
}

int main(int argc, char* argv[]) {
	config.Step = Step;
	config.H = H;
	config.M = M;
	config.Rho0 = Rho0;
	config.K = K;
	config.Mu = Mu;

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
		glDebugMessageCallback((GLDEBUGPROC)openglCallbackFunction, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);
		glEnable(GL_DEBUG_OUTPUT);
	}
	else
		cout << "glDebugMessageCallback not available" << endl;

  glutDisplayFunc(DrawImage);
  glutKeyboardFunc(HandleKeys);
  glutIdleFunc(idleFunc);

	Bounds b(BoxSize);
	app.reset(new Application(std::unique_ptr<SPH>(new SPHimpl(config, b)), b));
  glutMainLoop();
  return 0;
}
