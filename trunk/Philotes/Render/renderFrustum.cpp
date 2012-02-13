
#include "renderFrustum.h"
#include "renderNode.h"
#include "math/sphere.h"
#include "math/planeBoundedVolume.h"

#include "render.h"
#include "gearsPlatformUtil.h"

_NAMESPACE_BEGIN

const scalar RenderFrustum::INFINITE_FAR_PLANE_ADJUST = 0.00001f;
//-----------------------------------------------------------------------
RenderFrustum::RenderFrustum(const String& name) : 
	RenderElement(name),
	mProjType(PT_PERSPECTIVE), 
	mFOVy(Radian(Math::PI/4.0f)), 
	mFarDist(10000.0f), 
	mNearDist(0.1f), 
	mAspect(1.33333333333333f), 
	mOrthoHeight(1000),
	mFrustumOffset(Vector2::ZERO),
	mFocalLength(1.0f),
	mLastParentOrientation(Quaternion::IDENTITY),
	mLastParentPosition(Vector3::ZERO),
	mRecalcFrustum(true), 
	mRecalcView(true), 
	mRecalcFrustumPlanes(true),
	mRecalcWorldSpaceCorners(true),
	mRecalcVertexData(true),
	mCustomViewMatrix(false),
	mCustomProjMatrix(false),
	mFrustumExtentsManuallySet(false),
	mOrientationMode(OR_DEGREE_0),
	mReflect(false), 
	mObliqueDepthProjection(false)
{
	updateView();
	updateFrustum();
}

//-----------------------------------------------------------------------
RenderFrustum::~RenderFrustum()
{
	// Do nothing
}

//-----------------------------------------------------------------------
void RenderFrustum::setFOVy(const Radian& fov)
{
	mFOVy = fov;
	invalidateFrustum();
}

//-----------------------------------------------------------------------
const Radian& RenderFrustum::getFOVy(void) const
{
	return mFOVy;
}


//-----------------------------------------------------------------------
void RenderFrustum::setFarClipDistance(scalar farPlane)
{
	mFarDist = farPlane;
	invalidateFrustum();
}

//-----------------------------------------------------------------------
scalar RenderFrustum::getFarClipDistance(void) const
{
	return mFarDist;
}

//-----------------------------------------------------------------------
void RenderFrustum::setNearClipDistance(scalar nearPlane)
{
	if (nearPlane <= 0)
	{
		PH_EXCEPT(ERR_RENDER, "Near clip distance must be greater than zero.");
	}
	mNearDist = nearPlane;
	invalidateFrustum();
}

//-----------------------------------------------------------------------
scalar RenderFrustum::getNearClipDistance(void) const
{
	return mNearDist;
}

//---------------------------------------------------------------------
void RenderFrustum::setFrustumOffset(const Vector2& offset)
{
	mFrustumOffset = offset;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::setFrustumOffset(scalar horizontal, scalar vertical)
{
	setFrustumOffset(Vector2(horizontal, vertical));
}
//---------------------------------------------------------------------
const Vector2& RenderFrustum::getFrustumOffset() const
{
	return mFrustumOffset;
}
//---------------------------------------------------------------------
void RenderFrustum::setFocalLength(scalar focalLength)
{
	if (focalLength <= 0)
	{
		PH_EXCEPT(ERR_RENDER,"Focal length must be greater than zero.");
	}

	mFocalLength = focalLength;
	invalidateFrustum();
}
//---------------------------------------------------------------------
scalar RenderFrustum::getFocalLength() const
{
	return mFocalLength;
}
//-----------------------------------------------------------------------
const Matrix4& RenderFrustum::getProjectionMatrix(void) const
{
	updateFrustum();

	return mProjMatrix;
}
//-----------------------------------------------------------------------
const Matrix4& RenderFrustum::getProjectionMatrixWithRSDepth(void) const
{
	updateFrustum();

	return mProjMatrixRSDepth;
}
//-----------------------------------------------------------------------
const Matrix4& RenderFrustum::getProjectionMatrixRS(void) const
{
	updateFrustum();

	return mProjMatrixRS;
}
//-----------------------------------------------------------------------
const Matrix4& RenderFrustum::getViewMatrix(void) const
{
	updateView();

	return mViewMatrix;

}

//-----------------------------------------------------------------------
const Plane* RenderFrustum::getFrustumPlanes(void) const
{
	// Make any pending updates to the calculated frustum planes
	updateFrustumPlanes();

	return mFrustumPlanes;
}

//-----------------------------------------------------------------------
const Plane& RenderFrustum::getFrustumPlane(unsigned short plane) const
{
	// Make any pending updates to the calculated frustum planes
	updateFrustumPlanes();

	return mFrustumPlanes[plane];

}

//-----------------------------------------------------------------------
bool RenderFrustum::isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy) const
{
	// Null boxes always invisible
	if (bound.isNull()) return false;

	// Infinite boxes always visible
	if (bound.isInfinite()) return true;

	// Make any pending updates to the calculated frustum planes
	updateFrustumPlanes();

	// Get centre of the box
	Vector3 centre = bound.getCenter();
	// Get the half-size of the box
	Vector3 halfSize = bound.getHalfSize();

	// For each plane, see if all points are on the negative side
	// If so, object is not visible
	for (int plane = 0; plane < 6; ++plane)
	{
		// Skip far plane if infinite view frustum
		if (plane == FRUSTUM_PLANE_FAR && mFarDist == 0)
			continue;

		Plane::Side side = mFrustumPlanes[plane].getSide(centre, halfSize);
		if (side == Plane::NEGATIVE_SIDE)
		{
			// ALL corners on negative side therefore out of view
			if (culledBy)
				*culledBy = (FrustumPlane)plane;
			return false;
		}

	}

	return true;
}

