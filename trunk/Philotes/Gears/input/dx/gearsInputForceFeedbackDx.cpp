
#include "gearsInputForceFeedbackDx.h"
#include <Math.h>

#if defined (_DEBUG)
  #include <sstream>
#endif

using namespace Philo;

//--------------------------------------------------------------//
Win32ForceFeedback::Win32ForceFeedback(IDirectInputDevice8* joy) :
	mHandles(0), mJoyStick(joy)
{
}

//--------------------------------------------------------------//
Win32ForceFeedback::~Win32ForceFeedback()
{
	//Get the effect - if it exists
	for(EffectList::iterator i = mEffectList.begin(); i != mEffectList.end(); ++i )
	{
		LPDIRECTINPUTEFFECT dxEffect = i->second;
		if( dxEffect )
			dxEffect->Unload();
	}

	mEffectList.clear();
}

//--------------------------------------------------------------//
void Win32ForceFeedback::upload( const GearInputEffect* effect )
{
	switch( effect->force )
	{
		case GearInputEffect::ConstantForce: _updateConstantEffect(effect);	break;
		case GearInputEffect::RampForce: _updateRampEffect(effect);	break;
		case GearInputEffect::PeriodicForce: _updatePeriodicEffect(effect);	break;
		case GearInputEffect::ConditionalForce:	_updateConditionalEffect(effect); break;
		//case OIS::Effect::CustomForce: _updateCustomEffect(effect); break;
		default: PH_EXCEPT(ERR_INPUT, "Requested Force not Implemented yet, sorry!"); break;
	}
}

//--------------------------------------------------------------//
void Win32ForceFeedback::modify( const GearInputEffect* eff )
{
	//Modifying is essentially the same as an upload, so, just reuse that function
	upload(eff);
}

//--------------------------------------------------------------//
void Win32ForceFeedback::remove( const GearInputEffect* eff )
{
	//Get the effect - if it exists
	EffectList::iterator i = mEffectList.find(eff->_handle);
	if( i != mEffectList.end() )
	{
		LPDIRECTINPUTEFFECT dxEffect = i->second;
		if( dxEffect )
		{
			dxEffect->Stop();
			//We care about the return value - as the effect might not
			//have been unlaoded
			if( SUCCEEDED(dxEffect->Unload()) )
				mEffectList.erase(i);
		}
		else
			mEffectList.erase(i);
	}
}

//--------------------------------------------------------------//
void Win32ForceFeedback::setMasterGain( float level )
{
	//Between 0 - 10,000
	int gain_level = (int)(10000.0f * level);

	if( gain_level > 10000 )
		gain_level = 10000;
	else if( gain_level < 0 )
		gain_level = 0;

	DIPROPDWORD DIPropGain;
	DIPropGain.diph.dwSize       = sizeof(DIPropGain);
	DIPropGain.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	DIPropGain.diph.dwObj        = 0;
	DIPropGain.diph.dwHow        = DIPH_DEVICE;
	DIPropGain.dwData            = gain_level;

	mJoyStick->SetProperty(DIPROP_FFGAIN, &DIPropGain.diph);
}

