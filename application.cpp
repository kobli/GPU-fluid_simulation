#include "application.hpp"

Application::Application(std::unique_ptr<SPH> &&sph, const Bounds &_bounds): b{_bounds}, sph{std::move(sph)}, avgFrameTime{0}, frameTimeN{0} {
	cameraPos = {-2,2,.5};
	glm::mat4x4 cameraView = lookAt(cameraPos, (b.max-b.min)/2.f, UP);
	glm::mat4x4 projection = glm::perspective(70., 1., 0.1, 1000.);
	camera = projection*cameraView;
	shader = loadShaderProgram({
			std::make_tuple(GL_VERTEX_SHADER, "shaders/pt.vert"),
			std::make_tuple(GL_FRAGMENT_SHADER, "shaders/cameraLight.frag"),
			});
	if(!shader)
		std::cerr << "Failed to load shader program\n";

	material.ambientK = .5;
	material.diffuseK = .5;
	material.color = {0.3,0.3,1,1};
}

void Application::reset() {
	sph.reset();
}

void Application::draw() {
	glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );

	glUseProgram(shader);
	setUniform(shader, camera, "ViewProject");
	GLint diffuseKLoc = glGetUniformLocation(shader, "Mat.diffuseK");
	glUniform1f(diffuseKLoc, material.diffuseK);
	GLint ambientKLoc = glGetUniformLocation(shader, "Mat.ambientK");
	glUniform1f(ambientKLoc, material.ambientK);
	GLint colorLoc = glGetUniformLocation(shader, "Mat.color");
	vec4 bboxColor(1,1,1,1);
	glUniform4fv(colorLoc, 1, &bboxColor[0]);
	GLint camPosLoc = glGetUniformLocation(shader, "CameraPos");
	glUniform3fv(camPosLoc, 1, &cameraPos[0]);

	glVertexAttrib3f(2, 0, 0, 0);

	b.draw();
	glUniform4fv(colorLoc, 1, &material.color[0]);
	sph->draw();
}

void Application::update() {
	sph->update();
	if(frameTimeN == 20)
		frameTimeN = 4;
	avgFrameTime = (avgFrameTime*frameTimeN + sph->frameTime) / (frameTimeN + 1);
	frameTimeN++;
}
