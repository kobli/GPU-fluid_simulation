//----------------------------------------------------------------------------------------
/**
 * \file       sphKernels.hpp
 * \author     Ales Koblizek
 * \date       2020/01/07
 * \brief      Kernels used for the SPH
*/
//----------------------------------------------------------------------------------------
#ifndef SPHKERNELS_HPP_20_01_07_21_10_08
#define SPHKERNELS_HPP_20_01_07_21_10_08 
#include <glm/glm.hpp>

inline double w(glm::vec3 r, double h) {
	double rLen = length(r);
	if(rLen >= 0 && rLen <= h)
		return 315./(64.*M_PI*pow(h,9)) * pow(h*h - rLen*rLen, 3);
	return 0;
}

inline glm::vec3 wPresure1(glm::vec3 r, double h) {
	double rLen = length(r);
	if(rLen > 0 && rLen <= h)
		return r * float(1./rLen * 45./(M_PI*pow(h,6)) * pow(h-rLen, 2));
	return {};
}

inline double wViscosity2(glm::vec3 r, double h) {
	double rLen = length(r);
	if(rLen >= 0 && rLen <= h)
		return 45./(M_PI*pow(h,6))*(h-rLen);
	return 0;
}
#endif /* SPHKERNELS_HPP_20_01_07_21_10_08 */
