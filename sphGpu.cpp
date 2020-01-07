#include "sphGpu.hpp"
using namespace std;
const unsigned localGroupSize = 1024;

SPHgpu::SPHgpu(SPHconfig config, Bounds& _b): SPH{config, _b}, queryActive{false} {
	glGenQueries(1, &queryID);
	updateProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SPHupdate.comp")});
	densityProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SPHdensity.comp")});
	particleRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/ParticleRec.comp")});
	sortParticleRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/SortParticleRec.comp")});
	cellRecProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/CellRec.comp")});
	particleReorderProgram = loadShaderProgram({make_tuple(GL_COMPUTE_SHADER, "shaders/ParticleReorder.comp")});
	glGenBuffers(1, &particlePositionBuffOut);
	glGenBuffers(1, &particleVelocityBuff);
	glGenBuffers(1, &particleVelocityBuffOut);
	glGenBuffers(1, &densityBuff);
	glGenBuffers(1, &particleRecBuffer);
	glGenBuffers(1, &cellRecBuffer);
	reset();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, densityBuff);
	glBufferData(GL_SHADER_STORAGE_BUFFER, config.particleN*sizeof(float), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlePositionBuffOut);
	glBufferData(GL_SHADER_STORAGE_BUFFER, config.particleN*sizeof(vec4), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleRecBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, config.particleN*sizeof(ParticleRecord), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cellRecBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, config.SubdivisionN*config.SubdivisionN*config.SubdivisionN*sizeof(CellRecord), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleVelocityBuffOut);
	glBufferData(GL_SHADER_STORAGE_BUFFER, config.particleN*sizeof(vec4), NULL, GL_DYNAMIC_COPY);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, densityBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particleVelocityBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particleRecBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, cellRecBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, particleVelocityBuffOut);

	setParticlePositionAttrBuffer(4);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void SPHgpu::reset() {
	vector<vec4> particlePos(config.particleN);
	vector<vec4> particleVel(config.particleN);
	for(vec4& p : particlePos)
		p = vec4(b.min + (b.max-b.min)*vec3(frand(), frand(), frand()), 0);
	for(vec4& v : particleVel)
		v = normalize(vec4(rand(), rand(), rand(), 0));
	bufferData(particlePositionBuff, particlePos, GL_DYNAMIC_COPY);
	bufferData(particleVelocityBuff, particleVel, GL_DYNAMIC_COPY);
}

void SPHgpu::update() {
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

void SPHgpu::step() {
	// prepare NN data structure (uniform grid)
	// calculate particle record for each particle (particle ID, cell ID)
	glUseProgram(particleRecProgram);
	setConfigUniforms(particleRecProgram);
	glDispatchCompute(config.particleN/localGroupSize, 1, 1); // one invocation per particle
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// sort the records by cellID
	glUseProgram(sortParticleRecProgram);
	for(size_t l = 2; l < 2*config.particleN; l *= 2) {
		for(size_t seqLen = l; seqLen > 1; seqLen /= 2) {
			glUniform1ui(glGetUniformLocation(sortParticleRecProgram, "seqLen"), seqLen);
			glUniform1ui(glGetUniformLocation(sortParticleRecProgram, "blockLen"), l);
			glDispatchCompute(config.particleN/localGroupSize, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
	}

	// update cell records (first_particle_rec, particle_rec_n)
	glUseProgram(cellRecProgram);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// reorder particle position and velocity according to the order of particleRecords
	glUseProgram(particleReorderProgram);
	glDispatchCompute(config.particleN/localGroupSize, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	swap(particlePositionBuff, particlePositionBuffOut);
	swap(particleVelocityBuff, particleVelocityBuffOut);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particleVelocityBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, particleVelocityBuffOut);

	// compute density at each particle's location
	glUseProgram(densityProgram);
	setConfigUniforms(densityProgram);
	glDispatchCompute(config.particleN/localGroupSize, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// update particle positions and velocities
	glUseProgram(updateProgram);
	setConfigUniforms(updateProgram);
	glDispatchCompute(config.particleN/localGroupSize, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	swap(particlePositionBuff, particlePositionBuffOut);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlePositionBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particlePositionBuffOut);
	setParticlePositionAttrBuffer(4);
	swap(particleVelocityBuff, particleVelocityBuffOut);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particleVelocityBuff);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, particleVelocityBuffOut);
}

void SPHgpu::setConfigUniforms(GLuint program) {
	glUniform1f(glGetUniformLocation(program, "Step"), config.Step);
	glUniform1f(glGetUniformLocation(program, "H"), config.H);
	glUniform1f(glGetUniformLocation(program, "M"), config.M);
	glUniform1f(glGetUniformLocation(program, "Rho0"), config.Rho0);
	glUniform1f(glGetUniformLocation(program, "K"), config.K);
	glUniform1f(glGetUniformLocation(program, "Mu"), config.Mu);
	glUniform1ui(glGetUniformLocation(program, "SubdivisionN"), config.SubdivisionN);
	glUniform3fv(glGetUniformLocation(program, "boundsMin"), 1, &b.min[0]);
	glUniform3fv(glGetUniformLocation(program, "boundsMax"), 1, &b.max[0]);
}
