
#ifndef RENDERER_PROJECTION_H
#define RENDERER_PROJECTION_H

#include <renderConfig.h>

// Õ∂”∞æÿ’Û

_NAMESPACE_BEGIN

class RenderProjection
{
	public:

		static void makeProjectionMatrix(const Radian& fovy, scalar aspectRatio, 
			scalar nearPlane, scalar farPlane, Matrix4& outMatrix);

		static void makeProjectionMatrix(scalar left, scalar right, scalar bottom, 
			scalar top, scalar near, scalar far, Matrix4& outMatrix);

		static void	buildProjectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view);

		static void	buildUnprojectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view);

		static const scalar INFINITE_FAR_PLANE_ADJUST;

};

_NAMESPACE_END

#endif
