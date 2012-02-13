
#pragma once

#include "../gearsInputJoyStick.h"
#include "gearsInputPrereqsDx.h"

namespace Philo
{
	class Win32JoyStick : public GearJoyStick
	{
	public:
		Win32JoyStick( GearInputManager* creator, IDirectInput8* pDI, bool buffered, DWORD coopSettings, const JoyStickInfo &info );
		virtual ~Win32JoyStick();
		
		/** @copydoc Object::setBuffered */
		virtual void setBuffered(bool buffered);
		
		/** @copydoc Object::capture */
		virtual void capture();

		/** @copydoc Object::queryInterface */
		virtual GearInputInterface* queryInterface(GearInputInterface::IType type);

		/** @copydoc Object::_initialize */
		virtual void _initialize();

	protected:
		//! Enumerates all things
		void _enumerate();
		//! Enumerate axis callback
		static BOOL CALLBACK DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
		//! Enumerate Force Feedback callback
		static BOOL CALLBACK DIEnumEffectsCallback(LPCDIEFFECTINFO pdei, LPVOID pvRef);

		bool _doButtonClick( int button, DIDEVICEOBJECTDATA& di );
		bool _changePOV( int pov, DIDEVICEOBJECTDATA& di );

		IDirectInput8* mDirectInput;
		IDirectInputDevice8* mJoyStick;
		DWORD coopSetting;
		GUID deviceGuid;

		//! A force feedback device
		Win32ForceFeedback* ff_device;

		//! Mapping
		int _AxisNumber;
	};
}
