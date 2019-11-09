#include <iostream>
#include <cstring>
#include <fstream>
#include <cstddef>
#include <cassert>
#include <GL/glew.h>
#include <glm/gtc/random.hpp>
#include "utils.hpp"
using namespace std;

std::ostream& operator<<(std::ostream& s, const glm::vec3& v) {
	return s << v.x << " " << v.y << " " << v.z;
}

std::istream& operator>>(std::istream& s, glm::vec3& v) {
	return s >> v.x >> v.y >> v.z;
}

void setUniform(GLuint program, const glm::mat4x4& m, std::string name) {
	GLint loc = glGetUniformLocation(program, name.c_str());
	if(loc == -1)
		cerr << "Could not set uniform value " << name << " - uniform not found.\n";
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

std::string fileAsString(std::string fileName) {
	std::ifstream f(fileName);
	if(!f) {
		cerr << "Could not open " << fileName << endl;
		return {};
	}

	std::string s;

	f.seekg(0, std::ios::end);   
	s.reserve(f.tellg());
	f.seekg(0, std::ios::beg);

	s.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	return s;
}

GLuint loadShaderObject(std::string fileName, GLenum shaderType) {
	GLuint shaderObject = glCreateShader(shaderType);
	if(shaderObject) {
		std::string f = fileAsString(fileName);
		GLint l = f.length();
		if(l != 0) {
			const GLchar* strs = f.c_str();
			glShaderSource(shaderObject, 1, &strs, &l);
			glCompileShader(shaderObject);
			GLint r;
			glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &r);
			if(r == GL_TRUE) {
				return shaderObject;
			}
			else {
				char buf[1000];
				GLsizei l;
				glGetShaderInfoLog(shaderObject, sizeof(buf), &l, buf);
				cerr << buf << endl;
			}
		}
	}
	return 0;
}

GLuint loadShaderProgram(std::vector<std::tuple<GLenum,std::string>> shaderFiles) {
	GLuint shaderProgram = glCreateProgram();
	if(shaderProgram) {
		for(const auto& sf: shaderFiles) {
			GLuint shaderObject = loadShaderObject(std::get<1>(sf), std::get<0>(sf));
			if(shaderObject) {
				glAttachShader(shaderProgram, shaderObject);
			}
			else
				return 0; // FIXME probabble shaderObject leak
		}
		glLinkProgram(shaderProgram);
		GLint r;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &r);
		if(r == GL_TRUE) {
			return shaderProgram;
		}
		else {
			char buf[1000];
			GLsizei l;
			glGetProgramInfoLog(shaderProgram, sizeof(buf), &l, buf);
			cerr << buf << endl;
		}
	}
	return 0;
}

//source: https://blog.nobel-joergensen.com/2013/02/17/debugging-opengl-part-2-using-gldebugmessagecallback/
void openglCallbackFunction(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam){
	if(type == GL_DEBUG_TYPE_OTHER)
		return;

	cout << "---------------------opengl-callback-start------------" << endl;
	cout << "message: "<< message << endl;
	cout << "type: ";
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			cout << "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			cout << "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			cout << "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			cout << "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			cout << "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			cout << "OTHER";
			break;
	}
	cout << endl;

	cout << "id: " << id << endl;
	cout << "severity: ";
	switch (severity){
		case GL_DEBUG_SEVERITY_LOW:
			cout << "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			cout << "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			cout << "HIGH";
			break;
	}
	cout << endl;
	cout << "---------------------opengl-callback-end--------------" << endl;
}
