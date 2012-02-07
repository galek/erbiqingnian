
#include <RendererProjection.h>

const scalar RendererProjection::INFINITE_FAR_PLANE_ADJUST = 0.00001f;

void RendererProjection::makeProjectionMatrix( const Radian& fovy, scalar aspect, 
											  scalar nearPlane, scalar farPlane, Matrix4& dest )
{
	Radian theta ( fovy * 0.5 );
	scalar h = 1 / MathMisc::Tan(theta);
	scalar w = h / aspect;
	scalar q, qn;
	if (farPlane == 0)
	{
		q = 1 - RendererProjection::INFINITE_FAR_PLANE_ADJUST;
		qn = nearPlane * (RendererProjection::INFINITE_FAR_PLANE_ADJUST - 1);
	}
	else
	{
		q = farPlane / ( farPlane - nearPlane );
		qn = -q * nearPlane;
	}

	dest = Matrix4::ZERO;
	dest[0][0] = w;
	dest[1][1] = h;

	dest[2][2] = -q;
	dest[3][2] = -1.0f;

	dest[2][3] = qn;
}

void RendererProjection::makeProjectionMatrix( scalar left, scalar right, scalar bottom,
											  scalar top, scalar nearPlane, scalar farPlane, Matrix4& dest )
{
	scalar width = right - left;
	scalar height = top - bottom;
	scalar q, qn;
	if (farPlane == 0)
	{
		q = 1 - RendererProjection::INFINITE_FAR_PLANE_ADJUST;
		qn = nearPlane * (RendererProjection::INFINITE_FAR_PLANE_ADJUST - 1);
	}
	else
	{
		q = farPlane / ( farPlane - nearPlane );
		qn = -q * nearPlane;
	}
	dest = Matrix4::ZERO;
	dest[0][0] = 2 * nearPlane / width;
	dest[0][2] = (right+left) / width;
	dest[1][1] = 2 * nearPlane / height;
	dest[1][2] = (top+bottom) / height;
	dest[2][2] = -q;
	dest[3][2] = -1.0f;

	dest[2][3] = qn;
}

void RendererProjection::buildProjectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view)
{
	Matrix4 projView = Matrix4(view * proj).inverse();
	memcpy(dst, projView[0], sizeof(float)*16);
}

void RendererProjection::buildUnprojectMatrix(float *dst, const Matrix4 &proj, const Matrix4 &view)
{
	Matrix4 invProjView = Matrix4(view * proj).inverse();
	memcpy(dst, invProjView[0], sizeof(float)*16);
}
