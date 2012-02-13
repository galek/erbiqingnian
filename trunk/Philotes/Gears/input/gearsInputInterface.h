
#ifndef PI_Interface_H
#define PI_Interface_H

#include "gearsInputPrereqs.h"

namespace Philo
{
	/**
		An Object's interface is a way to gain write access to devices which support
		it. For example, force feedack.
	*/
	class GearInputInterface
	{
	public:
		virtual ~GearInputInterface() {};

		//! Type of Interface
		enum IType
		{
			ForceFeedback,
			Reserved
		};
	};
}
#endif //PI_Interface_H
