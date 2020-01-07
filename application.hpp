//----------------------------------------------------------------------------------------
/**
 * \file       application.hpp
 * \author     Ales Koblizek
 * \date       2020/01/07
 * \brief      Application class
*/
//----------------------------------------------------------------------------------------
#ifndef APPLICATION_HPP_20_01_07_21_40_12
#define APPLICATION_HPP_20_01_07_21_40_12 
#include <memory>
#include "sph.hpp"

/// Draw everything, call update, calculate avg frameTime
class Application {
	public:
		Application(std::unique_ptr<SPH> &&sph, const Bounds &_bounds);

		void reset();
		void draw();
		void update();

		Bounds b;
		GLuint shader;
		vec3 cameraPos;
		glm::mat4x4 camera;
		std::unique_ptr<SPH> sph;
		Material material;

		float avgFrameTime;
		unsigned frameTimeN;
};

#endif /* APPLICATION_HPP_20_01_07_21_40_12 */
