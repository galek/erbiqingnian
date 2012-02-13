
#ifndef PI_InputManager_H
#define PI_InputManager_H

#include "gearsInputPrereqs.h"

namespace Philo
{
	class GearInputManager
	{
	public:

		static GearInputManager* createInputSystem( std::size_t winHandle );
		
		static GearInputManager* createInputSystem( ParamList &paramList );

		static void destroyInputSystem(GearInputManager* manager);

		const std::string& inputSystemName();

		int getNumberOfDevices( Type iType );

		DeviceList listFreeDevices();

		GearInputObject* createInputObject( Type iType, bool bufferMode, const std::string &vendor = "");

		void destroyInputObject( GearInputObject* obj );

		void addFactoryCreator( GearInputFactoryCreator* factory );

		void removeFactoryCreator( GearInputFactoryCreator* factory );

	protected:
		
		virtual void _initialize(ParamList &paramList) = 0;

		GearInputManager(const std::string& name);

		virtual ~GearInputManager();

		FactoryList mFactories;

		FactoryCreatedObject mFactoryObjects;

		const std::string mInputSystemName;
	};
}
#endif
