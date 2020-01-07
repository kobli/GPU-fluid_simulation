#ifndef BOUNDS_HPP_20_01_07_21_15_07
#define BOUNDS_HPP_20_01_07_21_15_07 
#include "utils.hpp"

class Bounds {
	public:
		Bounds(vec3 _size);

		void draw();
		bool isOutside(const vec3 &p, vec3 &n);

		vec3 min;
		vec3 max;
		glm::mat4x4 transform;
		GLuint vao;
		GLuint vbo;
		GLuint vboLineIndices;
};

#endif /* BOUNDS_HPP_20_01_07_21_15_07 */
