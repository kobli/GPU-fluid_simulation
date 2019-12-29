#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include "utils.hpp"

using namespace std;
using namespace glm;

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 1024

const unsigned localGroupSize = 1024;
const unsigned ParticleN = localGroupSize*8; // must be power of two
const float ParticleRad = 0.02;

float Step = 0.005; // [seconds]
float H = 0.1;
float M = 32;
float Rho0 = 1;
float K = 2.4;
float Mu = 2048;
const unsigned SubdivisionN = 8; // must be power of two
const vec3 BoxSize{2,2,2};


struct Material {
	vec4 color;
	float ambientK;
	float diffuseK;
	float specularK;
	float shininess;
};

float frand() {
	return float(rand())/RAND_MAX;
}

double w(vec3 r, double h) {
	double rLen = length(r);
	if(rLen >= 0 && rLen <= h)
		return 315./(64.*M_PI*pow(h,9)) * pow(h*h - rLen*rLen, 3);
	return 0;
}

vec3 wPresure1(vec3 r, double h) {
	double rLen = length(r);
	if(rLen > 0 && rLen <= h)
		return r * float(1./rLen * 45./(M_PI*pow(h,6)) * pow(h-rLen, 2));
	return {};
}

double wViscosity2(vec3 r, double h) {
	double rLen = length(r);
	if(rLen >= 0 && rLen <= h)
		return 45./(M_PI*pow(h,6))*(h-rLen);
	return 0;
}

