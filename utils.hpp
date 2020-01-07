#ifndef UTILS_HPP_18_12_29_15_46_10
#define UTILS_HPP_18_12_29_15_46_10 
#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct Material {
	glm::vec4 color;
	float ambientK;
	float diffuseK;
	float specularK;
	float shininess;
};

using vec3 = glm::vec3;
using vec4 = glm::vec4;

const glm::vec3 UP(0,1,0);

float frand();

std::ostream& operator<<(std::ostream& s, const glm::vec3& v);
std::istream& operator>>(std::istream& s, glm::vec3& v);

void setUniform(GLuint program, const glm::mat4x4& m, std::string name);

GLuint loadShaderProgram(std::vector<std::tuple<GLenum,std::string>> shaderFiles);

void openglCallbackFunction(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam);

#endif /* UTILS_HPP_18_12_29_15_46_10 */
