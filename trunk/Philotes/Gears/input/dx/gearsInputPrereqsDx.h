#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//Max number of elements to collect from buffered DirectInput
#define KEYBOARD_DX_BUFFERSIZE	17
#define MOUSE_DX_BUFFERSIZE		64
#define JOYSTICK_DX_BUFFERSIZE	124

namespace Philo
{
	//Local Forward declarations
	class Win32InputManager;
	class Win32Keyboard;
	class Win32JoyStick;
	class Win32Mouse;
	class Win32ForceFeedback;

	//Information needed to create DirectInput joysticks
	class JoyStickInfo
	{
	public:
		int devId;
		GUID deviceID;
		std::string vendor;
	};

	typedef std::vector< JoyStickInfo > JoyStickInfoList;
}