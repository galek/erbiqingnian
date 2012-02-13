
#pragma once

#include "renderElement.h"
#include "math/axisAlignedBox.h"
#include "math/plane.h"

_NAMESPACE_BEGIN

enum OrientationMode
{
	OR_DEGREE_0       = 0,
	OR_DEGREE_90      = 1,
	OR_DEGREE_180     = 2,
	OR_DEGREE_270     = 3,

	OR_PORTRAIT       = OR_DEGREE_0,
	OR_LANDSCAPERIGHT = OR_DEGREE_90,
	OR_LANDSCAPELEFT  = OR_DEGREE_270
};

enum ProjectionType
{
	PT_ORTHOGRAPHIC,
	PT_PERSPECTIVE
};

enum FrustumPlane
{
	FRUSTUM_PLANE_NEAR   = 0,
	FRUSTUM_PLANE_FAR    = 1,
	FRUSTUM_PLANE_LEFT   = 2,
	FRUSTUM_PLANE_RIGHT  = 3,
	FRUSTUM_PLANE_TOP    = 4,
	FRUSTUM_PLANE_BOTTOM = 5
};

//////////////////////////////////////////////////////////////////////////

class RenderFrustum : public RenderElement
{
public:

	RenderFrustum(const String& name = String::BLANK);

	virtual ~RenderFrustum();

public:

	virtual bool	isValid(void) const {return true;}
	
	virtual void setFOVy(const Radian& fovy);

	/** Retrieves the frustums Y-dimension Field Of View (FOV).
	*/
	virtual const Radian& getFOVy(void) const;

	virtual void setNearClipDistance(scalar nearDist);

	/** Sets the position of the near clipping plane.
	*/
	virtual scalar getNearClipDistance(void) const;

	virtual void setFarClipDistance(scalar farDist);

	/** Retrieves the distance from the frustum to the far clipping plane.
	*/
	virtual scalar getFarClipDistance(void) const;

	virtual void setAspectRatio(scalar ratio);

	/** Retreives the current aspect ratio.
	*/
	virtual scalar getAspectRatio(void) const;

	virtual void setFrustumOffset(const Vector2& offset);

	virtual void setFrustumOffset(scalar horizontal = 0.0, scalar vertical = 0.0);

	virtual const Vector2& getFrustumOffset() const;

	virtual void setFocalLength(scalar focalLength = 1.0);

	virtual scalar getFocalLength() const;

	virtual void setFrustumExtents(scalar left, scalar right, scalar top, scalar bottom);

	virtual void resetFrustumExtents(); 

	virtual void getFrustumExtents(scalar& outleft, scalar& outright, scalar& outtop, scalar& outbottom) const;

	// 直接给API使用的投影矩阵
	virtual const Matrix4& getProjectionMatrixRS(void) const;
	
	// 直接给API用的，深度调整后的投影矩阵
	virtual const Matrix4& getProjectionMatrixWithRSDepth(void) const;
	
	virtual const Matrix4& getProjectionMatrix(void) const;

	virtual const Matrix4& getViewMatrix(void) const;

	virtual void calcViewMatrixRelative(const Vector3& relPos, Matrix4& matToUpdate) const;

	virtual void setCustomViewMatrix(bool enable, 
		const Matrix4& viewMatrix = Matrix4::IDENTITY);
	
	virtual bool isCustomViewMatrixEnabled(void) const 
	{ return mCustomViewMatrix; }

	virtual void setCustomProjectionMatrix(bool enable, 
		const Matrix4& projectionMatrix = Matrix4::IDENTITY);
	
	virtual bool isCustomProjectionMatrixEnabled(void) const
	{ return mCustomProjMatrix; }

	virtual const Plane* getFrustumPlanes(void) const;

	virtual const Plane& getFrustumPlane( unsigned short plane ) const;

