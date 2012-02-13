
#include "testCamera.h"

_NAMESPACE_BEGIN

TestCameraManager::TestCameraManager( RenderCamera* cam ) : mCamera(0)
	, mOrbiting(false)
	, mZooming(false)
	, mTopSpeed(150)
	, mVelocity(Vector3::ZERO)
	, mTarget(Vector3::ZERO)
	, mGoingForward(false)
	, mGoingBack(false)
	, mGoingLeft(false)
	, mGoingRight(false)
	, mGoingUp(false)
	, mGoingDown(false)
	, mFastMove(false)
{
	setCamera(cam);
	setStyle(CS_ORBIT);
}

void TestCameraManager::setYawPitchDist( Radian yaw, Radian pitch, scalar dist )
{
	mCamera->setPosition(mTarget);
	mCamera->setOrientation(Quaternion::IDENTITY);
	mCamera->yaw(yaw);
	mCamera->pitch(-pitch);
	mCamera->moveRelative(Vector3(0, 0, dist));
}

void TestCameraManager::setStyle( CameraStyle style )
{
	if (mStyle != CS_ORBIT && style == CS_ORBIT)
	{
		mCamera->setFixedYawAxis(true);
		manualStop();
		setYawPitchDist(Degree(0), Degree(15), 150);

	}
	else if (mStyle != CS_FREELOOK && style == CS_FREELOOK)
	{
		mCamera->setFixedYawAxis(true);
	}
	else if (mStyle != CS_MANUAL && style == CS_MANUAL)
	{
		manualStop();
	}
	mStyle = style;
}

void TestCameraManager::manualStop()
{
	if (mStyle == CS_FREELOOK)
	{
		mGoingForward = false;
		mGoingBack = false;
		mGoingLeft = false;
		mGoingRight = false;
		mGoingUp = false;
		mGoingDown = false;
		mVelocity = Vector3::ZERO;
	}
}

bool TestCameraManager::update( float elapsed )
{
	if (mStyle == CS_FREELOOK)
	{
		// build our acceleration vector based on keyboard input composite
		Vector3 accel = Vector3::ZERO;
		if (mGoingForward) accel += mCamera->getDirection();
		if (mGoingBack) accel -= mCamera->getDirection();
		if (mGoingRight) accel += mCamera->getRight();
		if (mGoingLeft) accel -= mCamera->getRight();
		if (mGoingUp) accel += mCamera->getUp();
		if (mGoingDown) accel -= mCamera->getUp();

		// if accelerating, try to reach top speed in a certain time
		scalar topSpeed = mFastMove ? mTopSpeed * 20 : mTopSpeed;
		if (accel.squaredLength() != 0)
		{
			accel.normalise();
			mVelocity += accel * topSpeed * elapsed * 10;
		}
		// if not accelerating, try to stop in a certain time
		else mVelocity -= mVelocity * elapsed * 10;

		scalar tooSmall = std::numeric_limits<scalar>::epsilon();

		// keep camera velocity below top speed and above epsilon
		if (mVelocity.squaredLength() > topSpeed * topSpeed)
		{
			mVelocity.normalise();
			mVelocity *= topSpeed;
		}
		else if (mVelocity.squaredLength() < tooSmall * tooSmall)
			mVelocity = Vector3::ZERO;

		if (mVelocity != Vector3::ZERO) mCamera->move(mVelocity * elapsed);
	}

	return true;
}




_NAMESPACE_END