//-----------------------------------------------------------------------
bool RenderFrustum::isVisible(const Vector3& vert, FrustumPlane* culledBy) const
{
	// Make any pending updates to the calculated frustum planes
	updateFrustumPlanes();

	// For each plane, see if all points are on the negative side
	// If so, object is not visible
	for (int plane = 0; plane < 6; ++plane)
	{
		// Skip far plane if infinite view frustum
		if (plane == FRUSTUM_PLANE_FAR && mFarDist == 0)
			continue;

		if (mFrustumPlanes[plane].getSide(vert) == Plane::NEGATIVE_SIDE)
		{
			// ALL corners on negative side therefore out of view
			if (culledBy)
				*culledBy = (FrustumPlane)plane;
			return false;
		}

	}

	return true;
}
//-----------------------------------------------------------------------
bool RenderFrustum::isVisible(const Sphere& sphere, FrustumPlane* culledBy) const
{
	// Make any pending updates to the calculated frustum planes
	updateFrustumPlanes();

	// For each plane, see if sphere is on negative side
	// If so, object is not visible
	for (int plane = 0; plane < 6; ++plane)
	{
		// Skip far plane if infinite view frustum
		if (plane == FRUSTUM_PLANE_FAR && mFarDist == 0)
			continue;

		// If the distance from sphere center to plane is negative, and 'more negative' 
		// than the radius of the sphere, sphere is outside frustum
		if (mFrustumPlanes[plane].getDistance(sphere.getCenter()) < -sphere.getRadius())
		{
			// ALL corners on negative side therefore out of view
			if (culledBy)
				*culledBy = (FrustumPlane)plane;
			return false;
		}

	}

	return true;
}
//-----------------------------------------------------------------------
void RenderFrustum::calcProjectionParameters(scalar& left, scalar& right, scalar& bottom, scalar& top) const
{ 
	if (mCustomProjMatrix)
	{
		// Convert clipspace corners to camera space
		Matrix4 invProj = mProjMatrix.inverse();
		Vector3 topLeft(-0.5f, 0.5f, 0.0f);
		Vector3 bottomRight(0.5f, -0.5f, 0.0f);

		topLeft = invProj * topLeft;
		bottomRight = invProj * bottomRight;

		left = topLeft.x;
		top = topLeft.y;
		right = bottomRight.x;
		bottom = bottomRight.y;

	}
	else
	{
		if (mFrustumExtentsManuallySet)
		{
			left = mLeft;
			right = mRight;
			top = mTop;
			bottom = mBottom;
		}
		// Calculate general projection parameters
		else if (mProjType == PT_PERSPECTIVE)
		{
			Radian thetaY (mFOVy * 0.5f);
			scalar tanThetaY = Math::Tan(thetaY);
			scalar tanThetaX = tanThetaY * mAspect;

			scalar nearFocal = mNearDist / mFocalLength;
			scalar nearOffsetX = mFrustumOffset.x * nearFocal;
			scalar nearOffsetY = mFrustumOffset.y * nearFocal;
			scalar half_w = tanThetaX * mNearDist;
			scalar half_h = tanThetaY * mNearDist;

			left   = - half_w + nearOffsetX;
			right  = + half_w + nearOffsetX;
			bottom = - half_h + nearOffsetY;
			top    = + half_h + nearOffsetY;

			mLeft = left;
			mRight = right;
			mTop = top;
			mBottom = bottom;
		}
		else
		{
			// Unknown how to apply frustum offset to orthographic camera, just ignore here
			scalar half_w = getOrthoWindowWidth() * 0.5f;
			scalar half_h = getOrthoWindowHeight() * 0.5f;

			left   = - half_w;
			right  = + half_w;
			bottom = - half_h;
			top    = + half_h;

			mLeft = left;
			mRight = right;
			mTop = top;
			mBottom = bottom;
		}

	}
}
//-----------------------------------------------------------------------
void RenderFrustum::updateFrustumImpl(void) const
{
	// Common calcs
	scalar left, right, bottom, top;

	calcProjectionParameters(left, right, bottom, top);

	if (!mCustomProjMatrix)
	{

		// The code below will dealing with general projection 
		// parameters, similar glFrustum and glOrtho.
		// Doesn't optimise manually except division operator, so the 
		// code more self-explaining.

		scalar inv_w = 1 / (right - left);
		scalar inv_h = 1 / (top - bottom);
		scalar inv_d = 1 / (mFarDist - mNearDist);

		// Recalc if frustum params changed
		if (mProjType == PT_PERSPECTIVE)
		{
			// Calc matrix elements
			scalar A = 2 * mNearDist * inv_w;
			scalar B = 2 * mNearDist * inv_h;
			scalar C = (right + left) * inv_w;
			scalar D = (top + bottom) * inv_h;
			scalar q, qn;
			if (mFarDist == 0)
			{
				// Infinite far plane
				q = RenderFrustum::INFINITE_FAR_PLANE_ADJUST - 1;
				qn = mNearDist * (RenderFrustum::INFINITE_FAR_PLANE_ADJUST - 2);
			}
			else
			{
				q = - (mFarDist + mNearDist) * inv_d;
				qn = -2 * (mFarDist * mNearDist) * inv_d;
			}

			// NB: This creates 'uniform' perspective projection matrix,
			// which depth range [-1,1], right-handed rules
			//
			// [ A   0   C   0  ]
			// [ 0   B   D   0  ]
			// [ 0   0   q   qn ]
			// [ 0   0   -1  0  ]
			//
			// A = 2 * near / (right - left)
			// B = 2 * near / (top - bottom)
			// C = (right + left) / (right - left)
			// D = (top + bottom) / (top - bottom)
			// q = - (far + near) / (far - near)
			// qn = - 2 * (far * near) / (far - near)

			mProjMatrix = Matrix4::ZERO;
			mProjMatrix[0][0] = A;
			mProjMatrix[0][2] = C;
			mProjMatrix[1][1] = B;
			mProjMatrix[1][2] = D;
			mProjMatrix[2][2] = q;
			mProjMatrix[2][3] = qn;
			mProjMatrix[3][2] = -1;

			if (mObliqueDepthProjection)
			{
				// Translate the plane into view space

				// Don't use getViewMatrix here, incase overrided by 
				// camera and return a cull frustum view matrix
				updateView();
				Plane plane = mViewMatrix * mObliqueProjPlane;

				// Thanks to Eric Lenyel for posting this calculation 
				// at www.terathon.com

				// Calculate the clip-space corner point opposite the 
				// clipping plane
				// as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
				// transform it into camera space by multiplying it
				// by the inverse of the projection matrix

				/* generalised version
				Vector4 q = matrix.inverse() * 
				Vector4(Math::Sign(plane.normal.x), 
				Math::Sign(plane.normal.y), 1.0f, 1.0f);
				*/
				Vector4 qVec;
				qVec.x = (Math::Sign(plane.normal.x) + mProjMatrix[0][2]) / mProjMatrix[0][0];
				qVec.y = (Math::Sign(plane.normal.y) + mProjMatrix[1][2]) / mProjMatrix[1][1];
				qVec.z = -1;
				qVec.w = (1 + mProjMatrix[2][2]) / mProjMatrix[2][3];

				// Calculate the scaled plane vector
				Vector4 clipPlane4d(plane.normal.x, plane.normal.y, plane.normal.z, plane.d);
				Vector4 c = clipPlane4d * (2 / (clipPlane4d.dotProduct(qVec)));

				// Replace the third row of the projection matrix
				mProjMatrix[2][0] = c.x;
				mProjMatrix[2][1] = c.y;
				mProjMatrix[2][2] = c.z + 1;
				mProjMatrix[2][3] = c.w; 
			}
		} // perspective
		else if (mProjType == PT_ORTHOGRAPHIC)
		{
			scalar A = 2 * inv_w;
			scalar B = 2 * inv_h;
			scalar C = - (right + left) * inv_w;
			scalar D = - (top + bottom) * inv_h;
			scalar q, qn;
			if (mFarDist == 0)
			{
				// Can not do infinite far plane here, avoid divided zero only
				q = - RenderFrustum::INFINITE_FAR_PLANE_ADJUST / mNearDist;
				qn = - RenderFrustum::INFINITE_FAR_PLANE_ADJUST - 1;
			}
			else
			{
				q = - 2 * inv_d;
				qn = - (mFarDist + mNearDist)  * inv_d;
			}

			// NB: This creates 'uniform' orthographic projection matrix,
			// which depth range [-1,1], right-handed rules
			//
			// [ A   0   0   C  ]
			// [ 0   B   0   D  ]
			// [ 0   0   q   qn ]
			// [ 0   0   0   1  ]
			//
			// A = 2 * / (right - left)
			// B = 2 * / (top - bottom)
			// C = - (right + left) / (right - left)
			// D = - (top + bottom) / (top - bottom)
			// q = - 2 / (far - near)
			// qn = - (far + near) / (far - near)

			mProjMatrix = Matrix4::ZERO;
			mProjMatrix[0][0] = A;
			mProjMatrix[0][3] = C;
			mProjMatrix[1][1] = B;
			mProjMatrix[1][3] = D;
			mProjMatrix[2][2] = q;
			mProjMatrix[2][3] = qn;
			mProjMatrix[3][3] = 1;
		} // ortho            
	} // !mCustomProjMatrix

	Render* renderSystem = GearPlatformUtil::getSingleton()->getRender();
	if (renderSystem)
	{
		renderSystem->convertProjectionMatrix(mProjMatrix, mProjMatrixRS);
		renderSystem->convertProjectionMatrix(mProjMatrix, mProjMatrixRSDepth);
	}

	// Calculate bounding box (local)
	// Box is from 0, down -Z, max dimensions as determined from far plane
	// If infinite view frustum just pick a far value
	scalar farDist = (mFarDist == 0) ? 100000 : mFarDist;
	// Near plane bounds
	Vector3 vmin(left, bottom, -farDist);
	Vector3 vmax(right, top, 0);

	if (mCustomProjMatrix)
	{
		// Some custom projection matrices can have unusual inverted settings
		// So make sure the AABB is the right way around to start with
		Vector3 tmp = vmin;
		vmin.makeFloor(vmax);
		vmax.makeCeil(tmp);
	}

	if (mProjType == PT_PERSPECTIVE)
	{
		// Merge with far plane bounds
		scalar radio = farDist / mNearDist;
		vmin.makeFloor(Vector3(left * radio, bottom * radio, -farDist));
		vmax.makeCeil(Vector3(right * radio, top * radio, 0));
	}
	mBoundingBox.setExtents(vmin, vmax);

	mRecalcFrustum = false;

	// Signal to update frustum clipping planes
	mRecalcFrustumPlanes = true;
}
//-----------------------------------------------------------------------
void RenderFrustum::updateFrustum(void) const
{
	if (isFrustumOutOfDate())
	{
		updateFrustumImpl();
	}
}

