
#ifndef PI_Win32ForceFeedBack_H
#define PI_Win32ForceFeedBack_H

#include "../gearsInputPrereqs.h"
#include "../gearsInputForceFeedback.h"
#include "gearsInputPrereqsDx.h"

namespace Philo
{
	class Win32ForceFeedback : public GearInputForceFeedback
	{
		Win32ForceFeedback() {}
	public:
		Win32ForceFeedback(IDirectInputDevice8* joy);
		~Win32ForceFeedback();

		/** @copydoc ForceFeedback::upload */
		void upload( const GearInputEffect* effect );

		/** @copydoc ForceFeedback::modify */
		void modify( const GearInputEffect* effect );

		/** @copydoc ForceFeedback::remove */
		void remove( const GearInputEffect* effect );

		/** @copydoc ForceFeedback::setMasterGain */
		void setMasterGain( float level );
		
		/** @copydoc ForceFeedback::setAutoCenterMode */
		void setAutoCenterMode( bool auto_on );

		/** @copydoc ForceFeedback::getFFAxesNumber 
			xxx todo - Actually return correct number
		*/
		short getFFAxesNumber() {return 1;}

		/**
			@remarks
			Internal use.. Used during enumeration to build a list of a devices
			support effects.
		*/
		void _addEffectSupport( LPCDIEFFECTINFO pdei );

	protected:
		//Specific Effect Settings
		void _updateConstantEffect( const GearInputEffect* effect );
		void _updateRampEffect( const GearInputEffect* effect );
		void _updatePeriodicEffect( const GearInputEffect* effect );
		void _updateConditionalEffect( const GearInputEffect* effect );
		void _updateCustomEffect( const GearInputEffect* effect );
		//Sets the common properties to all effects
		void _setCommonProperties( DIEFFECT* diEffect, DWORD* rgdwAxes,
									LONG* rglDirection, DWORD struct_size, 
									LPVOID struct_type, const GearInputEffect* effect );
		//Actually do the upload
		void _upload( GUID, DIEFFECT*, const GearInputEffect* );

		typedef std::map<int,LPDIRECTINPUTEFFECT> EffectList;
		EffectList mEffectList;
		//Simple unique handle creation - allows for upto 2+ million effects
		//during the lifetime of application. Hopefully, that is enough.
		int mHandles;

		IDirectInputDevice8* mJoyStick;
	};
}
#endif //PI_Win32ForceFeedBack_H
