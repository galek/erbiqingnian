
#pragma once

#include "../gearsInputMouse.h"
#include "gearsInputPrereqsDx.h"

namespace Philo
{
	class Win32Mouse : public GearMouse
	{
	public:
		Win32Mouse( GearInputManager* creator, IDirectInput8* pDI, bool buffered, DWORD coopSettings );
		virtual ~Win32Mouse();
		
		/** @copydoc Object::setBuffered */
		virtual void setBuffered(bool buffered);

		/** @copydoc Object::capture */
		virtual void capture();

		/** @copydoc Object::queryInterface */
		virtual GearInputInterface* queryInterface(GearInputInterface::IType type) {return 0;}

		/** @copydoc Object::_initialize */
		virtual void _initialize();

	protected:
		bool _doMouseClick( int mouseButton, DIDEVICEOBJECTDATA& di );

		IDirectInput8* mDirectInput;
		IDirectInputDevice8* mMouse;
		DWORD coopSetting;
		HWND mHwnd;
	};
}
