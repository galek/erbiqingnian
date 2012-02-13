
#include "renderCamera.h"
#include "renderNode.h"
#include "math/ray.h"

_NAMESPACE_BEGIN

RenderCamera::RenderCamera( const String& name)
	: RenderFrustum(name),
	mOrientation(Quaternion::IDENTITY),
	mPosition(Vector3::ZERO),
	mSceneDetail(PM_SOLID),
	mWindowSet(false),
	mAutoAspectRatio(false),
	mCullFrustum(0),
	mUseRenderingDistance(true),
	mUseMinPixelSize(false),
	mPixelDisplayRatio(0)
{

	// Reasonable defaults to camera params
	mFOVy = Radian(Math::PI/4.0f);
	mNearDist = 100.0f;
	mFarDist = 100000.0f;
	mAspect = 1.33333333333333f;
	mProjType = PT_PERSPECTIVE;
	setFixedYawAxis(true);    // Default to fixed yaw, like freelook since most people expect this

	invalidateFrustum();
	invalidateView();

	// Init matrices
	mViewMatrix = Matrix4::ZERO;
	mProjMatrixRS = Matrix4::ZERO;

	m_parentNode = 0;

	// no reflection
	mReflect = false;
}

//-----------------------------------------------------------------------
RenderCamera::~RenderCamera()
{
}

//-----------------------------------------------------------------------
void RenderCamera::setPolygonMode(PolygonMode sd)
{
	mSceneDetail = sd;
}

//-----------------------------------------------------------------------
PolygonMode RenderCamera::getPolygonMode(void) const
{
	return mSceneDetail;
}

//-----------------------------------------------------------------------
void RenderCamera::setPosition(scalar x, scalar y, scalar z)
{
	mPosition.x = x;
	mPosition.y = y;
	mPosition.z = z;
	invalidateView();
}

//-----------------------------------------------------------------------
void RenderCamera::setPosition(const Vector3& vec)
{
	mPosition = vec;
	invalidateView();
}

//-----------------------------------------------------------------------
const Vector3& RenderCamera::getPosition(void) const
{
	return mPosition;
}

//-----------------------------------------------------------------------
void RenderCamera::move(const Vector3& vec)
{
	mPosition = mPosition + vec;
	invalidateView();
}

//-----------------------------------------------------------------------
void RenderCamera::moveRelative(const Vector3& vec)
{
	// Transform the axes of the relative vector by camera's local axes
	Vector3 trans = mOrientation * vec;

	mPosition = mPosition + trans;
	invalidateView();
}

//-----------------------------------------------------------------------
void RenderCamera::setDirection(scalar x, scalar y, scalar z)
{
	setDirection(Vector3(x,y,z));
}

//-----------------------------------------------------------------------
void RenderCamera::setDirection(const Vector3& vec)
{
	// Do nothing if given a zero vector
	// (Replaced assert since this could happen with auto tracking camera and
	//  camera passes through the lookAt point)
	if (vec == Vector3::ZERO) return;

	// Remember, camera points down -Z of local axes!
	// Therefore reverse direction of direction vector before determining local Z
	Vector3 zAdjustVec = -vec;
	zAdjustVec.normalise();

	Quaternion targetWorldOrientation;


	if( mYawFixed )
	{
		Vector3 xVec = mYawFixedAxis.crossProduct( zAdjustVec );
		xVec.normalise();

		Vector3 yVec = zAdjustVec.crossProduct( xVec );
		yVec.normalise();

		targetWorldOrientation.FromAxes( xVec, yVec, zAdjustVec );
	}
	else
	{

		// Get axes from current quaternion
		Vector3 axes[3];
		updateView();
		mRealOrientation.ToAxes(axes);
		Quaternion rotQuat;
		if ( (axes[2]+zAdjustVec).squaredLength() <  0.00005f) 
		{
			// Oops, a 180 degree turn (infinite possible rotation axes)
			// Default to yaw i.e. use current UP
			rotQuat.FromAngleAxis(Radian(Math::PI), axes[1]);
		}
		else
		{
			// Derive shortest arc to new direction
			rotQuat = axes[2].getRotationTo(zAdjustVec);

		}
		targetWorldOrientation = rotQuat * mRealOrientation;
	}

	// transform to parent space
	if (m_parentNode)
	{
		mOrientation = m_parentNode->_getDerivedOrientation().Inverse() * targetWorldOrientation;
	}
	else
	{
		mOrientation = targetWorldOrientation;
	}

	// TODO If we have a fixed yaw axis, we mustn't break it by using the
	// shortest arc because this will sometimes cause a relative yaw
	// which will tip the camera

	invalidateView();

}

