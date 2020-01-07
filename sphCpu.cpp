#include <algorithm>
#include "sphCpu.hpp"
#include "utils.hpp"
using namespace std;
using namespace glm;

SPHcpu::SPHcpu(SPHconfig &config, Bounds& _b): SPH{config, _b} {
	particlePosTmp.resize(config.particleN);
	particleVel.resize(config.particleN,{});
	particlePos.resize(config.particleN);
	reset();
	cellRecords.resize(config.SubdivisionN*config.SubdivisionN*config.SubdivisionN);
	particleRecords.resize(config.particleN);
}

void SPHcpu::reset() {
	for(unsigned i = 0; i < particlePos.size(); ++i) {
		particlePos[i] = b.min + (b.max-b.min)*vec3(frand(), frand(), frand());
		particleVel[i] = normalize(vec3(rand(), rand(), rand()));
	}
	bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_DRAW);
}

void SPHcpu::update() {
	updateCellRecords();
	// calculate density and presure at each particle position
	vector<float> density(particlePos.size(), config.M);
	vector<float> pressure(particlePos.size(), 0);
	for(unsigned i = 0; i < particlePos.size(); ++i) {
		vector<unsigned> neighbourCells = nnCells(particlePos[i]);
		for(unsigned cellID : neighbourCells) {
			CellRecord r = cellRecords[cellID];
			for(unsigned j = r.firstParticleID; j < r.firstParticleID+r.particleN; ++j) {
				density[i] += config.M*w(particlePos[i]-particlePos[j], config.H);
			}
		}
		pressure[i] = config.K*(density[i]-config.Rho0);
		assert(density[i] != 0);
	}
	// calculate forces acting upon its particle, its acceleration; update its position and speed
	for(unsigned i = 0; i < particlePos.size(); ++i) {
		vec3 fPressure = {};
		vec3 fViscosity = {};
		vector<unsigned> neighbourCells = nnCells(particlePos[i]);
		for(unsigned cellID : neighbourCells) {
			CellRecord r = cellRecords[cellID];
			for(unsigned j = r.firstParticleID; j < r.firstParticleID+r.particleN; ++j) {
				vec3 fp = config.M*(pressure[i]+pressure[j])/(2*density[j])*wPresure1(particlePos[i]-particlePos[j], config.H);
				vec3 fv = (particleVel[j]-particleVel[i])*float(config.Mu*config.M/density[j]*wViscosity2(particlePos[i]-particlePos[j], config.H));
				assert(!isnan(length(fp)));
				assert(!isnan(length(fv)));
				fPressure += fp;
				fViscosity += fv;
			}
		}
		vec3 fGravity = -UP*9.81f*density[i];
		vec3 f = fViscosity + fPressure + fGravity;
		vec3 a = f/density[i];
		assert(length(a) >= 0);
		assert(length(f) >= 0);
		// update position using the current speed
		particlePosTmp[i] = particlePos[i] + particleVel[i]*config.Step;
		// update speed using the computed acceleration
		particleVel[i] += a*config.Step;
	}
	swap(particlePos, particlePosTmp);
	collide();

	bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_DRAW);
}

void SPHcpu::collide() {
	for(unsigned i = 0; i < particlePos.size(); ++i) {
		vec3 surfaceNormal;
		float velL = length(particleVel[i]);
		if(b.isOutside(particlePos[i], surfaceNormal) && velL > 0 && !isinf(velL)) {
			//if(surfaceNormal != vec3(0,-1,0)) // open-topped box
			{
				particlePos[i] += -particleVel[i]*config.Step;
				vec3 d = -particleVel[i]/velL;
				particleVel[i] = (2*dot(d, surfaceNormal)*surfaceNormal-d)*velL;
			}
		}
	}
}

unsigned SPHcpu::particlePosToCellID(const vec3& particlePos) {
	vec3 c = (particlePos - b.min) / (b.max - b.min) * float(config.SubdivisionN);
	return cellPosToID(c);
}

unsigned SPHcpu::cellPosToID(const vec3& c) {
	return unsigned(c.x)*config.SubdivisionN*config.SubdivisionN + unsigned(c.y)*config.SubdivisionN + unsigned(c.z);
}

vector<unsigned> SPHcpu::nnCells(const vec3& particlePos) {
	using std::max;
	using std::min;
	vector<unsigned> r;
	r.reserve(3*3*3);
	vec3 c = ((particlePos - b.min) / (b.max - b.min)) * float(config.SubdivisionN);
	int cx = c.x;
	int cy = c.y;
	int cz = c.z;
	const int maxC = int(config.SubdivisionN)-1;
	for(int x = max(0, cx-1); x <= min(maxC, cx+1); ++x) {
		for(int y = max(0, cy-1); y <= min(maxC, cy+1); ++y) {
			for(int z = max(0, cz-1); z <= min(maxC, cz+1); ++z) {
				r.push_back(cellPosToID(vec3(x,y,z)));
			}
		}
	}
	return r;
}

void SPHcpu::updateCellRecords() {
	for(CellRecord& r : cellRecords)
		r.particleN = 0;
	// for each particle: calculate cell coordinates -> hash
	for(unsigned i = 0; i < config.particleN; ++i) {
		particleRecords[i].particleID = i;
		particleRecords[i].cellID = particlePosToCellID(particlePos[i]);
	}
	// sort particleRecords by cell_id (this will form clusters for each cell)
	std::sort(particleRecords.begin(), particleRecords.end(), [](ParticleRecord& l, ParticleRecord& r){ return l.cellID < r.cellID; });
	// for each cell_id_hash create cell_rec (first_particle_rec, particle_rec_n)
	// reorder particle array according to particle_rec
	unsigned cellID = particleRecords[0].cellID;
	unsigned cellParticleN = 0;
	cellRecords[cellID].firstParticleID = 0;
	vector<vec3> particleVelTmp(config.particleN);
	for(unsigned i = 0; i < config.particleN; ++i) {
		ParticleRecord& r = particleRecords[i];
		particlePosTmp[i] = particlePos[r.particleID];
		particleVelTmp[i] = particleVel[r.particleID];
		if(r.cellID != cellID) {
			cellRecords[r.cellID].firstParticleID = i;
			cellRecords[cellID].particleN = cellParticleN;
			cellParticleN = 0;
			cellID = r.cellID;
		}
		++cellParticleN;
	}
	cellRecords[cellID].particleN = cellParticleN;
	swap(particlePos, particlePosTmp);
	swap(particleVel, particleVelTmp);
}