//-----------------------------------------------------------------------
bool RenderFrustum::isViewOutOfDate(void) const
{
	// Attached to node?
	if (m_parentNode)
	{
		if (mRecalcView ||
			m_parentNode->_getDerivedOrientation() != mLastParentOrientation ||
			m_parentNode->_getDerivedPosition() != mLastParentPosition)
		{
			// Ok, we're out of date with SceneNode we're attached to
			mLastParentOrientation = m_parentNode->_getDerivedOrientation();
			mLastParentPosition = m_parentNode->_getDerivedPosition();
			mRecalcView = true;
		}
	}
	// Deriving reflection from linked plane?
	// TODO : linked plane
// 	if (mLinkedReflectPlane && 
// 		!(mLastLinkedReflectionPlane == mLinkedReflectPlane->_getDerivedPlane()))
// 	{
// 		mReflectPlane = mLinkedReflectPlane->_getDerivedPlane();
// 		mReflectMatrix = Math::buildReflectionMatrix(mReflectPlane);
// 		mLastLinkedReflectionPlane = mLinkedReflectPlane->_getDerivedPlane();
// 		mRecalcView = true;
// 	}

	return mRecalcView;
}

//-----------------------------------------------------------------------
bool RenderFrustum::isFrustumOutOfDate(void) const
{
	// Deriving custom near plane from linked plane?
	if (mObliqueDepthProjection)
	{
		// Out of date when view out of data since plane needs to be in view space
		if (isViewOutOfDate())
		{
			mRecalcFrustum = true;
		}
		//// Update derived plane
		//if (mLinkedObliqueProjPlane && 
		//	!(mLastLinkedObliqueProjPlane == mLinkedObliqueProjPlane->_getDerivedPlane()))
		//{
		//	mObliqueProjPlane = mLinkedObliqueProjPlane->_getDerivedPlane();
		//	mLastLinkedObliqueProjPlane = mObliqueProjPlane;
		//	mRecalcFrustum = true;
		//}
	}

	return mRecalcFrustum;
}