//-----------------------------------------------------------------------
Vector3 RenderCamera::getDirection(void) const
{
	// Direction points down -Z by default
	return mOrientation * -Vector3::UNIT_Z;
}

//-----------------------------------------------------------------------
Vector3 RenderCamera::getUp(void) const
{
	return mOrientation * Vector3::UNIT_Y;
}

//-----------------------------------------------------------------------
Vector3 RenderCamera::getRight(void) const
{
	return mOrientation * Vector3::UNIT_X;
}

//-----------------------------------------------------------------------
void RenderCamera::lookAt(const Vector3& targetPoint)
{
	updateView();
	this->setDirection(targetPoint - mRealPosition);
}

//-----------------------------------------------------------------------
void RenderCamera::lookAt( scalar x, scalar y, scalar z )
{
	Vector3 vTemp( x, y, z );
	this->lookAt(vTemp);
}

//-----------------------------------------------------------------------
void RenderCamera::roll(const Radian& angle)
{
	// Rotate around local Z axis
	Vector3 zAxis = mOrientation * Vector3::UNIT_Z;
	rotate(zAxis, angle);

	invalidateView();
}

//-----------------------------------------------------------------------
void RenderCamera::yaw(const Radian& angle)
{
	Vector3 yAxis;

	if (mYawFixed)
	{
		// Rotate around fixed yaw axis
		yAxis = mYawFixedAxis;
	}
	else
	{
		// Rotate around local Y axis
		yAxis = mOrientation * Vector3::UNIT_Y;
	}

	rotate(yAxis, angle);

	invalidateView();
}

//-----------------------------------------------------------------------
void RenderCamera::pitch(const Radian& angle)
{
	// Rotate around local X axis
	Vector3 xAxis = mOrientation * Vector3::UNIT_X;
	rotate(xAxis, angle);

	invalidateView();

}

//-----------------------------------------------------------------------
void RenderCamera::rotate(const Vector3& axis, const Radian& angle)
{
	Quaternion q;
	q.FromAngleAxis(angle,axis);
	rotate(q);
}

//-----------------------------------------------------------------------
void RenderCamera::rotate(const Quaternion& q)
{
	// Note the order of the mult, i.e. q comes after

	// Normalise the quat to avoid cumulative problems with precision
	Quaternion qnorm = q;
	qnorm.normalise();
	mOrientation = qnorm * mOrientation;

	invalidateView();

}

//-----------------------------------------------------------------------
bool RenderCamera::isViewOutOfDate(void) const
{
	// Overridden from Frustum to use local orientation / position offsets
	// Attached to node?
	if (m_parentNode != 0)
	{
		if (mRecalcView ||
			m_parentNode->_getDerivedOrientation() != mLastParentOrientation ||
			m_parentNode->_getDerivedPosition() != mLastParentPosition)
		{
			// Ok, we're out of date with SceneNode we're attached to
			mLastParentOrientation = m_parentNode->_getDerivedOrientation();
			mLastParentPosition = m_parentNode->_getDerivedPosition();
			mRealOrientation = mLastParentOrientation * mOrientation;
			mRealPosition = (mLastParentOrientation * mPosition) + mLastParentPosition;
			mRecalcView = true;
			mRecalcWindow = true;
		}
	}
	else
	{
		// Rely on own updates
		mRealOrientation = mOrientation;
		mRealPosition = mPosition;
	}

	// Deriving reflected orientation / position
	if (mRecalcView)
	{
		if (mReflect)
		{
			// Calculate reflected orientation, use up-vector as fallback axis.
			Vector3 dir = mRealOrientation * Vector3::NEGATIVE_UNIT_Z;
			Vector3 rdir = dir.reflect(mReflectPlane.normal);
			Vector3 up = mRealOrientation * Vector3::UNIT_Y;
			mDerivedOrientation = dir.getRotationTo(rdir, up) * mRealOrientation;

			// Calculate reflected position.
			mDerivedPosition = mReflectMatrix.transformAffine(mRealPosition);
		}
		else
		{
			mDerivedOrientation = mRealOrientation;
			mDerivedPosition = mRealPosition;
		}
	}

	return mRecalcView;

}

