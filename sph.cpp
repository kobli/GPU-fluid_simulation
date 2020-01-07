#include "sph.hpp"
using namespace std;
using namespace glm;
const float ParticleRad = 0.02;

SPH::SPH(SPHconfig &_config, Bounds& _b): frameTime{0}, b{_b}, config{_config} {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	initSphereMesh();

	glGenBuffers(1, &particlePositionBuff);
	setParticlePositionAttrBuffer(3);
}

void SPH::draw() {
	glBindVertexArray(vao);
	GLuint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
	mat4x4 transform = scale(identity<mat4x4>(), vec3(ParticleRad, ParticleRad, ParticleRad));
	setUniform(program, transform, "Model");
	setUniform(program, transpose(inverse(transform)), "ModelInvT");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
	glDrawElementsInstanced(GL_QUADS, indexN, GL_UNSIGNED_INT, NULL, config.particleN);
}

void SPH::setParticlePositionAttrBuffer(int numPositionComponents) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuff);
	glVertexAttribPointer(2, numPositionComponents, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);
}

void SPH::initSphereMesh() {
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
}
