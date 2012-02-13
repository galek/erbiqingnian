
#ifndef PI_Mouse_H
#define PI_Mouse_H

#include "gearsInputObject.h"
#include "gearsInputEvents.h"

namespace Philo
{
	//! Button ID for mouse devices
	enum MouseButtonID
	{
		MB_Left = 0, MB_Right, MB_Middle,
		MB_Button3, MB_Button4,	MB_Button5, MB_Button6,	MB_Button7
	};

	/**
		Represents the state of the mouse
		All members are valid for both buffered and non buffered mode
	*/
	class MouseState
	{
	public:
		MouseState() : width(50), height(50), buttons(0) {};

		/** Represents the height/width of your display area.. used if mouse clipping
		or mouse grabbed in case of X11 - defaults to 50.. Make sure to set this
		and change when your size changes.. */
		mutable int width, height;

		//! X Axis component
		Axis X;

		//! Y Axis Component
		Axis Y;

		//! Z Axis Component
		Axis Z;

		//! represents all buttons - bit position indicates button down
		int buttons;

		//! Button down test
		inline bool buttonDown( MouseButtonID button ) const
		{
			return ((buttons & ( 1L << button )) == 0) ? false : true;
		}

		//! Clear all the values
		void clear()
		{
			X.clear();
			Y.clear();
			Z.clear();
			buttons = 0;
		}
	};

	/** Specialised for mouse events */
	class MouseEvent : public EventArg
	{
	public:
		MouseEvent( GearInputObject *obj, const MouseState &ms )	: EventArg(obj), state(ms) {}
		virtual ~MouseEvent() {}

		//! The state of the mouse - including buttons and axes
		const MouseState &state;
	};

	/**
		To recieve buffered mouse input, derive a class from this, and implement the
		methods here. Then set the call back to your Mouse instance with Mouse::setEventCallback
	*/
	class GearMouseListener
	{
	public:
		virtual ~GearMouseListener() {}
		virtual bool mouseMoved( const MouseEvent &arg ) = 0;
		virtual bool mousePressed( const MouseEvent &arg, MouseButtonID id ) = 0;
		virtual bool mouseReleased( const MouseEvent &arg, MouseButtonID id ) = 0;
	};

	/**
		Mouse base class. To be implemented by specific system (ie. DirectX Mouse)
		This class is useful as you remain OS independent using this common interface.
	*/
	class GearMouse : public GearInputObject
	{
	public:
		virtual ~GearMouse() {}

		/**
		@remarks
			Register/unregister a Mouse Listener - Only one allowed for simplicity. If broadcasting
			is neccessary, just broadcast from the callback you registered.
		@param mouseListener
			Send a pointer to a class derived from MouseListener or 0 to clear the callback
		*/
		virtual void setEventCallback( GearMouseListener *mouseListener ) {mListener = mouseListener;}

		/** @remarks Returns currently set callback.. or 0 */
		GearMouseListener* getEventCallback() {return mListener;}

		/** @remarks Returns the state of the mouse - is valid for both buffered and non buffered mode */
		const MouseState& getMouseState() const { return mState; }

	protected:
		GearMouse(const std::string &vendor, bool buffered, int devID, GearInputManager* creator)
			: GearInputObject(vendor, PI_Mouse, buffered, devID, creator), mListener(0) {}

		//! The state of the mouse
		MouseState mState;

		//! Used for buffered/actionmapping callback
		GearMouseListener *mListener;
	};
}
#endif
