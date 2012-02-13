
#ifndef PI_Joystick_H
#define PI_Joystick_H

#include "gearsInputObject.h"
#include "gearsInputEvents.h"

namespace Philo
{
	#define PI_JOYSTICK_VECTOR3_DEFAULT 2.28f

	class Pov : Component
	{
	public:
		Pov() : Component(PI_POV), direction(0) {}

		static const int Centered  = 0x00000000;
		static const int North     = 0x00000001;
		static const int South     = 0x00000010;
		static const int East      = 0x00000100;
		static const int West      = 0x00001000;
		static const int NorthEast = 0x00000101;
		static const int SouthEast = 0x00000110;
		static const int NorthWest = 0x00001001;
		static const int SouthWest = 0x00001010;

		int direction;
	};

	//! A sliding axis - only used in Win32 Right Now
	class Slider : Component
	{
	public:
		Slider() : Component(PI_Slider), abX(0), abY(0) {};
		//! true if pushed, false otherwise
		int abX, abY;
	};

	/**
		Represents the state of the joystick
		All members are valid for both buffered and non buffered mode
		Sticks with zero values are not present on the device
	*/
	class JoyStickState
	{
	public:
		//! Constructor
		JoyStickState() { clear(); }

		//! Represents all the buttons (uses a bitset)
		std::vector<bool> mButtons;

		//! Represents all the single axes on the device
		std::vector<Axis> mAxes;

		//! Represents the value of a POV. Maximum of 4
		Pov mPOV[4];

		//! Represent the max sliders
		Slider mSliders[4];

		//! Represents all Vector type controls the device exports
		std::vector<Vector3> mVectors;

		//! internal method to reset all variables to initial values
		void clear()
		{
			for( std::vector<bool>::iterator i = mButtons.begin(), e = mButtons.end(); i != e; ++i )
			{
				(*i) = false;
			}

			for( std::vector<Axis>::iterator i = mAxes.begin(), e = mAxes.end(); i != e; ++i )
			{
				i->absOnly = true; //Currently, joysticks only report Absolute values
				i->clear();
			}

			for( std::vector<Vector3>::iterator i = mVectors.begin(), e = mVectors.end(); i != e; ++i )
			{
				*i = Vector3::ZERO;
			}

			for( int i = 0; i < 4; ++i )
			{
				mPOV[i].direction = Pov::Centered;
				mSliders[i].abX = mSliders[i].abY = 0;
			}
		}
	};

	/** Specialised for joystick events */
	class JoyStickEvent : public EventArg
	{
	public:
		JoyStickEvent( GearInputObject* obj, const JoyStickState &st ) : EventArg(obj), state(st) {}
		virtual ~JoyStickEvent() {}

		const JoyStickState &state;
	};

	/**
		To recieve buffered joystick input, derive a class from this, and implement the
		methods here. Then set the call back to your JoyStick instance with JoyStick::setEventCallback
		Each JoyStick instance can use the same callback class, as a devID number will be provided
		to differentiate between connected joysticks. Of course, each can have a seperate
		callback instead.
	*/
	class GearJoyStickListener
	{
	public:
		virtual ~GearJoyStickListener() {}
		/** @remarks Joystick button down event */
		virtual bool buttonPressed( const JoyStickEvent &arg, int button ) = 0;
		
		/** @remarks Joystick button up event */
		virtual bool buttonReleased( const JoyStickEvent &arg, int button ) = 0;

		/** @remarks Joystick axis moved event */
		virtual bool axisMoved( const JoyStickEvent &arg, int axis ) = 0;

		//-- Not so common control events, so are not required --//
		//! Joystick Event, and sliderID
		virtual bool sliderMoved( const JoyStickEvent &, int index) {return true;}
		
		//! Joystick Event, and povID
		virtual bool povMoved( const JoyStickEvent &arg, int index) {return true;}

		//! Joystick Event, and Vector3ID
		virtual bool vector3Moved( const JoyStickEvent &arg, int index) {return true;}
	};

	/**
		Joystick base class. To be implemented by specific system (ie. DirectX joystick)
		This class is useful as you remain OS independent using this common interface.
	*/
	class GearJoyStick : public GearInputObject
	{
	public:
		virtual ~GearJoyStick() {}

		/**
		@remarks
			Returns the number of requested components
		@param cType
			The ComponentType you are interested in knowing about
		*/
		int getNumberOfComponents(ComponentType cType);

		/**
		@remarks
			Sets a cutoff limit for changes in the Vector3 component for movement to 
			be ignored. Helps reduce much event traffic for frequent small/sensitive
			changes
		@param degrees
			The degree under which Vector3 events should be discarded
		*/
		void setVector3Sensitivity(float degrees = PI_JOYSTICK_VECTOR3_DEFAULT);

		/**
		@remarks
			Returns the sensitivity cutoff for Vector3 Component
		*/
		float getVector3Sensitivity();

		/**
		@remarks
			Register/unregister a JoyStick Listener - Only one allowed for simplicity. If broadcasting
			is neccessary, just broadcast from the callback you registered.
		@param joyListener
			Send a pointer to a class derived from JoyStickListener or 0 to clear the callback
		*/
		virtual void setEventCallback( GearJoyStickListener *joyListener );

		/** @remarks Returns currently set callback.. or null */
		GearJoyStickListener* getEventCallback();

		/** @remarks Returns the state of the joystick - is valid for both buffered and non buffered mode */
		const JoyStickState& getJoyStickState() const { return mState; }

		//! The minimal axis value
		static const int MIN_AXIS = -32768;

		//! The maximum axis value
		static const int MAX_AXIS = 32767;

	protected:
		GearJoyStick(const std::string &vendor, bool buffered, int devID, GearInputManager* creator);

		//! Number of sliders
		int mSliders;

		//! Number of POVs
		int mPOVs;

		//! The JoyStickState structure (contains all component values)
		JoyStickState mState;

		//! The callback listener
		GearJoyStickListener *mListener;

		//! Adjustment factor for orientation vector accuracy
		float mVector3Sensitivity;
	};
}
#endif