//-----------------------------------------------------------------------
void RenderFrustum::updateViewImpl(void) const
{
	// ----------------------
	// Update the view matrix
	// ----------------------

	// Get orientation from quaternion

	if (!mCustomViewMatrix)
	{
		Matrix3 rot;
		const Quaternion& orientation = getOrientationForViewUpdate();
		const Vector3& position = getPositionForViewUpdate();

		mViewMatrix = Math::makeViewMatrix(position, orientation, mReflect? &mReflectMatrix : 0);
	}

	mRecalcView = false;

	// Signal to update frustum clipping planes
	mRecalcFrustumPlanes = true;
	// Signal to update world space corners
	mRecalcWorldSpaceCorners = true;
	// Signal to update frustum if oblique plane enabled,
	// since plane needs to be in view space
	if (mObliqueDepthProjection)
	{
		mRecalcFrustum = true;
	}
}
//---------------------------------------------------------------------
void RenderFrustum::calcViewMatrixRelative(const Vector3& relPos, Matrix4& matToUpdate) const
{
	Matrix4 matTrans = Matrix4::IDENTITY;
	matTrans.setTrans(relPos);
	matToUpdate = getViewMatrix() * matTrans;

}
//-----------------------------------------------------------------------
void RenderFrustum::updateView(void) const
{
	if (isViewOutOfDate())
	{
		updateViewImpl();
	}
}

