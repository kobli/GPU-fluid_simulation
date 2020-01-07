//----------------------------------------------------------------------------------------
/**
 * \file       sphGpu.hpp
 * \author     Ales Koblizek
 * \date       2020/01/07
 * \brief      GPU implementation of SPH
*/
//----------------------------------------------------------------------------------------
#ifndef SPHGPU_HPP_20_01_07_21_10_21
#define SPHGPU_HPP_20_01_07_21_10_21 
#include "sph.hpp"

/// GPU implementation of SPH
class SPHgpu: public SPH {
	public:
		SPHgpu(SPHconfig &config, Bounds& _b);
		void reset() override;
		void update() override;

	private:
		void step();
		void setConfigUniforms(GLuint program);

	private:
		GLuint queryID;
		bool queryActive;
		GLuint particleVelocityBuff;
		GLuint particleVelocityBuffOut;
		GLuint particlePositionBuffOut;
		GLuint densityBuff;
		GLuint particleRecBuffer;
		GLuint cellRecBuffer;
		GLuint updateProgram;
		GLuint densityProgram;
		GLuint particleRecProgram;
		GLuint sortParticleRecProgram;
		GLuint cellRecProgram;
		GLuint particleReorderProgram;
};

#endif /* SPHGPU_HPP_20_01_07_21_10_21 */