//--------------------------------------------------------------//
void Win32ForceFeedback::setAutoCenterMode( bool auto_on )
{
	//DI Property DIPROPAUTOCENTER_OFF = 0, 1 is on
	DIPROPDWORD DIPropAutoCenter;
	DIPropAutoCenter.diph.dwSize       = sizeof(DIPropAutoCenter);
	DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	DIPropAutoCenter.diph.dwObj        = 0;
	DIPropAutoCenter.diph.dwHow        = DIPH_DEVICE;
	DIPropAutoCenter.dwData            = auto_on;

	//hr =
	mJoyStick->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph);
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_updateConstantEffect( const GearInputEffect* effect )
{
	DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
	LONG            rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf;
	DIEFFECT        diEffect;

	//Currently only support 1 axis
	//if( effect->getNumAxes() == 1 )
	cf.lMagnitude = static_cast<ConstantEffect*>(effect->getForceEffect())->level;

	_setCommonProperties(&diEffect, rgdwAxes, rglDirection, sizeof(DICONSTANTFORCE), &cf, effect);
	_upload(GUID_ConstantForce, &diEffect, effect);
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_updateRampEffect( const GearInputEffect* effect )
{
	DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
	LONG            rglDirection[2] = { 0, 0 };
	DIRAMPFORCE     rf;
	DIEFFECT        diEffect;

	//Currently only support 1 axis
	rf.lStart = static_cast<RampEffect*>(effect->getForceEffect())->startLevel;
	rf.lEnd = static_cast<RampEffect*>(effect->getForceEffect())->endLevel;

	_setCommonProperties(&diEffect, rgdwAxes, rglDirection, sizeof(DIRAMPFORCE), &rf, effect);
	_upload(GUID_RampForce, &diEffect, effect);
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_updatePeriodicEffect( const GearInputEffect* effect )
{
	DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
	LONG            rglDirection[2] = { 0, 0 };
	DIPERIODIC      pf;
	DIEFFECT        diEffect;

	//Currently only support 1 axis
	pf.dwMagnitude = static_cast<PeriodicEffect*>(effect->getForceEffect())->magnitude;
	pf.lOffset = static_cast<PeriodicEffect*>(effect->getForceEffect())->offset;
	pf.dwPhase = static_cast<PeriodicEffect*>(effect->getForceEffect())->phase;
	pf.dwPeriod = static_cast<PeriodicEffect*>(effect->getForceEffect())->period;

	_setCommonProperties(&diEffect, rgdwAxes, rglDirection, sizeof(DIPERIODIC), &pf, effect);

	switch( effect->type )
	{
	case GearInputEffect::Square: _upload(GUID_Square, &diEffect, effect); break;
	case GearInputEffect::Triangle: _upload(GUID_Triangle, &diEffect, effect); break;
	case GearInputEffect::Sine: _upload(GUID_Sine, &diEffect, effect); break;
	case GearInputEffect::SawToothUp: _upload(GUID_SawtoothUp, &diEffect, effect); break;
	case GearInputEffect::SawToothDown:	_upload(GUID_SawtoothDown, &diEffect, effect); break;
	default: break;
	}
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_updateConditionalEffect( const GearInputEffect* effect )
{
	DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
	LONG            rglDirection[2] = { 0, 0 };
	DICONDITION     cf;
	DIEFFECT        diEffect;

	cf.lOffset = static_cast<ConditionalEffect*>(effect->getForceEffect())->deadband;
	cf.lPositiveCoefficient = static_cast<ConditionalEffect*>(effect->getForceEffect())->rightCoeff;
	cf.lNegativeCoefficient = static_cast<ConditionalEffect*>(effect->getForceEffect())->leftCoeff;
	cf.dwPositiveSaturation = static_cast<ConditionalEffect*>(effect->getForceEffect())->rightSaturation;
	cf.dwNegativeSaturation = static_cast<ConditionalEffect*>(effect->getForceEffect())->leftSaturation;
	cf.lDeadBand = static_cast<ConditionalEffect*>(effect->getForceEffect())->deadband;

	_setCommonProperties(&diEffect, rgdwAxes, rglDirection, sizeof(DICONDITION), &cf, effect);

	switch( effect->type )
	{
	case GearInputEffect::Friction:	_upload(GUID_Friction, &diEffect, effect); break;
	case GearInputEffect::Damper: _upload(GUID_Damper, &diEffect, effect); break;
	case GearInputEffect::Inertia: _upload(GUID_Inertia, &diEffect, effect); break;
	case GearInputEffect::Spring: _upload(GUID_Spring, &diEffect, effect); break;
	default: break;
	}
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_updateCustomEffect( const GearInputEffect* /*effect*/ )
{
	
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_setCommonProperties(
		DIEFFECT* diEffect, DWORD* rgdwAxes,
		LONG* rglDirection, DWORD struct_size,
		LPVOID struct_type, const GearInputEffect* effect )
{
	ZeroMemory(diEffect, sizeof(DIEFFECT));

	diEffect->dwSize                  = sizeof(DIEFFECT);
	diEffect->dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	diEffect->dwDuration              = effect->replay_length;
	diEffect->dwSamplePeriod          = 0;
	diEffect->dwGain                  = DI_FFNOMINALMAX;
	diEffect->dwTriggerButton         = DIEB_NOTRIGGER;
	diEffect->dwTriggerRepeatInterval = 0;
	diEffect->cAxes                   = effect->getNumAxes();
	diEffect->rgdwAxes                = rgdwAxes;
	diEffect->rglDirection            = rglDirection;
	diEffect->lpEnvelope              = 0;
	diEffect->cbTypeSpecificParams    = struct_size;
	diEffect->lpvTypeSpecificParams   = struct_type;
	diEffect->dwStartDelay            = effect->replay_delay;
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_upload( GUID guid, DIEFFECT* diEffect, const GearInputEffect* effect)
{
	LPDIRECTINPUTEFFECT dxEffect = 0;

	//Get the effect - if it exists
	EffectList::iterator i = mEffectList.find(effect->_handle);
	//It has been created already
	if( i != mEffectList.end() )
		dxEffect = i->second;
	else //This effect has not yet been created - generate a handle
		effect->_handle = mHandles++;

	if( dxEffect == 0 )
	{
		//This effect has not yet been created, so create it
		HRESULT hr = mJoyStick->CreateEffect(guid, diEffect, &dxEffect, NULL);
		if(SUCCEEDED(hr))
		{
			mEffectList[effect->_handle] = dxEffect;
			dxEffect->Start(INFINITE,0);
		}
		else if( hr == DIERR_DEVICEFULL )
		{
			PH_EXCEPT(ERR_INPUT, "Remove an effect before adding more!");
		}
		else
		{
			PH_EXCEPT(ERR_INPUT, "Unknown error creating effect->..");
		}
	}
	else
	{
		//ToDo -- Update the Effect
		HRESULT hr = dxEffect->SetParameters( diEffect, DIEP_DIRECTION |
			DIEP_DURATION | DIEP_ENVELOPE | DIEP_STARTDELAY | DIEP_TRIGGERBUTTON |
			DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS | DIEP_START );

		if(FAILED(hr)) 
		{
			PH_EXCEPT(ERR_INPUT, "Error updating device!");
		}
	}
}

//--------------------------------------------------------------//
void Win32ForceFeedback::_addEffectSupport( LPCDIEFFECTINFO pdei )
{
	//Determine what the effect is and how it corresponds to our OIS's Enums
	//We could save the GUIDs too, however, we will just use the predefined
	//ones later
	if( pdei->guid == GUID_ConstantForce )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Constant );
	else if( pdei->guid == GUID_Triangle )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Triangle );
	else if( pdei->guid == GUID_Spring )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Spring );
	else if( pdei->guid == GUID_Friction )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Friction );
	else if( pdei->guid == GUID_Square )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Square );
	else if( pdei->guid == GUID_Sine )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Sine );
	else if( pdei->guid == GUID_SawtoothUp )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::SawToothUp );
	else if( pdei->guid == GUID_SawtoothDown )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::SawToothDown );
	else if( pdei->guid == GUID_Damper )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Damper );
	else if( pdei->guid == GUID_Inertia )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Inertia );
	else if( pdei->guid == GUID_CustomForce )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Custom );
	else if( pdei->guid == GUID_RampForce )
		_addEffectTypes((GearInputEffect::EForce)DIEFT_GETTYPE(pdei->dwEffType), GearInputEffect::Ramp );
}
