
#pragma once

#include "renderFrustum.h"
#include "math/plane.h"

_NAMESPACE_BEGIN

class RenderCamera : public RenderFrustum
{
public:

	RenderCamera(const String& name);

	virtual ~RenderCamera();

public:

	void setPolygonMode(PolygonMode sd);

	PolygonMode getPolygonMode(void) const;

	void setPosition(scalar x, scalar y, scalar z);

	void setPosition(const Vector3& vec);

	const Vector3& getPosition(void) const;

	void move(const Vector3& vec);

	void moveRelative(const Vector3& vec);

	void setDirection(scalar x, scalar y, scalar z);

	void setDirection(const Vector3& vec);

	Vector3 getDirection(void) const;

	Vector3 getUp(void) const;

	Vector3 getRight(void) const;

	void lookAt( const Vector3& targetPoint );

	void lookAt(scalar x, scalar y, scalar z);

	void roll(const Radian& angle);

	void yaw(const Radian& angle);

	void pitch(const Radian& angle);

	void rotate(const Vector3& axis, const Radian& angle);

	void rotate(const Quaternion& q);

	void setFixedYawAxis( bool useFixed, const Vector3& fixedAxis = Vector3::UNIT_Y );

	const Quaternion& getOrientation(void) const;

	void setOrientation(const Quaternion& q);

	const Quaternion& getDerivedOrientation(void) const;

	const Vector3& getDerivedPosition(void) const;

	Vector3 getDerivedDirection(void) const;

	Vector3 getDerivedUp(void) const;

	Vector3 getDerivedRight(void) const;

	const Quaternion& getRealOrientation(void) const;

	const Vector3& getRealPosition(void) const;

	Vector3 getRealDirection(void) const;

	Vector3 getRealUp(void) const;

	Vector3 getRealRight(void) const;

	void getWorldTransforms(Matrix4* mat) const;

	Ray getCameraToViewportRay(scalar screenx, scalar screeny) const;

	void getCameraToViewportRay(scalar screenx, scalar screeny, Ray* outRay) const;

	PlaneBoundedVolume getCameraToViewportBoxVolume(scalar screenLeft, 
		scalar screenTop, scalar screenRight, scalar screenBottom, bool includeFarPlane = false);

	void getCameraToViewportBoxVolume(scalar screenLeft, 
		scalar screenTop, scalar screenRight, scalar screenBottom, 
		PlaneBoundedVolume* outVolume, bool includeFarPlane = false);

	scalar _getLodBiasInverse(void) const;

	void _autoTrack(void);

	virtual void setWindow (scalar Left, scalar Top, scalar Right, scalar Bottom);
	/// Cancel view window.
	virtual void resetWindow (void);
	/// Returns if a viewport window is being used
	virtual bool isWindowSet(void) const { return mWindowSet; }
	/// Gets the window clip planes, only applicable if isWindowSet == true
	const Array<Plane>& getWindowPlanes(void) const;

	scalar getBoundingRadius(void) const;

	void setAutoAspectRatio(bool autoratio);

	bool getAutoAspectRatio(void) const;

	void setCullingFrustum(RenderFrustum* frustum) { mCullFrustum = frustum; }

	RenderFrustum* getCullingFrustum(void) const { return mCullFrustum; }

	virtual void forwardIntersect(const Plane& worldPlane, Array<Vector4>* intersect3d) const;

	/// @copydoc Frustum::isVisible
	bool isVisible(const AxisAlignedBox& bound, FrustumPlane* culledBy = 0) const;
	/// @copydoc Frustum::isVisible
	bool isVisible(const Sphere& bound, FrustumPlane* culledBy = 0) const;
	/// @copydoc Frustum::isVisible
	bool isVisible(const Vector3& vert, FrustumPlane* culledBy = 0) const;
	/// @copydoc Frustum::getWorldSpaceCorners
	const Vector3* getWorldSpaceCorners(void) const;
	/// @copydoc Frustum::getFrustumPlane
	const Plane& getFrustumPlane( unsigned short plane ) const;
	/// @copydoc Frustum::projectSphere
	bool projectSphere(const Sphere& sphere, 
		scalar* left, scalar* top, scalar* right, scalar* bottom) const;
	/// @copydoc Frustum::getNearClipDistance
	scalar getNearClipDistance(void) const;
	/// @copydoc Frustum::getFarClipDistance
	scalar getFarClipDistance(void) const;
	/// @copydoc Frustum::getViewMatrix
	const Matrix4& getViewMatrix(void) const;

	const Matrix4& getViewMatrix(bool ownFrustumOnly) const;

	virtual void setUseRenderingDistance(bool use) { mUseRenderingDistance = use; }

	virtual bool getUseRenderingDistance(void) const { return mUseRenderingDistance; }

	const Vector3& getPositionForViewUpdate(void) const;

	const Quaternion& getOrientationForViewUpdate(void) const;

	void setUseMinPixelSize(bool enable) { mUseMinPixelSize = enable; }

	bool getUseMinPixelSize() const { return mUseMinPixelSize; }

	scalar getPixelDisplayRatio() const { return mPixelDisplayRatio; }

	void synchroniseBaseSettingsWith(const RenderCamera* cam);

protected:

	/// Camera orientation, quaternion style
	Quaternion mOrientation;

	/// Camera position - default (0,0,0)
	Vector3 mPosition;

	/// Derived orientation/position of the camera, including reflection
	mutable Quaternion mDerivedOrientation;
	mutable Vector3 mDerivedPosition;

	/// scalar world orientation/position of the camera
	mutable Quaternion mRealOrientation;
	mutable Vector3 mRealPosition;

	/// Whether to yaw around a fixed axis.
	bool mYawFixed;
	/// Fixed axis to yaw around
	Vector3 mYawFixedAxis;

	/// Rendering type
	PolygonMode mSceneDetail;

	/// Stored number of visible faces in the last render
	unsigned int mVisFacesLastRender;

	/// Stored number of visible faces in the last render
	unsigned int mVisBatchesLastRender;

	/// Shared class-level name for Movable type
	static String msMovableType;

	/** Viewing window. 
	@remarks
	Generalize camera class for the case, when viewing frustum doesn't cover all viewport.
	*/
	scalar mWLeft, mWTop, mWRight, mWBottom;
	/// Is viewing window used.
	bool mWindowSet;
	/// Windowed viewport clip planes 
	mutable Array<Plane> mWindowClipPlanes;
	// Was viewing window changed.
	mutable bool mRecalcWindow;
	/// The last viewport to be added using this camera

	//Viewport* mLastViewport;
	/** Whether aspect ratio will automatically be recalculated 
	when a viewport changes its size
	*/
	bool mAutoAspectRatio;
	/// Custom culling frustum
	RenderFrustum *mCullFrustum;
	/// Whether or not the rendering distance of objects should take effect for this camera
	bool mUseRenderingDistance;

	/// Whether or not the minimum display size of objects should take effect for this camera
	bool mUseMinPixelSize;
	/// @see Camera::getPixelDisplayRatio
	scalar mPixelDisplayRatio;

	// Internal functions for calcs
	bool isViewOutOfDate(void) const;
	/// Signal to update frustum information.
	void invalidateFrustum(void) const;
	/// Signal to update view information.
	void invalidateView(void) const;

	virtual void setWindowImpl(void) const;

	/** Helper function for forwardIntersect that intersects rays with canonical plane */
	virtual Array<Vector4> getRayForwardIntersect(const Vector3& anchor, const Vector3 *dir, scalar planeOffset) const;

};

_NAMESPACE_END