//-----------------------------------------------------------------------
void RenderFrustum::updateFrustumPlanesImpl(void) const
{
	// -------------------------
	// Update the frustum planes
	// -------------------------
	Matrix4 combo = mProjMatrix * mViewMatrix;

	mFrustumPlanes[FRUSTUM_PLANE_LEFT].normal.x = combo[3][0] + combo[0][0];
	mFrustumPlanes[FRUSTUM_PLANE_LEFT].normal.y = combo[3][1] + combo[0][1];
	mFrustumPlanes[FRUSTUM_PLANE_LEFT].normal.z = combo[3][2] + combo[0][2];
	mFrustumPlanes[FRUSTUM_PLANE_LEFT].d = combo[3][3] + combo[0][3];

	mFrustumPlanes[FRUSTUM_PLANE_RIGHT].normal.x = combo[3][0] - combo[0][0];
	mFrustumPlanes[FRUSTUM_PLANE_RIGHT].normal.y = combo[3][1] - combo[0][1];
	mFrustumPlanes[FRUSTUM_PLANE_RIGHT].normal.z = combo[3][2] - combo[0][2];
	mFrustumPlanes[FRUSTUM_PLANE_RIGHT].d = combo[3][3] - combo[0][3];

	mFrustumPlanes[FRUSTUM_PLANE_TOP].normal.x = combo[3][0] - combo[1][0];
	mFrustumPlanes[FRUSTUM_PLANE_TOP].normal.y = combo[3][1] - combo[1][1];
	mFrustumPlanes[FRUSTUM_PLANE_TOP].normal.z = combo[3][2] - combo[1][2];
	mFrustumPlanes[FRUSTUM_PLANE_TOP].d = combo[3][3] - combo[1][3];

	mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].normal.x = combo[3][0] + combo[1][0];
	mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].normal.y = combo[3][1] + combo[1][1];
	mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].normal.z = combo[3][2] + combo[1][2];
	mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].d = combo[3][3] + combo[1][3];

	mFrustumPlanes[FRUSTUM_PLANE_NEAR].normal.x = combo[3][0] + combo[2][0];
	mFrustumPlanes[FRUSTUM_PLANE_NEAR].normal.y = combo[3][1] + combo[2][1];
	mFrustumPlanes[FRUSTUM_PLANE_NEAR].normal.z = combo[3][2] + combo[2][2];
	mFrustumPlanes[FRUSTUM_PLANE_NEAR].d = combo[3][3] + combo[2][3];

	mFrustumPlanes[FRUSTUM_PLANE_FAR].normal.x = combo[3][0] - combo[2][0];
	mFrustumPlanes[FRUSTUM_PLANE_FAR].normal.y = combo[3][1] - combo[2][1];
	mFrustumPlanes[FRUSTUM_PLANE_FAR].normal.z = combo[3][2] - combo[2][2];
	mFrustumPlanes[FRUSTUM_PLANE_FAR].d = combo[3][3] - combo[2][3];

	// Renormalise any normals which were not unit length
	for(int i=0; i<6; i++ ) 
	{
		scalar length = mFrustumPlanes[i].normal.normalise();
		mFrustumPlanes[i].d /= length;
	}

	mRecalcFrustumPlanes = false;
}
//-----------------------------------------------------------------------
void RenderFrustum::updateFrustumPlanes(void) const
{
	updateView();
	updateFrustum();

	if (mRecalcFrustumPlanes)
	{
		updateFrustumPlanesImpl();
	}
}
//-----------------------------------------------------------------------
void RenderFrustum::updateWorldSpaceCornersImpl(void) const
{
	Matrix4 eyeToWorld = mViewMatrix.inverseAffine();

	// Note: Even though we can dealing with general projection matrix here,
	//       but because it's incompatibly with infinite far plane, thus, we
	//       still need to working with projection parameters.

	// Calc near plane corners
	scalar nearLeft, nearRight, nearBottom, nearTop;
	calcProjectionParameters(nearLeft, nearRight, nearBottom, nearTop);

	// Treat infinite fardist as some arbitrary far value
	scalar farDist = (mFarDist == 0) ? 100000 : mFarDist;

	// Calc far palne corners
	scalar radio = mProjType == PT_PERSPECTIVE ? farDist / mNearDist : 1;
	scalar farLeft = nearLeft * radio;
	scalar farRight = nearRight * radio;
	scalar farBottom = nearBottom * radio;
	scalar farTop = nearTop * radio;

	// near
	mWorldSpaceCorners[0] = eyeToWorld.transformAffine(Vector3(nearRight, nearTop,    -mNearDist));
	mWorldSpaceCorners[1] = eyeToWorld.transformAffine(Vector3(nearLeft,  nearTop,    -mNearDist));
	mWorldSpaceCorners[2] = eyeToWorld.transformAffine(Vector3(nearLeft,  nearBottom, -mNearDist));
	mWorldSpaceCorners[3] = eyeToWorld.transformAffine(Vector3(nearRight, nearBottom, -mNearDist));
	// far
	mWorldSpaceCorners[4] = eyeToWorld.transformAffine(Vector3(farRight,  farTop,     -farDist));
	mWorldSpaceCorners[5] = eyeToWorld.transformAffine(Vector3(farLeft,   farTop,     -farDist));
	mWorldSpaceCorners[6] = eyeToWorld.transformAffine(Vector3(farLeft,   farBottom,  -farDist));
	mWorldSpaceCorners[7] = eyeToWorld.transformAffine(Vector3(farRight,  farBottom,  -farDist));


	mRecalcWorldSpaceCorners = false;
}
//-----------------------------------------------------------------------
void RenderFrustum::updateWorldSpaceCorners(void) const
{
	updateView();

	if (mRecalcWorldSpaceCorners)
	{
		updateWorldSpaceCornersImpl();
	}

}

