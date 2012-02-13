
#ifndef PI_ForceFeedBack_H
#define PI_ForceFeedBack_H

#include "gearsInputPrereqs.h"
#include "gearsInputInterface.h"
#include "gearsInputEffect.h"

namespace Philo
{
	/**
		Interface class for dealing with Force Feedback devices
	*/
	class GearInputForceFeedback : public GearInputInterface
	{
	public:
		GearInputForceFeedback() {}
		virtual ~GearInputForceFeedback() {}

		/**
		@remarks
			This is like setting the master volume of an audio device.
			Individual effects have gain levels; however, this affects all
			effects at once.
		@param level
			A value between 0.0 and 1.0 represent the percentage of gain. 1.0
			being the highest possible force level (means no scaling).
		*/
		virtual void setMasterGain( float level ) = 0;
		
		/**
		@remarks
			If using Force Feedback effects, this should be turned off
			before uploading any effects. Auto centering is the motor moving
			the joystick back to center. DirectInput only has an on/off setting,
			whereas linux has levels.. Though, we go with DI's on/off mode only
		@param auto_on
			true to turn auto centering on, false to turn off.
		*/
		virtual void setAutoCenterMode( bool auto_on ) = 0;

		/**
		@remarks
			Creates and Plays the effect immediately. If the device is full
			of effects, it will fail to be uploaded. You will know this by
			an invalid Effect Handle
		*/
		virtual void upload( const GearInputEffect* effect ) = 0;

		/**
		@remarks
			Modifies an effect that is currently playing
		*/
		virtual void modify( const GearInputEffect* effect ) = 0;

		/**
		@remarks
			Remove the effect from the device
		*/
		virtual void remove( const GearInputEffect* effect ) = 0;

		/**
		@remarks
			Get the number of supported Axes for FF usage
		*/
        virtual short getFFAxesNumber() = 0;

		typedef std::map<GearInputEffect::EForce, GearInputEffect::EType> SupportedEffectList;
		/**
		@remarks
			Get a list of all supported effects
		*/
		const SupportedEffectList& getSupportedEffects() const;

		void _addEffectTypes( GearInputEffect::EForce force, GearInputEffect::EType type );

	protected:
		SupportedEffectList mSupportedEffects;
	};
}
#endif //PI_ForceFeedBack_H
