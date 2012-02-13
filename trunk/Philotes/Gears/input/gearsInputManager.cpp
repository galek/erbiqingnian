
#include "gearsInputManager.h"
#include "gearsInputFactoryCreator.h"
#include "gearsInputObject.h"
#include <sstream>
#include <algorithm>

#include "dx/gearsInputManagerDx.h"

using namespace Philo;

//----------------------------------------------------------------------------//
GearInputManager::GearInputManager(const std::string& name) :
	mInputSystemName(name)
{
}

//----------------------------------------------------------------------------//
GearInputManager::~GearInputManager()
{

}

//----------------------------------------------------------------------------//
GearInputManager* GearInputManager::createInputSystem( std::size_t windowhandle )
{
	ParamList pl;
	std::ostringstream wnd;
	wnd << windowhandle;
	pl.insert(std::make_pair( std::string("WINDOW"), wnd.str() ));

	return createInputSystem( pl );
}

//----------------------------------------------------------------------------//
GearInputManager* GearInputManager::createInputSystem( ParamList &paramList )
{
	GearInputManager* im = 0;

	im = new Win32InputManager();

	try
	{
		im->_initialize(paramList);
	}
	catch(...)
	{
		delete im;
		throw; //rethrow
	}

	return im;
}

//----------------------------------------------------------------------------//
void GearInputManager::destroyInputSystem(GearInputManager* manager)
{
	if( manager == 0 )
		return;

	//Cleanup before deleting...
	for( FactoryCreatedObject::iterator i = manager->mFactoryObjects.begin(); 
		i != manager->mFactoryObjects.end(); ++i )
	{
		i->second->destroyObject( i->first );
	}

	manager->mFactoryObjects.clear();
	delete manager;
}

//--------------------------------------------------------------------------------//
const std::string& GearInputManager::inputSystemName()
{
	return mInputSystemName;
}

//--------------------------------------------------------------------------------//
int GearInputManager::getNumberOfDevices( Type iType )
{
	//Count up all the factories devices
	int factoyObjects = 0;
	FactoryList::iterator i = mFactories.begin(), e = mFactories.end();
	for( ; i != e; ++i )
		factoyObjects += (*i)->totalDevices(iType);

	return factoyObjects;
}

//----------------------------------------------------------------------------//
DeviceList GearInputManager::listFreeDevices()
{
	DeviceList list;
	FactoryList::iterator i = mFactories.begin(), e = mFactories.end();
	for( ; i != e; ++i )
	{
		DeviceList temp = (*i)->freeDeviceList();
		list.insert(temp.begin(), temp.end());
	}

	return list;
}

//----------------------------------------------------------------------------//
GearInputObject* GearInputManager::createInputObject( Type iType, bool bufferMode, const std::string &vendor )
{
	GearInputObject* obj = 0;
	FactoryList::iterator i = mFactories.begin(), e = mFactories.end();
	for( ; i != e; ++i)
	{
		if( (*i)->freeDevices(iType) > 0 )
		{
			if( vendor == "" || (*i)->vendorExist(iType, vendor) )
			{
				obj = (*i)->createObject(this, iType, bufferMode, vendor);
				mFactoryObjects[obj] = (*i);
				break;
			}
		}
	}

	if(!obj)
		PH_EXCEPT( ERR_INPUT, "No devices match requested type.");

	try
	{	//Intialize device
		obj->_initialize();
	}
	catch(...)
	{	//Somekind of error, cleanup and rethrow
		destroyInputObject(obj);
		throw;
	}

	return obj;
}

//----------------------------------------------------------------------------//
void GearInputManager::destroyInputObject( GearInputObject* obj )
{
	if( obj == 0 )
		return;

	FactoryCreatedObject::iterator i = mFactoryObjects.find(obj);
	if( i != mFactoryObjects.end() )
	{
		i->second->destroyObject(obj);
		mFactoryObjects.erase(i);
	}
	else
	{
		PH_EXCEPT( ERR_INPUT, "Object creator not known.");
	}
}

//----------------------------------------------------------------------------//
void GearInputManager::addFactoryCreator( GearInputFactoryCreator* factory )
{
	if(factory != 0)
		mFactories.push_back(factory);
}

//----------------------------------------------------------------------------//
void GearInputManager::removeFactoryCreator( GearInputFactoryCreator* factory )
{
	if(factory != 0)
	{
		//First, destroy all devices created with the factory
		for( FactoryCreatedObject::iterator i = mFactoryObjects.begin(); i != mFactoryObjects.end(); ++i )
		{
			if( i->second == factory )
			{
				i->second->destroyObject(i->first);
				mFactoryObjects.erase(i++);
			}
		}

		//Now, remove the factory itself
		FactoryList::iterator fact = std::find(mFactories.begin(), mFactories.end(), factory);
		if( fact != mFactories.end() )
			mFactories.erase(fact);
	}
}
