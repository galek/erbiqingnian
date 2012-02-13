
#ifndef _PI_EVENTHEADERS_
#define _PI_EVENTHEADERS_

#include "gearsInputPrereqs.h"

namespace Philo
{
	class EventArg
	{
	public:
		EventArg( GearInputObject* obj ) : device(obj) {}
		virtual ~EventArg() {}

		//! Pointer to the Input Device
		const GearInputObject* device;
	};
}

#endif //_PI_EVENTHEADERS_
