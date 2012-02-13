
#include "gearsInputEffect.h"

using namespace Philo;

//VC7.1 had a problem with these not getting included.. 
//Perhaps a case of a crazy extreme optimizer :/ (moved to header)
//const unsigned int Effect::PI_INFINITE = 0xFFFFFFFF;

//------------------------------------------------------------------------------//
GearInputEffect::GearInputEffect() : 
	force(UnknownForce), 
	type(Unknown),
	effect(0),
	axes(1)
{
}

//------------------------------------------------------------------------------//
GearInputEffect::GearInputEffect(EForce ef, EType et) : 
	force(ef), 
	type(et),
	direction(North), 
	trigger_button(-1),
	trigger_interval(0),
	replay_length(GearInputEffect::PI_INFINITE),
	replay_delay(0),
	_handle(-1),
	axes(1)
{
	effect = 0;

	switch( ef )
	{
	case ConstantForce:    effect = new ConstantEffect(); break;
	case RampForce:	       effect = new RampEffect(); break;
	case PeriodicForce:    effect = new PeriodicEffect(); break;
	case ConditionalForce: effect = new ConditionalEffect(); break;
	default: break;
	}
}

//------------------------------------------------------------------------------//
GearInputEffect::~GearInputEffect()
{
	delete effect;
}

//------------------------------------------------------------------------------//
ForceEffect* GearInputEffect::getForceEffect() const
{
	//If no effect was created in constructor, then we raise an error here
	if( effect == 0 )
		PH_EXCEPT( ERR_INPUT, "Requested ForceEffect is null!" );

	return effect;
}

//------------------------------------------------------------------------------//
void GearInputEffect::setNumAxes(short nAxes)
{
	//Can only be set before a handle was assigned (effect created)
	if( _handle != -1 )
        axes = nAxes;
}

//------------------------------------------------------------------------------//
short GearInputEffect::getNumAxes() const
{
	return axes;
}
