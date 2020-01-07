#ifndef SPH_HPP_20_01_07_21_09_53
#define SPH_HPP_20_01_07_21_09_53 
#include "sphKernels.hpp"
#include "bounds.hpp"

struct ParticleRecord {
	unsigned cellID;
	unsigned particleID;
};

struct CellRecord {
	unsigned firstParticleID;
	unsigned particleN;
};

struct SPHconfig {
	SPHconfig(unsigned _particleN, unsigned _subdivisionN): SubdivisionN{_subdivisionN}, particleN{_particleN}
	{}
	float Step; // [seconds]
	float H;
	float M;
	float Rho0;
	float K;
	float Mu;
	const unsigned SubdivisionN; // must be power of two
	const unsigned particleN;
};

class SPH {
	public:
		SPH(SPHconfig &_config, Bounds& _b);
		void draw();
		void setParticlePositionAttrBuffer(int numPositionComponents);

		virtual void reset() = 0;
		virtual void update() = 0;

	protected:
		template <typename T>
			void bufferData(GLuint buffer, const std::vector<T>& data, GLenum usage) {
				glBindVertexArray(vao);
				glBindBuffer(GL_ARRAY_BUFFER, buffer);
				glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(T), data.data(), usage);
			}


	private:
		void initSphereMesh();

	public:
		float frameTime;

	protected:
		GLuint particlePositionBuff;
		Bounds b;
		SPHconfig &config;

	private:
		GLuint vao;
		GLuint vbo;
		GLuint vboNormals;
		GLuint vboIndices;
		unsigned indexN;
};

#endif /* SPH_HPP_20_01_07_21_09_53 */