//-----------------------------------------------------------------------
scalar RenderFrustum::getAspectRatio(void) const
{
	return mAspect;
}

//-----------------------------------------------------------------------
void RenderFrustum::setAspectRatio(scalar r)
{
	mAspect = r;
	invalidateFrustum();
}

//-----------------------------------------------------------------------
const AxisAlignedBox& RenderFrustum::getBoundingBox(void) const
{
	return mBoundingBox;
}
//-----------------------------------------------------------------------
scalar RenderFrustum::getBoundingRadius(void) const
{
	return (mFarDist == 0)? 100000 : mFarDist;
}
// -------------------------------------------------------------------
void RenderFrustum::invalidateFrustum() const
{
	mRecalcFrustum = true;
	mRecalcFrustumPlanes = true;
	mRecalcWorldSpaceCorners = true;
	mRecalcVertexData = true;
}
// -------------------------------------------------------------------
void RenderFrustum::invalidateView() const
{
	mRecalcView = true;
	mRecalcFrustumPlanes = true;
	mRecalcWorldSpaceCorners = true;
}
// -------------------------------------------------------------------
const Vector3* RenderFrustum::getWorldSpaceCorners(void) const
{
	updateWorldSpaceCorners();

	return mWorldSpaceCorners;
}
//-----------------------------------------------------------------------
void RenderFrustum::setProjectionType(ProjectionType pt)
{
	mProjType = pt;
	invalidateFrustum();
}

