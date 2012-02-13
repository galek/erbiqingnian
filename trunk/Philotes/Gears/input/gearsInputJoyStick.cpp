
#include "gearsInputJoyStick.h"

using namespace Philo;

//----------------------------------------------------------------------------//
GearJoyStick::GearJoyStick(const std::string &vendor, bool buffered, int devID, GearInputManager* creator) :
	GearInputObject(vendor, PI_JoyStick, buffered, devID, creator),
	mSliders(0),
	mPOVs(0),
	mListener(0),
	mVector3Sensitivity(PI_JOYSTICK_VECTOR3_DEFAULT)
{
}

//----------------------------------------------------------------------------//
int GearJoyStick::getNumberOfComponents(ComponentType cType)
{
	switch( cType )
	{
	case PI_Button:	return (int)mState.mButtons.size();
	case PI_Axis:		return (int)mState.mAxes.size();
	case PI_Slider:	return mSliders;
	case PI_POV:		return mPOVs;
	case PI_Vector3:	return (int)mState.mVectors.size();
	default:			return 0;
	}
}

//----------------------------------------------------------------------------//
void GearJoyStick::setVector3Sensitivity(float degrees)
{
	mVector3Sensitivity = degrees;
}

//----------------------------------------------------------------------------//
float GearJoyStick::getVector3Sensitivity()
{
	return mVector3Sensitivity;
}

//----------------------------------------------------------------------------//
void GearJoyStick::setEventCallback( GearJoyStickListener *joyListener )
{
	mListener = joyListener;
}

//----------------------------------------------------------------------------//
GearJoyStickListener* GearJoyStick::getEventCallback()
{
	return mListener;
}
