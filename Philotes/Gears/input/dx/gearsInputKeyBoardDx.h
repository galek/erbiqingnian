
#ifndef _WIN32_KEYBOARD_H_EADER_
#define _WIN32_KEYBOARD_H_EADER_

#include "../gearsInputKeyboard.h"
#include "gearsInputPrereqsDx.h"

namespace Philo
{
	class Win32Keyboard : public GearKeyboard
	{
	public:
		/**
		@remarks
			Constructor
		@param pDI
			Valid DirectInput8 Interface
		@param buffered
			True for buffered input mode
		@param coopSettings
			A combination of DI Flags (see DX Help for info on input device settings)
		*/
		Win32Keyboard( GearInputManager* creator, IDirectInput8* pDI, bool buffered, DWORD coopSettings );
		virtual ~Win32Keyboard();

		/** @copydoc Keyboard::isKeyDown */
		virtual bool isKeyDown( KeyCode key );
		
		/** @copydoc Keyboard::getAsString */
		virtual const std::string& getAsString( KeyCode kc );

		/** @copydoc Keyboard::copyKeyStates */
		virtual void copyKeyStates( char keys[256] );

		/** @copydoc Object::setBuffered */
		virtual void setBuffered(bool buffered);
		
		/** @copydoc Object::capture */
		virtual void capture();

		/** @copydoc Object::queryInterface */
		virtual GearInputInterface* queryInterface(GearInputInterface::IType type) {return 0;}
		
		/** @copydoc Object::_initialize */
		virtual void _initialize();
	protected:
		void _readBuffered();
		void _read();

		IDirectInput8* mDirectInput;
		IDirectInputDevice8* mKeyboard;
		DWORD coopSetting;

		unsigned char KeyBuffer[256];
		
		//! Internal method for translating KeyCodes to Text
		int _translateText( KeyCode kc );

		//! Stored dead key from last translation
		WCHAR deadKey;

		//! used for getAsString
		std::string mGetString;
	};
}
#endif //_WIN32_KEYBOARD_H_EADER_