//-----------------------------------------------------------------------
ProjectionType RenderFrustum::getProjectionType(void) const
{
	return mProjType;
}
//-----------------------------------------------------------------------
const Vector3& RenderFrustum::getPositionForViewUpdate(void) const
{
	return mLastParentPosition;
}
//-----------------------------------------------------------------------
const Quaternion& RenderFrustum::getOrientationForViewUpdate(void) const
{
	return mLastParentOrientation;
}
//-----------------------------------------------------------------------
void RenderFrustum::enableReflection(const Plane& p)
{
	mReflect = true;
	mReflectPlane = p;
	mReflectMatrix = Math::buildReflectionMatrix(p);
	invalidateView();

}
//-----------------------------------------------------------------------
void RenderFrustum::disableReflection(void)
{
	mReflect = false;
	invalidateView();
}
//---------------------------------------------------------------------
bool RenderFrustum::projectSphere(const Sphere& sphere, 
							scalar* left, scalar* top, scalar* right, scalar* bottom) const
{
	// See http://www.gamasutra.com/features/20021011/lengyel_06.htm
	// Transform light position into camera space

	updateView();
	Vector3 eyeSpacePos = mViewMatrix.transformAffine(sphere.getCenter());

	// initialise
	*left = *bottom = -1.0f;
	*right = *top = 1.0f;

	if (eyeSpacePos.z < 0)
	{
		updateFrustum();
		const Matrix4& projMatrix = getProjectionMatrix();
		scalar r = sphere.getRadius();
		scalar rsq = r * r;

		// early-exit
		if (eyeSpacePos.squaredLength() <= rsq)
			return false;

		scalar Lxz = Math::Sqr(eyeSpacePos.x) + Math::Sqr(eyeSpacePos.z);
		scalar Lyz = Math::Sqr(eyeSpacePos.y) + Math::Sqr(eyeSpacePos.z);

		// Find the tangent planes to the sphere
		// XZ first
		// calculate quadratic discriminant: b*b - 4ac
		// x = Nx
		// a = Lx^2 + Lz^2
		// b = -2rLx
		// c = r^2 - Lz^2
		scalar a = Lxz;
		scalar b = -2.0f * r * eyeSpacePos.x;
		scalar c = rsq - Math::Sqr(eyeSpacePos.z);
		scalar D = b*b - 4.0f*a*c;

		// two roots?
		if (D > 0)
		{
			scalar sqrootD = Math::Sqrt(D);
			// solve the quadratic to get the components of the normal
			scalar Nx0 = (-b + sqrootD) / (2 * a);
			scalar Nx1 = (-b - sqrootD) / (2 * a);

			// Derive Z from this
			scalar Nz0 = (r - Nx0 * eyeSpacePos.x) / eyeSpacePos.z;
			scalar Nz1 = (r - Nx1 * eyeSpacePos.x) / eyeSpacePos.z;

			// Get the point of tangency
			// Only consider points of tangency in front of the camera
			scalar Pz0 = (Lxz - rsq) / (eyeSpacePos.z - ((Nz0 / Nx0) * eyeSpacePos.x));
			if (Pz0 < 0)
			{
				// Project point onto near plane in worldspace
				scalar nearx0 = (Nz0 * mNearDist) / Nx0;
				// now we need to map this to viewport coords
				// use projection matrix since that will take into account all factors
				Vector3 relx0 = projMatrix * Vector3(nearx0, 0, -mNearDist);

				// find out whether this is a left side or right side
				scalar Px0 = -(Pz0 * Nz0) / Nx0;
				if (Px0 > eyeSpacePos.x)
				{
					*right = Math::Min(*right, relx0.x);
				}
				else
				{
					*left = Math::Max(*left, relx0.x);
				}
			}
			scalar Pz1 = (Lxz - rsq) / (eyeSpacePos.z - ((Nz1 / Nx1) * eyeSpacePos.x));
			if (Pz1 < 0)
			{
				// Project point onto near plane in worldspace
				scalar nearx1 = (Nz1 * mNearDist) / Nx1;
				// now we need to map this to viewport coords
				// use projection matrix since that will take into account all factors
				Vector3 relx1 = projMatrix * Vector3(nearx1, 0, -mNearDist);

				// find out whether this is a left side or right side
				scalar Px1 = -(Pz1 * Nz1) / Nx1;
				if (Px1 > eyeSpacePos.x)
				{
					*right = Math::Min(*right, relx1.x);
				}
				else
				{
					*left = Math::Max(*left, relx1.x);
				}
			}
		}


		// Now YZ 
		// calculate quadratic discriminant: b*b - 4ac
		// x = Ny
		// a = Ly^2 + Lz^2
		// b = -2rLy
		// c = r^2 - Lz^2
		a = Lyz;
		b = -2.0f * r * eyeSpacePos.y;
		c = rsq - Math::Sqr(eyeSpacePos.z);
		D = b*b - 4.0f*a*c;

		// two roots?
		if (D > 0)
		{
			scalar sqrootD = Math::Sqrt(D);
			// solve the quadratic to get the components of the normal
			scalar Ny0 = (-b + sqrootD) / (2 * a);
			scalar Ny1 = (-b - sqrootD) / (2 * a);

			// Derive Z from this
			scalar Nz0 = (r - Ny0 * eyeSpacePos.y) / eyeSpacePos.z;
			scalar Nz1 = (r - Ny1 * eyeSpacePos.y) / eyeSpacePos.z;

			// Get the point of tangency
			// Only consider points of tangency in front of the camera
			scalar Pz0 = (Lyz - rsq) / (eyeSpacePos.z - ((Nz0 / Ny0) * eyeSpacePos.y));
			if (Pz0 < 0)
			{
				// Project point onto near plane in worldspace
				scalar neary0 = (Nz0 * mNearDist) / Ny0;
				// now we need to map this to viewport coords
				// use projection matriy since that will take into account all factors
				Vector3 rely0 = projMatrix * Vector3(0, neary0, -mNearDist);

				// find out whether this is a top side or bottom side
				scalar Py0 = -(Pz0 * Nz0) / Ny0;
				if (Py0 > eyeSpacePos.y)
				{
					*top = Math::Min(*top, rely0.y);
				}
				else
				{
					*bottom = Math::Max(*bottom, rely0.y);
				}
			}
			scalar Pz1 = (Lyz - rsq) / (eyeSpacePos.z - ((Nz1 / Ny1) * eyeSpacePos.y));
			if (Pz1 < 0)
			{
				// Project point onto near plane in worldspace
				scalar neary1 = (Nz1 * mNearDist) / Ny1;
				// now we need to map this to viewport coords
				// use projection matriy since that will take into account all factors
				Vector3 rely1 = projMatrix * Vector3(0, neary1, -mNearDist);

				// find out whether this is a top side or bottom side
				scalar Py1 = -(Pz1 * Nz1) / Ny1;
				if (Py1 > eyeSpacePos.y)
				{
					*top = Math::Min(*top, rely1.y);
				}
				else
				{
					*bottom = Math::Max(*bottom, rely1.y);
				}
			}
		}
	}

	return (*left != -1.0f) || (*top != 1.0f) || (*right != 1.0f) || (*bottom != -1.0f);

}
//---------------------------------------------------------------------
void RenderFrustum::enableCustomNearClipPlane(const Plane& plane)
{
	mObliqueDepthProjection = true;
	mObliqueProjPlane = plane;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::disableCustomNearClipPlane(void)
{
	mObliqueDepthProjection = false;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::setCustomViewMatrix(bool enable, const Matrix4& viewMatrix)
{
	mCustomViewMatrix = enable;
	if (enable)
	{
		assert(viewMatrix.isAffine());
		mViewMatrix = viewMatrix;
	}
	invalidateView();
}
//---------------------------------------------------------------------
void RenderFrustum::setCustomProjectionMatrix(bool enable, const Matrix4& projMatrix)
{
	mCustomProjMatrix = enable;
	if (enable)
	{
		mProjMatrix = projMatrix;
	}
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::setOrthoWindow(scalar w, scalar h)
{
	mOrthoHeight = h;
	mAspect = w / h;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::setOrthoWindowHeight(scalar h)
{
	mOrthoHeight = h;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::setOrthoWindowWidth(scalar w)
{
	mOrthoHeight = w / mAspect;
	invalidateFrustum();
}
//---------------------------------------------------------------------
scalar RenderFrustum::getOrthoWindowHeight() const
{
	return mOrthoHeight;
}
//---------------------------------------------------------------------
scalar RenderFrustum::getOrthoWindowWidth() const
{
	return mOrthoHeight * mAspect;	
}
//---------------------------------------------------------------------
void RenderFrustum::setFrustumExtents(scalar left, scalar right, scalar top, scalar bottom)
{
	mFrustumExtentsManuallySet = true;
	mLeft = left;
	mRight = right;
	mTop = top;
	mBottom = bottom;

	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::resetFrustumExtents()
{
	mFrustumExtentsManuallySet = false;
	invalidateFrustum();
}
//---------------------------------------------------------------------
void RenderFrustum::getFrustumExtents(scalar& outleft, scalar& outright, scalar& outtop, scalar& outbottom) const
{
	updateFrustum();
	outleft = mLeft;
	outright = mRight;
	outtop = mTop;
	outbottom = mBottom;
}
//---------------------------------------------------------------------
PlaneBoundedVolume RenderFrustum::getPlaneBoundedVolume()
{
	updateFrustumPlanes();

	PlaneBoundedVolume volume;
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_NEAR]);
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_FAR]);
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_BOTTOM]);
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_TOP]);
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_LEFT]);
	volume.planes.push_back(mFrustumPlanes[FRUSTUM_PLANE_RIGHT]);
	return volume;
}
//---------------------------------------------------------------------
void RenderFrustum::setOrientationMode(OrientationMode orientationMode)
{
	mOrientationMode = orientationMode;
	invalidateFrustum();
}
//---------------------------------------------------------------------
OrientationMode RenderFrustum::getOrientationMode() const
{
	return mOrientationMode;
}

_NAMESPACE_END