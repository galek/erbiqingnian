
#ifndef PI_FactoryCreator_H
#define PI_FactoryCreator_H

#include "gearsInputPrereqs.h"

namespace Philo
{
	class GearInputFactoryCreator
	{
	public:
		/**
			@remarks Virtual Destructor
		*/
		virtual ~GearInputFactoryCreator() {};

		/**
			@remarks Return a list of all unused devices the factory maintains
		*/
		virtual DeviceList freeDeviceList() = 0;

		/**
			@remarks Number of total devices of requested type
			@param iType Type of devices to check
		*/
		virtual int totalDevices(Type iType) = 0;

		/**
			@remarks Number of free devices of requested type
			@param iType Type of devices to check
		*/
		virtual int freeDevices(Type iType) = 0;

		/**
			@remarks Does a Type exist with the given vendor name
			@param iType Type to check
			@param vendor Vendor name to test
		*/
		virtual bool vendorExist(Type iType, const std::string & vendor) = 0;

		/**
			@remarks Creates the object
			@param iType Type to create
			@param bufferMode True to setup for buffered events
			@param vendor Create a device with the vendor name, "" means vendor name is unimportant
		*/
		virtual GearInputObject* createObject(GearInputManager* creator, Type iType, bool bufferMode, const std::string & vendor = "") = 0;

		/**
			@remarks Destroys object
			@param obj Object to destroy
		*/
		virtual void destroyObject(GearInputObject* obj) = 0;
	};
}
#endif //PI_FactoryCreator_H