class Bounds {
	public:
		Bounds(vec3 _size): min{0,0,0}, max{_size} {
			transform = scale(identity<mat4x4>(), _size);
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			vector<vec3> vertices = {
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
			vector<GLuint> lineIndices = {
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

		void draw() {
			glBindVertexArray(vao);
			GLuint program;
			glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
			setUniform(program, transform, "Model");
			glDrawElements(GL_LINES, 2*12, GL_UNSIGNED_INT, 0);
		}

		bool isOutside(const vec3 &p, vec3 &n) {
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

		vec3 min;
		vec3 max;
		mat4x4 transform;
		GLuint vao;
		GLuint vbo;
		GLuint vboLineIndices;
};

struct ParticleRecord {
	unsigned cellID;
	unsigned particleID;
};

struct CellRecord {
	unsigned firstParticleID;
	unsigned particleN;
};

class SPH {
	public:
		SPH(Bounds& _b): frameTime{0}, b{_b} {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			initSphereMesh();

			glGenBuffers(1, &particlePositionBuff);
			setParticlePositionAttrBuffer(3);
		}

		void draw() {
			glBindVertexArray(vao);
			GLuint program;
			glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&program);
			mat4x4 transform = scale(identity<mat4x4>(), vec3(ParticleRad, ParticleRad, ParticleRad));
			setUniform(program, transform, "Model");
			setUniform(program, transpose(inverse(transform)), "ModelInvT");

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
			glDrawElementsInstanced(GL_QUADS, indexN, GL_UNSIGNED_INT, NULL, ParticleN);
		}

		void setParticlePositionAttrBuffer(int numPositionComponents) {
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuff);
			glVertexAttribPointer(2, numPositionComponents, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(2);
			glVertexAttribDivisor(2, 1);
		}

		virtual void reset() = 0;
		virtual void update() = 0;

	protected:
		template <typename T>
		void bufferData(GLuint buffer, const vector<T>& data, GLenum usage) {
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(T), data.data(), usage);
		}

	private:
		void initSphereMesh() {
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

	public:
		float frameTime;

	protected:
		GLuint particlePositionBuff;
		Bounds &b;

	private:
		GLuint vao;
		GLuint vbo;
		GLuint vboNormals;
		GLuint vboIndices;
		unsigned indexN;
};

class SPHgpu: public SPH {
	public:
		SPHgpu(unsigned /*n*/, Bounds& _b): SPH{_b} {
			glGenQueries(1, &queryID);
			updateProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SPHupdate.comp")});
			densityProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SPHdensity.comp")});
			particleRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/ParticleRec.comp")});
			sortParticleRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SortParticleRec.comp")});
			cellRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/CellRec.comp")});
			particleReorderProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/ParticleReorder.comp")});
			glGenBuffers(1, &particlePositionBuffOut);
			glGenBuffers(1, &particleVelocityBuff);
			glGenBuffers(1, &particleVelocityBuff2);
			glGenBuffers(1, &densityBuff);
			glGenBuffers(1, &particleRecBuffer);
			glGenBuffers(1, &cellRecBuffer);
			reset();
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, densityBuff);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleN*sizeof(float), NULL, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlePositionBuffOut);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleN*sizeof(vec4), NULL, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleRecBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleN*sizeof(ParticleRecord), NULL, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cellRecBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, SubdivisionN*SubdivisionN*SubdivisionN*sizeof(CellRecord), NULL, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleVelocityBuff2);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleN*sizeof(vec4), NULL, GL_DYNAMIC_COPY);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, densityBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particleVelocityBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particleRecBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, cellRecBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, particleVelocityBuff2);

			setParticlePositionAttrBuffer(4);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		void reset() override {
			vector<vec4> particlePos(ParticleN);
			vector<vec4> particleVel(ParticleN);
			for(vec4& p : particlePos)
				p = vec4(b.min + (b.max-b.min)*vec3(frand(), frand(), frand()), 0);
			for(vec4& v : particleVel)
				v = normalize(vec4(rand(), rand(), rand(), 0));
			bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_COPY);
			bufferData(particleVelocityBuff, particleVel, GL_DYNAMIC_COPY);
		}

		void update() override {
			GLuint64 qr = GL_FALSE;
			if(queryActive)
				glGetQueryObjectui64v(queryID, GL_QUERY_RESULT_AVAILABLE, &qr);
			bool queryResultReady = (qr == GL_TRUE);
			if(queryResultReady) {
				glGetQueryObjectui64v(queryID, GL_QUERY_RESULT, &qr);
				frameTime = double(qr)/1000/1000;
				queryActive = false;
			}
			if(!queryActive) {
				glBeginQuery(GL_TIME_ELAPSED, queryID);
				step();
				glEndQuery(GL_TIME_ELAPSED);
				queryActive = true;
			}
			else
				step();
		}

	private:
		void step() {
			// prepare NN data structure (uniform grid)
			// calculate particle record for each particle (particle ID, cell ID)
			glUseProgram(particleRecProgram);
			setConfigUniforms(particleRecProgram);
			glDispatchCompute(ParticleN/localGroupSize, 1, 1); // one invocation per particle
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//vector<ParticleRecord> particleRec(ParticleN);
			//glGetNamedBufferSubData(particleRecBuffer, 0, particleRec.size()*sizeof(ParticleRecord), particleRec.data());
			
			// sort the records by cellID
			glUseProgram(sortParticleRecProgram);
			for(size_t l = 2; l < 2*ParticleN; l *= 2) {
				for(size_t seqLen = l; seqLen > 1; seqLen /= 2) {
					glUniform1ui(glGetUniformLocation(sortParticleRecProgram, "seqLen"), seqLen);
					glUniform1ui(glGetUniformLocation(sortParticleRecProgram, "blockLen"), l);
					glDispatchCompute(ParticleN/localGroupSize, 1, 1);
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}
			}
			//vector<ParticleRecord> particleRec(ParticleN);
			//glGetNamedBufferSubData(particleRecBuffer, 0, particleRec.size()*sizeof(ParticleRecord), particleRec.data());
			
			// update cell records (first_particle_rec, particle_rec_n)
			glUseProgram(cellRecProgram);
			glDispatchCompute(SubdivisionN*SubdivisionN*SubdivisionN/localGroupSize+1, 1, 1); // one invocation per cell 
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//vector<CellRecord> cellRec(SubdivisionN*SubdivisionN*SubdivisionN);
			//glGetNamedBufferSubData(cellRecBuffer, 0, cellRec.size()*sizeof(CellRecord), cellRec.data());
			
			// reorder particle position and velocity according to the order of particleRecords
			glUseProgram(particleReorderProgram);
			glDispatchCompute(ParticleN/localGroupSize, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			swap(particlePositionBuff, particlePositionBuffOut);
			swap(particleVelocityBuff, particleVelocityBuff2);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particleVelocityBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, particleVelocityBuff2);

			// compute density at each particle's location
			glUseProgram(densityProgram);
			setConfigUniforms(densityProgram);
			glDispatchCompute(ParticleN/localGroupSize, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//vector<float> density(ParticleN);
			//glGetNamedBufferSubData(densityBuff, 0, density.size()*sizeof(float), density.data());

			// update particle positions and velocities
			glUseProgram(updateProgram);
			setConfigUniforms(updateProgram);
			glDispatchCompute(ParticleN/localGroupSize, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			//vector<vec4> pos(ParticleN);
			//glGetNamedBufferSubData(particlePositionBuffOut, 0, pos.size()*sizeof(vec4), pos.data());
			//vector<vec4> vel(ParticleN);
			//glGetNamedBufferSubData(particleVelocityBuff, 0, vel.size()*sizeof(vec4), vel.data());

			swap(particlePositionBuff, particlePositionBuffOut);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
			setParticlePositionAttrBuffer(4);
		}
		
		void setConfigUniforms(GLuint program) {
			glUniform1f(glGetUniformLocation(program, "Step"), Step);
			glUniform1f(glGetUniformLocation(program, "H"), H);
			glUniform1f(glGetUniformLocation(program, "M"), M);
			glUniform1f(glGetUniformLocation(program, "Rho0"), Rho0);
			glUniform1f(glGetUniformLocation(program, "K"), K);
			glUniform1f(glGetUniformLocation(program, "Mu"), Mu);
			glUniform1ui(glGetUniformLocation(program, "SubdivisionN"), SubdivisionN);
			glUniform3fv(glGetUniformLocation(program, "boundsMin"), 1, b.min.data.data);
			glUniform3fv(glGetUniformLocation(program, "boundsMax"), 1, b.max.data.data);
		}

	private:
		GLuint queryID;
		bool queryActive;
		GLuint particleVelocityBuff;
		GLuint particleVelocityBuff2;
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

class SPHcpu: public SPH {
	public:
		SPHcpu(unsigned n, Bounds& _b): SPH{_b} {
			particlePosTmp.resize(n);
			particleVel.resize(n,{});
			particlePos.resize(n);
			reset();
			cellRecords.resize(SubdivisionN*SubdivisionN*SubdivisionN);
			particleRecords.resize(ParticleN);
		}

		void reset() override {
			for(unsigned i = 0; i < particlePos.size(); ++i) {
				particlePos[i] = b.min + (b.max-b.min)*vec3(frand(), frand(), frand());
				particleVel[i] = normalize(vec3(rand(), rand(), rand()));
			}
			bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_DRAW);
		}

		void update() override {
			updateCellRecords();
			// calculate density and presure at each particle position
			vector<float> density(particlePos.size(), M);
			vector<float> pressure(particlePos.size(), 0);
			for(unsigned i = 0; i < particlePos.size(); ++i) {
				vector<unsigned> neighbourCells = nnCells(particlePos[i]);
				for(unsigned cellID : neighbourCells) {
					CellRecord r = cellRecords[cellID];
					for(unsigned j = r.firstParticleID; j < r.firstParticleID+r.particleN; ++j) {
						density[i] += M*w(particlePos[i]-particlePos[j], H);
					}
				}
				pressure[i] = K*(density[i]-Rho0);
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
						vec3 fp = M*(pressure[i]+pressure[j])/(2*density[j])*wPresure1(particlePos[i]-particlePos[j], H);
						vec3 fv = (particleVel[j]-particleVel[i])*float(Mu*M/density[j]*wViscosity2(particlePos[i]-particlePos[j], H));
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
				particlePosTmp[i] = particlePos[i] + particleVel[i]*Step;
				// update speed using the computed acceleration
				particleVel[i] += a*Step;
			}
			swap(particlePos, particlePosTmp);
			collide();

			bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_DRAW);
		}

		void collide() {
			for(unsigned i = 0; i < particlePos.size(); ++i) {
				vec3 surfaceNormal;
				float velL = length(particleVel[i]);
				if(b.isOutside(particlePos[i], surfaceNormal) && velL > 0 && !isinf(velL)) {
					//if(surfaceNormal != vec3(0,-1,0)) // open-topped box
					{
						particlePos[i] += -particleVel[i]*Step;
						vec3 d = -particleVel[i]/velL;
						particleVel[i] = (2*dot(d, surfaceNormal)*surfaceNormal-d)*velL;
					}
				}
			}
		}

		unsigned particlePosToCellID(const vec3& particlePos) {
			vec3 c = (particlePos - b.min) / (b.max - b.min) * float(SubdivisionN);
			return cellPosToID(c);
		}

		unsigned cellPosToID(const vec3& c) {
			return unsigned(c.x)*SubdivisionN*SubdivisionN + unsigned(c.y)*SubdivisionN + unsigned(c.z);
		}

		vector<unsigned> nnCells(const vec3& particlePos) {
			using std::max;
			using std::min;
			vector<unsigned> r;
			r.reserve(3*3*3);
			vec3 c = ((particlePos - b.min) / (b.max - b.min)) * float(SubdivisionN);
			int cx = c.x;
			int cy = c.y;
			int cz = c.z;
			const int maxC = int(SubdivisionN)-1;
			for(int x = max(0, cx-1); x <= min(maxC, cx+1); ++x) {
				for(int y = max(0, cy-1); y <= min(maxC, cy+1); ++y) {
					for(int z = max(0, cz-1); z <= min(maxC, cz+1); ++z) {
						r.push_back(cellPosToID(vec3(x,y,z)));
					}
				}
			}
			return r;
		}

		void updateCellRecords() {
			for(CellRecord& r : cellRecords)
				r.particleN = 0;
			// for each particle: calculate cell coordinates -> hash
			for(unsigned i = 0; i < ParticleN; ++i) {
				particleRecords[i].particleID = i;
				particleRecords[i].cellID = particlePosToCellID(particlePos[i]);
			}
			// sort particleRecords by cell_id (this will form clusters for each cell)
			sort(particleRecords.begin(), particleRecords.end(), [](ParticleRecord& l, ParticleRecord& r){ return l.cellID < r.cellID; });
			// for each cell_id_hash create cell_rec (first_particle_rec, particle_rec_n)
			// reorder particle array according to particle_rec
			unsigned cellID = particleRecords[0].cellID;
			unsigned cellParticleN = 0;
			cellRecords[cellID].firstParticleID = 0;
			vector<vec3> particleVelTmp(ParticleN);
			for(unsigned i = 0; i < ParticleN; ++i) {
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

	private:
		vector<vec3> particlePos;
		vector<vec3> particleVel;
		vector<vec3> particlePosTmp;

		vector<CellRecord> cellRecords;
		vector<ParticleRecord> particleRecords;
};

class Application {
	public:
		Application(): b{BoxSize}, sph{ParticleN, b}, avgFrameTime{0}, frameTimeN{0} {
			cameraPos = {-2,2,.5};
			mat4x4 cameraView = lookAt(cameraPos, BoxSize/2.f, UP);
			mat4x4 projection = perspective(70., 1., 0.1, 1000.);
			camera = projection*cameraView;
			shader = loadShaderProgram({
					make_tuple(GL_VERTEX_SHADER, "shaders/pt.vert"),
					make_tuple(GL_FRAGMENT_SHADER, "shaders/cameraLight.frag"),
					});
			if(!shader)
				cerr << "Failed to load shader program\n";

			material.ambientK = .5;
			material.diffuseK = .5;
			material.color = {0.3,0.3,1,1};
		}

		void reset() {
			sph.reset();
		}

		void draw() {
			glClearColor( 0.0, 0.0, 0.0, 1.0 );
			glClear( GL_COLOR_BUFFER_BIT );

			glUseProgram(shader);
			setUniform(shader, camera, "ViewProject");
			GLint diffuseKLoc = glGetUniformLocation(shader, "Mat.diffuseK");
			glUniform1f(diffuseKLoc, material.diffuseK);
			GLint ambientKLoc = glGetUniformLocation(shader, "Mat.ambientK");
			glUniform1f(ambientKLoc, material.ambientK);
			GLint colorLoc = glGetUniformLocation(shader, "Mat.color");
			glUniform4fv(colorLoc, 1, material.color.data.data);
			GLint camPosLoc = glGetUniformLocation(shader, "CameraPos");
			glUniform3fv(camPosLoc, 1, cameraPos.data.data);

			glVertexAttrib3f(2, 0, 0, 0);

			b.draw();
			sph.draw();
		}

		void update() {
			sph.update();
			if(frameTimeN != unsigned(-1)) {
				avgFrameTime = (avgFrameTime*frameTimeN + sph.frameTime) / (frameTimeN + 1);
				frameTimeN++;
			}
		}

		Bounds b;
		GLuint shader;
		vec3 cameraPos;
		mat4x4 camera;
		SPHgpu sph;
		Material material;

		float avgFrameTime;
		unsigned frameTimeN;
};
unique_ptr<Application> app;

void DrawImage( void ) {
	app->draw();
  glutSwapBuffers();
}
static void HandleKeys(unsigned char key, int x, int y) {
  switch (key) {
    case 27:	// ESC
      exit(0);
			break;
		case 'r':
			app->reset();
			break;
		case 's':
			Step /= 2;
			break;
		case 'S':
			Step *= 2;
			break;
		case 'h':
			H /= 2;
			break;
		case 'H':
			H *= 2;
			break;
		case 'm':
			M /= 2;
			break;
		case 'M':
			M *= 2;
			break;
		case 'k':
			K /= 2;
			break;
		case 'K':
			K *= 2;
			break;
		case 'u':
			Mu /= 2;
			break;
		case 'U':
			Mu *= 2;
			break;
		case 'o':
			Rho0 /= 2;
			break;
		case 'O':
			Rho0 *= 2;
			break;
  }
	cout << "step: " << Step << endl;
	cout << "h: " << H << endl;
	cout << "m: " << M << endl;
	cout << "k: " << K << endl;
	cout << "mu: " << Mu << endl;
	cout << "rho0: " << Rho0 << endl;
	cout << endl;
}
void idleFunc() {
	app->update();
	ostringstream title;
	title << "SPH demo - avg frame time: " << std::fixed << setw(8) << setprecision(2) << app->avgFrameTime << " [ms]";
	glutSetWindowTitle(title.str().c_str());
  glutPostRedisplay();
}

int main(int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutInitWindowSize(IMAGE_WIDTH, IMAGE_HEIGHT);
  glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
  glutCreateWindow("SPH demo");

	if(glewInit()) {
		cerr << "Cannot initialize GLEW\n";
		exit(EXIT_FAILURE);
	}
	if(glDebugMessageCallback){
		cout << "Register OpenGL debug callback " << endl;
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglCallbackFunction, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);
		glEnable(GL_DEBUG_OUTPUT);
	}
	else
		cout << "glDebugMessageCallback not available" << endl;

  glutDisplayFunc(DrawImage);
  glutKeyboardFunc(HandleKeys);
  glutIdleFunc(idleFunc);

	app.reset(new Application);
  glutMainLoop();
  return 0;
}
