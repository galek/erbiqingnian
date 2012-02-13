
#pragma once

#include "common.h"
#include "renderPrerequisites.h"
#include "gearsPrerequisites.h"
#include "renderWindow.h"
#include "renderCamera.h"
#include "input/gearsInput.h"

_NAMESPACE_BEGIN

// ²âÊÔÉãÏñ»úÄ£Ê½
enum CameraStyle
{
	CS_FREELOOK,
	CS_ORBIT,
	CS_MANUAL
};

//////////////////////////////////////////////////////////////////////////

class TestCameraManager
{
public:

	TestCameraManager(RenderCamera* cam);

	virtual ~TestCameraManager() {}

public:

	virtual void setCamera(RenderCamera* cam)
	{
		mCamera = cam;
	}

	virtual RenderCamera* getCamera()
	{
		return mCamera;
	}

	virtual void setYawPitchDist(Radian yaw, Radian pitch, scalar dist);

	virtual void setTopSpeed(scalar topSpeed)
	{
		mTopSpeed = topSpeed;
	}

	virtual scalar getTopSpeed()
	{
		return mTopSpeed;
	}

	virtual void setStyle(CameraStyle style);

	virtual CameraStyle getStyle()
	{
		return mStyle;
	}

	virtual void manualStop();

	virtual bool update(float elapsed);

	virtual void injectKeyDown(const KeyEvent& evt)
	{
		if (mStyle == CS_FREELOOK)
		{
			if (evt.key == KC_W || evt.key == KC_UP) mGoingForward = true;
			else if (evt.key == KC_S || evt.key == KC_DOWN) mGoingBack = true;
			else if (evt.key == KC_A || evt.key == KC_LEFT) mGoingLeft = true;
			else if (evt.key == KC_D || evt.key == KC_RIGHT) mGoingRight = true;
			else if (evt.key == KC_PGUP) mGoingUp = true;
			else if (evt.key == KC_PGDOWN) mGoingDown = true;
			else if (evt.key == KC_LSHIFT) mFastMove = true;
		}
	}

	virtual void injectKeyUp(const KeyEvent& evt)
	{
		if (mStyle == CS_FREELOOK)
		{
			if (evt.key == KC_W || evt.key == KC_UP) mGoingForward = false;
			else if (evt.key == KC_S || evt.key == KC_DOWN) mGoingBack = false;
			else if (evt.key == KC_A || evt.key == KC_LEFT) mGoingLeft = false;
			else if (evt.key == KC_D || evt.key == KC_RIGHT) mGoingRight = false;
			else if (evt.key == KC_PGUP) mGoingUp = false;
			else if (evt.key == KC_PGDOWN) mGoingDown = false;
			else if (evt.key == KC_LSHIFT) mFastMove = false;
		}
	}

	virtual void injectMouseMove(const MouseEvent& evt)
	{
		if (mStyle == CS_ORBIT)
		{
			scalar dist = (mCamera->getPosition() - mTarget).length();

			if (mOrbiting)   // yaw around the target, and pitch locally
			{
				mCamera->setPosition(mTarget);

				mCamera->yaw(Degree(-evt.state.X.rel * 0.25f));
				mCamera->pitch(Degree(-evt.state.Y.rel * 0.25f));

				mCamera->moveRelative(Vector3(0, 0, dist));

				// don't let the camera go over the top or around the bottom of the target
			}
			else if (mZooming)  // move the camera toward or away from the target
			{
				// the further the camera is, the faster it moves
				mCamera->moveRelative(Vector3(0, 0, evt.state.Y.rel * 0.004f * dist));
			}
			else if (evt.state.Z.rel != 0)  // move the camera toward or away from the target
			{
				// the further the camera is, the faster it moves
				mCamera->moveRelative(Vector3(0, 0, -evt.state.Z.rel * 0.0008f * dist));
			}
		}
		else if (mStyle == CS_FREELOOK)
		{
			mCamera->yaw(Degree(-evt.state.X.rel * 0.15f));
			mCamera->pitch(Degree(-evt.state.Y.rel * 0.15f));
		}
	}

	virtual void injectMouseDown(const MouseEvent& evt, MouseButtonID id)
	{
		if (mStyle == CS_ORBIT)
		{
			if (id == MB_Left) mOrbiting = true;
			else if (id == MB_Right) mZooming = true;
		}
	}

	virtual void injectMouseUp(const MouseEvent& evt, MouseButtonID id)
	{
		if (mStyle == CS_ORBIT)
		{
			if (id == MB_Left) mOrbiting = false;
			else if (id == MB_Right) mZooming = false;
		}
	}

protected:

	RenderCamera* mCamera;
	CameraStyle mStyle;
	bool mOrbiting;
	bool mZooming;
	scalar mTopSpeed;
	Vector3 mVelocity;
	Vector3 mTarget;
	bool mGoingForward;
	bool mGoingBack;
	bool mGoingLeft;
	bool mGoingRight;
	bool mGoingUp;
	bool mGoingDown;
	bool mFastMove;

};

_NAMESPACE_END