// -------------------------------------------------------------------
void RenderCamera::invalidateView() const
{
	mRecalcWindow = true;
	RenderFrustum::invalidateView();
}
// -------------------------------------------------------------------
void RenderCamera::invalidateFrustum(void) const
{
	mRecalcWindow = true;
	RenderFrustum::invalidateFrustum();
}
//-----------------------------------------------------------------------
void RenderCamera::setFixedYawAxis(bool useFixed, const Vector3& fixedAxis)
{
	mYawFixed = useFixed;
	mYawFixedAxis = fixedAxis;
}
//-----------------------------------------------------------------------
const Quaternion& RenderCamera::getOrientation(void) const
{
	return mOrientation;
}

//-----------------------------------------------------------------------
void RenderCamera::setOrientation(const Quaternion& q)
{
	mOrientation = q;
	mOrientation.normalise();
	invalidateView();
}
//-----------------------------------------------------------------------
const Quaternion& RenderCamera::getDerivedOrientation(void) const
{
	updateView();
	return mDerivedOrientation;
}
//-----------------------------------------------------------------------
const Vector3& RenderCamera::getDerivedPosition(void) const
{
	updateView();
	return mDerivedPosition;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getDerivedDirection(void) const
{
	// Direction points down -Z
	updateView();
	return mDerivedOrientation * Vector3::NEGATIVE_UNIT_Z;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getDerivedUp(void) const
{
	updateView();
	return mDerivedOrientation * Vector3::UNIT_Y;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getDerivedRight(void) const
{
	updateView();
	return mDerivedOrientation * Vector3::UNIT_X;
}
//-----------------------------------------------------------------------
const Quaternion& RenderCamera::getRealOrientation(void) const
{
	updateView();
	return mRealOrientation;
}
//-----------------------------------------------------------------------
const Vector3& RenderCamera::getRealPosition(void) const
{
	updateView();
	return mRealPosition;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getRealDirection(void) const
{
	// Direction points down -Z
	updateView();
	return mRealOrientation * Vector3::NEGATIVE_UNIT_Z;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getRealUp(void) const
{
	updateView();
	return mRealOrientation * Vector3::UNIT_Y;
}
//-----------------------------------------------------------------------
Vector3 RenderCamera::getRealRight(void) const
{
	updateView();
	return mRealOrientation * Vector3::UNIT_X;
}
//-----------------------------------------------------------------------
void RenderCamera::getWorldTransforms(Matrix4* mat) const 
{
	updateView();

	Vector3 scale(1.0, 1.0, 1.0);
	if (m_parentNode)
		scale = m_parentNode->_getDerivedScale();

	mat->makeTransform(
		mDerivedPosition,
		scale,
		mDerivedOrientation);
}
//-----------------------------------------------------------------------
Ray RenderCamera::getCameraToViewportRay(scalar screenX, scalar screenY) const
{
	Ray ret;
	getCameraToViewportRay(screenX, screenY, &ret);
	return ret;
}
//---------------------------------------------------------------------
void RenderCamera::getCameraToViewportRay(scalar screenX, scalar screenY, Ray* outRay) const
{
	Matrix4 inverseVP = (getProjectionMatrix() * getViewMatrix(true)).inverse();

	scalar nx = (2.0f * screenX) - 1.0f;
	scalar ny = 1.0f - (2.0f * screenY);
	Vector3 nearPoint(nx, ny, -1.f);
	// Use midPoint rather than far point to avoid issues with infinite projection
	Vector3 midPoint (nx, ny,  0.0f);

	// Get ray origin and ray target on near plane in world space
	Vector3 rayOrigin, rayTarget;

	rayOrigin = inverseVP * nearPoint;
	rayTarget = inverseVP * midPoint;

	Vector3 rayDirection = rayTarget - rayOrigin;
	rayDirection.normalise();

	outRay->setOrigin(rayOrigin);
	outRay->setDirection(rayDirection);
} 
//---------------------------------------------------------------------
PlaneBoundedVolume RenderCamera::getCameraToViewportBoxVolume(scalar screenLeft, 
														scalar screenTop, scalar screenRight, scalar screenBottom, bool includeFarPlane)
{
	PlaneBoundedVolume vol;
	getCameraToViewportBoxVolume(screenLeft, screenTop, screenRight, screenBottom, 
		&vol, includeFarPlane);
	return vol;

}
//---------------------------------------------------------------------()
void RenderCamera::getCameraToViewportBoxVolume(scalar screenLeft, 
										  scalar screenTop, scalar screenRight, scalar screenBottom, 
										  PlaneBoundedVolume* outVolume, bool includeFarPlane)
{
	outVolume->planes.clear();

	if (mProjType == PT_PERSPECTIVE)
	{

		// Use the corner rays to generate planes
		Ray ul = getCameraToViewportRay(screenLeft, screenTop);
		Ray ur = getCameraToViewportRay(screenRight, screenTop);
		Ray bl = getCameraToViewportRay(screenLeft, screenBottom);
		Ray br = getCameraToViewportRay(screenRight, screenBottom);


		Vector3 normal;
		// top plane
		normal = ul.getDirection().crossProduct(ur.getDirection());
		normal.normalise();
		outVolume->planes.push_back(
			Plane(normal, getDerivedPosition()));

		// right plane
		normal = ur.getDirection().crossProduct(br.getDirection());
		normal.normalise();
		outVolume->planes.push_back(
			Plane(normal, getDerivedPosition()));

		// bottom plane
		normal = br.getDirection().crossProduct(bl.getDirection());
		normal.normalise();
		outVolume->planes.push_back(
			Plane(normal, getDerivedPosition()));

		// left plane
		normal = bl.getDirection().crossProduct(ul.getDirection());
		normal.normalise();
		outVolume->planes.push_back(
			Plane(normal, getDerivedPosition()));

	}
	else
	{
		// ortho planes are parallel to frustum planes

		Ray ul = getCameraToViewportRay(screenLeft, screenTop);
		Ray br = getCameraToViewportRay(screenRight, screenBottom);

		updateFrustumPlanes();
		outVolume->planes.push_back(
			Plane(mFrustumPlanes[FRUSTUM_PLANE_TOP].normal, ul.getOrigin()));
		outVolume->planes.push_back(
			Plane(mFrustumPlanes[FRUSTUM_PLANE_RIGHT].normal, br.getOrigin()));
		outVolume->planes.push_back(
			Plane(mFrustumPlanes[FRUSTUM_PLANE_BOTTOM].normal, br.getOrigin()));
		outVolume->planes.push_back(
			Plane(mFrustumPlanes[FRUSTUM_PLANE_LEFT].normal, ul.getOrigin()));


	}

	// near & far plane applicable to both projection types
	outVolume->planes.push_back(getFrustumPlane(FRUSTUM_PLANE_NEAR));
	if (includeFarPlane)
		outVolume->planes.push_back(getFrustumPlane(FRUSTUM_PLANE_FAR));
}
// -------------------------------------------------------------------
void RenderCamera::setWindow (scalar Left, scalar Top, scalar Right, scalar Bottom)
{
	mWLeft = Left;
	mWTop = Top;
	mWRight = Right;
	mWBottom = Bottom;

	mWindowSet = true;
	mRecalcWindow = true;
}
// -------------------------------------------------------------------
void RenderCamera::resetWindow ()
{
	mWindowSet = false;
}
// -------------------------------------------------------------------
void RenderCamera::setWindowImpl() const
{
	if (!mWindowSet || !mRecalcWindow)
		return;

	// Calculate general projection parameters
	scalar vpLeft, vpRight, vpBottom, vpTop;
	calcProjectionParameters(vpLeft, vpRight, vpBottom, vpTop);

	scalar vpWidth = vpRight - vpLeft;
	scalar vpHeight = vpTop - vpBottom;

	scalar wvpLeft   = vpLeft + mWLeft * vpWidth;
	scalar wvpRight  = vpLeft + mWRight * vpWidth;
	scalar wvpTop    = vpTop - mWTop * vpHeight;
	scalar wvpBottom = vpTop - mWBottom * vpHeight;

	Vector3 vp_ul (wvpLeft, wvpTop, -mNearDist);
	Vector3 vp_ur (wvpRight, wvpTop, -mNearDist);
	Vector3 vp_bl (wvpLeft, wvpBottom, -mNearDist);
	Vector3 vp_br (wvpRight, wvpBottom, -mNearDist);

	Matrix4 inv = mViewMatrix.inverseAffine();

	Vector3 vw_ul = inv.transformAffine(vp_ul);
	Vector3 vw_ur = inv.transformAffine(vp_ur);
	Vector3 vw_bl = inv.transformAffine(vp_bl);
	Vector3 vw_br = inv.transformAffine(vp_br);

	mWindowClipPlanes.Clear();
	if (mProjType == PT_PERSPECTIVE)
	{
		Vector3 position = getPositionForViewUpdate();
		mWindowClipPlanes.Append(Plane(position, vw_bl, vw_ul));
		mWindowClipPlanes.Append(Plane(position, vw_ul, vw_ur));
		mWindowClipPlanes.Append(Plane(position, vw_ur, vw_br));
		mWindowClipPlanes.Append(Plane(position, vw_br, vw_bl));
	}
	else
	{
		Vector3 x_axis(inv[0][0], inv[0][1], inv[0][2]);
		Vector3 y_axis(inv[1][0], inv[1][1], inv[1][2]);
		x_axis.normalise();
		y_axis.normalise();
		mWindowClipPlanes.Append(Plane( x_axis, vw_bl));
		mWindowClipPlanes.Append(Plane(-x_axis, vw_ur));
		mWindowClipPlanes.Append(Plane( y_axis, vw_bl));
		mWindowClipPlanes.Append(Plane(-y_axis, vw_ur));
	}

	mRecalcWindow = false;

}
// -------------------------------------------------------------------
const Array<Plane>& RenderCamera::getWindowPlanes(void) const
{
	updateView();
	setWindowImpl();
	return mWindowClipPlanes;
}
// -------------------------------------------------------------------
scalar RenderCamera::getBoundingRadius(void) const
{
	// return a little bigger than the near distance
	// just to keep things just outside
	return mNearDist * 1.5f;

}
//-----------------------------------------------------------------------
const Vector3& RenderCamera::getPositionForViewUpdate(void) const
{
	// Note no update, because we're calling this from the update!
	return mRealPosition;
}
//-----------------------------------------------------------------------
const Quaternion& RenderCamera::getOrientationForViewUpdate(void) const
{
	return mRealOrientation;
}
//-----------------------------------------------------------------------
bool RenderCamera::getAutoAspectRatio(void) const
{
	return mAutoAspectRatio;
}
//-----------------------------------------------------------------------
void RenderCamera::setAutoAspectRatio(bool autoratio)
{
	mAutoAspectRatio = autoratio;
}
//-----------------------------------------------------------------------
bool RenderCamera::isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->isVisible(bound, culledBy);
	}
	else
	{
		return RenderFrustum::isVisible(bound, culledBy);
	}
}
//-----------------------------------------------------------------------
bool RenderCamera::isVisible(const Sphere& bound, FrustumPlane* culledBy) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->isVisible(bound, culledBy);
	}
	else
	{
		return RenderFrustum::isVisible(bound, culledBy);
	}
}
//-----------------------------------------------------------------------
bool RenderCamera::isVisible(const Vector3& vert, FrustumPlane* culledBy) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->isVisible(vert, culledBy);
	}
	else
	{
		return RenderFrustum::isVisible(vert, culledBy);
	}
}
//-----------------------------------------------------------------------
const Vector3* RenderCamera::getWorldSpaceCorners(void) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->getWorldSpaceCorners();
	}
	else
	{
		return RenderFrustum::getWorldSpaceCorners();
	}
}
//-----------------------------------------------------------------------
const Plane& RenderCamera::getFrustumPlane( unsigned short plane ) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->getFrustumPlane(plane);
	}
	else
	{
		return RenderFrustum::getFrustumPlane(plane);
	}
}
//-----------------------------------------------------------------------
bool RenderCamera::projectSphere(const Sphere& sphere, 
						   scalar* left, scalar* top, scalar* right, scalar* bottom) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->projectSphere(sphere, left, top, right, bottom);
	}
	else
	{
		return RenderFrustum::projectSphere(sphere, left, top, right, bottom);
	}
}
//-----------------------------------------------------------------------
scalar RenderCamera::getNearClipDistance(void) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->getNearClipDistance();
	}
	else
	{
		return RenderFrustum::getNearClipDistance();
	}
}
//-----------------------------------------------------------------------
scalar RenderCamera::getFarClipDistance(void) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->getFarClipDistance();
	}
	else
	{
		return RenderFrustum::getFarClipDistance();
	}
}
//-----------------------------------------------------------------------
const Matrix4& RenderCamera::getViewMatrix(void) const
{
	if (mCullFrustum)
	{
		return mCullFrustum->getViewMatrix();
	}
	else
	{
		return RenderFrustum::getViewMatrix();
	}
}
//-----------------------------------------------------------------------
const Matrix4& RenderCamera::getViewMatrix(bool ownFrustumOnly) const
{
	if (ownFrustumOnly)
	{
		return RenderFrustum::getViewMatrix();
	}
	else
	{
		return getViewMatrix();
	}
}
//-----------------------------------------------------------------------
//_______________________________________________________
//|														|
//|	getRayForwardIntersect								|
//|	-----------------------------						|
//|	get the intersections of frustum rays with a plane	|
//| of interest.  The plane is assumed to have constant	|
//| z.  If this is not the case, rays					|
//| should be rotated beforehand to work in a			|
//| coordinate system in which this is true.			|
//|_____________________________________________________|
//
Array<Vector4> RenderCamera::getRayForwardIntersect(const Vector3& anchor, const Vector3 *dir, scalar planeOffset) const
{
	Array<Vector4> res;

	if(!dir)
		return res;

	int infpt[4] = {0, 0, 0, 0}; // 0=finite, 1=infinite, 2=straddles infinity
	Vector3 vec[4];

	// find how much the anchor point must be displaced in the plane's
	// constant variable
	scalar delta = planeOffset - anchor.z;

	// now set the intersection point and note whether it is a 
	// point at infinity or straddles infinity
	unsigned int i;
	for (i=0; i<4; i++)
	{
		scalar test = dir[i].z * delta;
		if (test == 0.0) {
			vec[i] = dir[i];
			infpt[i] = 1;
		}
		else {
			scalar lambda = delta / dir[i].z;
			vec[i] = anchor + (lambda * dir[i]);
			if(test < 0.0)
				infpt[i] = 2;
		}
	}

	for (i=0; i<4; i++)
	{
		// store the finite intersection points
		if (infpt[i] == 0)
			res.Append(Vector4(vec[i].x, vec[i].y, vec[i].z, 1.0));
		else
		{
			// handle the infinite points of intersection;
			// cases split up into the possible frustum planes 
			// pieces which may contain a finite intersection point
			int nextind = (i+1) % 4;
			int prevind = (i+3) % 4;
			if ((infpt[prevind] == 0) || (infpt[nextind] == 0))
			{
				if (infpt[i] == 1)
					res.Append(Vector4(vec[i].x, vec[i].y, vec[i].z, 0.0));
				else
				{
					// handle the intersection points that straddle infinity (back-project)
					if(infpt[prevind] == 0) 
					{
						Vector3 temp = vec[prevind] - vec[i];
						res.Append(Vector4(temp.x, temp.y, temp.z, 0.0));
					}
					if(infpt[nextind] == 0)
					{
						Vector3 temp = vec[nextind] - vec[i];
						res.Append(Vector4(temp.x, temp.y, temp.z, 0.0));
					}
				}
			} // end if we need to add an intersection point to the list
		} // end if infinite point needs to be considered
	} // end loop over frustun corners

	// we end up with either 0, 3, 4, or 5 intersection points

	return res;
}

