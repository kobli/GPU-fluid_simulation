//----------------------------------------------------------------------------------------
/**
 * \file       sph.hpp
 * \author     Ales Koblizek
 * \date       2020/01/07
 * \brief      SPH base class and related declarations
*/
//----------------------------------------------------------------------------------------
#ifndef SPH_HPP_20_01_07_21_09_53
#define SPH_HPP_20_01_07_21_09_53 
#include "sphKernels.hpp"
#include "bounds.hpp"

/// Used for grid-based neighbour search
struct ParticleRecord {
	unsigned cellID;
	unsigned particleID;
};

/// Used for grid-based neighbour search
struct CellRecord {
	unsigned firstParticleID;
	unsigned particleN;
};

/// Holds SPH coefficients and other simulation parameters
struct SPHconfig {
	SPHconfig(unsigned _particleN, unsigned _subdivisionN): SubdivisionN{_subdivisionN}, particleN{_particleN}
	{}
	float Step; /// simulation step [seconds]
	float H; /// kernel radius
	float M; /// particle mass - is the same for all particles
	float Rho0; /// fluid rest density - same for all particles
	float K; /// pressure coefficient
	float Mu; /// viscosity coefficient
	const unsigned SubdivisionN; /// neighbour-search grid number of subdivisions in each dimension (must be power of two)
	const unsigned particleN; /// numbe of particles
};


/* Base class for SPH implementation.
 * Draws the particles.
 */
class SPH {
	public:
		SPH(SPHconfig &_config, Bounds& _b);

		/// draw particles
		void draw();
		/// sets the buffer which contains the particle positions
		/// \param numPositionComponents 	Number of components of the position vector. (GPU implementation uses 4 component vectors)
		void setParticlePositionAttrBuffer(int numPositionComponents);

		/// restart the simulation
		virtual void reset() = 0;
		/// single simulation step
		virtual void update() = 0;

	protected:
		template <typename T>
			void bufferData(GLuint buffer, const std::vector<T>& data, GLenum usage) {
				glBindVertexArray(vao);
				glBindBuffer(GL_ARRAY_BUFFER, buffer);
				glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(T), data.data(), usage);
			}


	private:
		/// initializes the sphere mesh used for particle visualization
		void initSphereMesh();

	public:
		float frameTime; /// the derived classes should write here the time required for simulation step

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
