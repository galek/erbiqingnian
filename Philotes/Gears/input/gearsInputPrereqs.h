
#pragma once

#include <vector>
#include <string>
#include <map>

namespace Philo
{
	//! Way to send OS nuetral parameters.. ie OS Window handles, modes, flags
	typedef std::multimap<std::string, std::string> ParamList;

	//! List of FactoryCreator's
	typedef std::vector<GearInputFactoryCreator*> FactoryList;

	//! Map of FactoryCreator created Objects
	typedef std::map<GearInputObject*, GearInputFactoryCreator*> FactoryCreatedObject;

	//! Each Input class has a General Type variable, a form of RTTI
    enum Type
	{
		PI_UnknownType   = 0,
		PI_Keyboard  	= 1,
		PI_Mouse     	= 2,
		PI_JoyStick  	= 3,
		PI_Tablet    	= 4
	};

	//! Map of device objects connected and their respective vendors
	typedef std::multimap<Type, std::string> DeviceList;

	//--------     Shared common components    ------------------------//

	//! Base type for all device components (button, axis, etc)
    enum ComponentType
	{
		PI_UnknownComType	= 0,
		PI_Button  			= 1, //ie. Key, mouse button, joy button, etc
		PI_Axis    			= 2, //ie. A joystick or mouse axis
		PI_Slider  			= 3, //
		PI_POV     			= 4, //ie. Arrow direction keys
		PI_Vector3 			= 5  //ie. WiiMote orientation
	};

	//! Base of all device components (button, axis, etc)
	class Component
	{
	public:
		Component() : cType(PI_UnknownComType) {};
		Component(ComponentType type) : cType(type) {};
		//! Indicates what type of coponent this is
		ComponentType cType;
	};

	//! Button can be a keyboard key, mouse button, etc
	class Button : public Component
	{
	public:
		Button() {}
		Button(bool bPushed) : Component(PI_Button), pushed(bPushed) {};
		//! true if pushed, false otherwise
		bool pushed;
	};

	//! Axis component
	class Axis : public Component
	{
	public:
		Axis() : Component(PI_Axis), abs(0), rel(0), absOnly(false) {};

		//! Absoulte and Relative value components
		int abs, rel;

		//! Indicates if this Axis only supports Absoulte (ie JoyStick)
		bool absOnly;

		//! Used internally by OIS
		void clear()
		{
			abs = rel = 0;
		}
	};
}