//_______________________________________________________
//|														|
//|	forwardIntersect									|
//|	-----------------------------						|
//|	Forward intersect the camera's frustum rays with	|
//| a specified plane of interest.						|
//| Note that if the frustum rays shoot out and would	|
//| back project onto the plane, this means the forward	|
//| intersection of the frustum would occur at the		|
//| line at infinity.									|
//|_____________________________________________________|
//
void RenderCamera::forwardIntersect(const Plane& worldPlane, Array<Vector4>* intersect3d) const
{
	if(!intersect3d)
		return;

	Vector3 trCorner = getWorldSpaceCorners()[0];
	Vector3 tlCorner = getWorldSpaceCorners()[1];
	Vector3 blCorner = getWorldSpaceCorners()[2];
	Vector3 brCorner = getWorldSpaceCorners()[3];

	// need some sort of rotation that will bring the plane normal to the z axis
	Plane pval = worldPlane;
	if(pval.normal.z < 0.0)
	{
		pval.normal *= -1.0;
		pval.d *= -1.0;
	}
	Quaternion invPlaneRot = pval.normal.getRotationTo(Vector3::UNIT_Z);

	// get rotated light
	Vector3 lPos = invPlaneRot * getDerivedPosition();
	Vector3 vec[4];
	vec[0] = invPlaneRot * trCorner - lPos;
	vec[1] = invPlaneRot * tlCorner - lPos; 
	vec[2] = invPlaneRot * blCorner - lPos; 
	vec[3] = invPlaneRot * brCorner - lPos; 

	// compute intersection points on plane
	Array<Vector4> iPnt = getRayForwardIntersect(lPos, vec, -pval.d);


	// return wanted data
	if(intersect3d) 
	{
		Quaternion planeRot = invPlaneRot.Inverse();
		(*intersect3d).Reset();
		for(int i=0; i<iPnt.Size(); i++)
		{
			Vector3 intersection = planeRot * Vector3(iPnt[i].x, iPnt[i].y, iPnt[i].z);
			(*intersect3d).Append(Vector4(intersection.x, intersection.y, intersection.z, iPnt[i].w));
		}
	}
}
//-----------------------------------------------------------------------
void RenderCamera::synchroniseBaseSettingsWith(const RenderCamera* cam)
{
	this->setPosition(cam->getPosition());
	this->setProjectionType(cam->getProjectionType());
	this->setOrientation(cam->getOrientation());
	this->setAspectRatio(cam->getAspectRatio());
	this->setNearClipDistance(cam->getNearClipDistance());
	this->setFarClipDistance(cam->getFarClipDistance());
	this->setUseRenderingDistance(cam->getUseRenderingDistance());
	this->setFOVy(cam->getFOVy());
	this->setFocalLength(cam->getFocalLength());

	// Don't do these, they're not base settings and can cause referencing issues
	//this->setLodCamera(cam->getLodCamera());
	//this->setCullingFrustum(cam->getCullingFrustum());

}

_NAMESPACE_END