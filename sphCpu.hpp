#ifndef SPHCPU_HPP_20_01_07_21_10_15
#define SPHCPU_HPP_20_01_07_21_10_15 
#include "sph.hpp"

class SPHcpu: public SPH {
	public:
		SPHcpu(SPHconfig &config, Bounds& _b);
		void reset() override;
		void update() override;
		void collide();
		unsigned particlePosToCellID(const vec3& particlePos);
		unsigned cellPosToID(const vec3& c);
		std::vector<unsigned> nnCells(const vec3& particlePos);
		void updateCellRecords();

	private:
		std::vector<vec3> particlePos;
		std::vector<vec3> particleVel;
		std::vector<vec3> particlePosTmp;

		std::vector<CellRecord> cellRecords;
		std::vector<ParticleRecord> particleRecords;
};

#endif /* SPHCPU_HPP_20_01_07_21_10_15 */
