
#include "gearsInputForceFeedback.h"
using namespace Philo;

//-------------------------------------------------------------//
void GearInputForceFeedback::_addEffectTypes( GearInputEffect::EForce force, GearInputEffect::EType type )
{
	if( force == GearInputEffect::UnknownForce || type == GearInputEffect::Unknown )
		PH_EXCEPT( ERR_INPUT, "Unknown Force||Type was added too effect list..." );

	mSupportedEffects[force] = type;
}

//-------------------------------------------------------------//
const GearInputForceFeedback::SupportedEffectList& 
						GearInputForceFeedback::getSupportedEffects() const
{
	return mSupportedEffects;
}
