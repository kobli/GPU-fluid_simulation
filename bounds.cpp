#include "bounds.hpp"

Bounds::Bounds(vec3 _size): min{0,0,0}, max{_size} {
	transform = scale(glm::identity<glm::mat4x4>(), _size);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	std::vector<vec3> vertices = {
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
	std::vector<GLuint> lineIndices = {
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

void Bounds::draw() {
	glBindVertexArray(vao);
	GLuint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
	setUniform(program, transform, "Model");
	glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
}

bool Bounds::isOutside(const vec3 &p, vec3 &n) {
	bool r = true;
	if(p.x < min.x)
		n = {1,0,0};
	else if(p.x > max.x)
		n = {-1,0,0};
	else if(p.y < min.y)
		n = {0,1,0};
	else if(p.y > max.y)
		n = {0,-1,0};
	else if(p.z < min.z)
		n = {0,0,1};
	else if(p.z > max.z)
		n = {0,0,-1};
	else
		r = false;
	return r;
}