	virtual bool isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy = 0) const;

	virtual bool isVisible(const Sphere& bound, FrustumPlane* culledBy = 0) const;

	virtual bool isVisible(const Vector3& vert, FrustumPlane* culledBy = 0) const;

	/** Overridden from MovableObject */
	const AxisAlignedBox& getBoundingBox(void) const;

	/** Overridden from MovableObject */
	scalar getBoundingRadius(void) const;

	virtual const Vector3* getWorldSpaceCorners(void) const;

	/** Sets the type of projection to use (orthographic or perspective). Default is perspective.
	*/
	virtual void setProjectionType(ProjectionType pt);

	/** Retrieves info on the type of projection used (orthographic or perspective).
	*/
	virtual ProjectionType getProjectionType(void) const;

	virtual void setOrthoWindow(scalar w, scalar h);

	virtual void setOrthoWindowHeight(scalar h);

	virtual void setOrthoWindowWidth(scalar w);

	virtual scalar getOrthoWindowHeight() const;

	virtual scalar getOrthoWindowWidth() const;

	virtual void enableReflection(const Plane& p);

	virtual void disableReflection(void);

	virtual bool isReflected(void) const { return mReflect; }
	
	virtual const Matrix4& getReflectionMatrix(void) const { return mReflectMatrix; }
	
	virtual const Plane& getReflectionPlane(void) const { return mReflectPlane; }

	virtual bool projectSphere(const Sphere& sphere, 
		scalar* left, scalar* top, scalar* right, scalar* bottom) const;

	virtual void enableCustomNearClipPlane(const Plane& plane);
	/** Disables any custom near clip plane. */
	virtual void disableCustomNearClipPlane(void);
	/** Is a custom near clip plane in use? */
	virtual bool isCustomNearClipPlaneEnabled(void) const 
	{ return mObliqueDepthProjection; }

	/// Small constant used to reduce far plane projection to avoid inaccuracies
	static const scalar INFINITE_FAR_PLANE_ADJUST;

	/** Get the derived position of this frustum. */
	virtual const Vector3& getPositionForViewUpdate(void) const;
	/** Get the derived orientation of this frustum. */
	virtual const Quaternion& getOrientationForViewUpdate(void) const;

	/** Gets a world-space list of planes enclosing the frustum.
	*/
	PlaneBoundedVolume getPlaneBoundedVolume();
	/** Set the orientation mode of the frustum. Default is OR_DEGREE_0
	@remarks
	Setting the orientation of a frustum is only supported on
	iPhone at this time.  An exception is thrown on other platforms.
	*/
	void setOrientationMode(OrientationMode orientationMode);

	OrientationMode getOrientationMode() const;

protected:
	/// Orthographic or perspective?
	ProjectionType mProjType;

	/// y-direction field-of-view (default 45)
	Radian mFOVy;
	/// Far clip distance - default 10000
	scalar mFarDist;
	/// Near clip distance - default 100
	scalar mNearDist;
	/// x/y viewport ratio - default 1.3333
	scalar mAspect;
	/// Ortho height size (world units)
	scalar mOrthoHeight;
	/// Off-axis frustum center offset - default (0.0, 0.0)
	Vector2 mFrustumOffset;
	/// Focal length of frustum (for stereo rendering, defaults to 1.0)
	scalar mFocalLength;

	/// The 6 main clipping planes
	mutable Plane mFrustumPlanes[6];

	/// Stored versions of parent orientation / position
	mutable Quaternion mLastParentOrientation;
	mutable Vector3 mLastParentPosition;

	/// Pre-calced projection matrix for the specific render system
	mutable Matrix4 mProjMatrixRS;
	/// Pre-calced standard projection matrix but with render system depth range
	mutable Matrix4 mProjMatrixRSDepth;
	/// Pre-calced standard projection matrix
	mutable Matrix4 mProjMatrix;
	/// Pre-calced view matrix
	mutable Matrix4 mViewMatrix;
	/// Something's changed in the frustum shape?
	mutable bool mRecalcFrustum;
	/// Something re the view pos has changed
	mutable bool mRecalcView;
	/// Something re the frustum planes has changed
	mutable bool mRecalcFrustumPlanes;
	/// Something re the world space corners has changed
	mutable bool mRecalcWorldSpaceCorners;
	/// Something re the vertex data has changed
	mutable bool mRecalcVertexData;
	/// Are we using a custom view matrix?
	bool mCustomViewMatrix;
	/// Are we using a custom projection matrix?
	bool mCustomProjMatrix;
	/// Have the frustum extents been manually set?
	bool mFrustumExtentsManuallySet;
	/// Frustum extents
	mutable scalar mLeft, mRight, mTop, mBottom;
	/// Frustum orientation mode
	mutable OrientationMode mOrientationMode;

	// Internal functions for calcs
	virtual void calcProjectionParameters(scalar& left, scalar& right, scalar& bottom, scalar& top) const;
	/// Update frustum if out of date
	virtual void updateFrustum(void) const;
	/// Update view if out of date
	virtual void updateView(void) const;
	/// Implementation of updateFrustum (called if out of date)
	virtual void updateFrustumImpl(void) const;
	/// Implementation of updateView (called if out of date)
	virtual void updateViewImpl(void) const;
	virtual void updateFrustumPlanes(void) const;
	/// Implementation of updateFrustumPlanes (called if out of date)
	virtual void updateFrustumPlanesImpl(void) const;
	virtual void updateWorldSpaceCorners(void) const;
	/// Implementation of updateWorldSpaceCorners (called if out of date)
	virtual void updateWorldSpaceCornersImpl(void) const;
	virtual bool isViewOutOfDate(void) const;
	virtual bool isFrustumOutOfDate(void) const;
	/// Signal to update frustum information.
	virtual void invalidateFrustum(void) const;
	/// Signal to update view information.
	virtual void invalidateView(void) const;

	mutable AxisAlignedBox mBoundingBox;
	mutable Vector3 mWorldSpaceCorners[8];

	/// Is this frustum to act as a reflection of itself?
	bool mReflect;
	/// Derived reflection matrix
	mutable Matrix4 mReflectMatrix;
	/// Fixed reflection plane
	mutable Plane mReflectPlane;

	/// Is this frustum using an oblique depth projection?
	bool mObliqueDepthProjection;
	/// Fixed oblique projection plane
	mutable Plane mObliqueProjPlane;

};

_NAMESPACE_END