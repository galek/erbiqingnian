
#pragma once

#include "gearsInputPrereqs.h"
#include "gearsInputInterface.h"

namespace Philo
{
	/**	The base class of all input types. */
	class GearInputObject
	{
	public:
		virtual ~GearInputObject() {}

		/**	@remarks Get the type of device	*/
		Type type() const { return mType; }

		/**	@remarks Get the vender string name	*/
		const std::string& vendor() const { return mVendor; }

		/**	@remarks Get buffered mode - true is buffered, false otherwise */
		virtual bool buffered() const { return mBuffered; }

		/** @remarks Returns this input object's creator */
		GearInputManager* getCreator() { return mCreator; }

		/** @remarks Sets buffered mode	*/
		virtual void setBuffered(bool buffered) = 0;

		/**	@remarks Used for updating call once per frame before checking state or to update events */
		virtual void capture() = 0;

		/**	@remarks This may/may not) differentiate the different controllers based on (for instance) a port number (useful for console InputManagers) */
		virtual int getID() const {return mDevID;}

		/**
		@remarks
			If available, get an interface to write to some devices.
			Examples include, turning on and off LEDs, ForceFeedback, etc
		@param type
			The type of interface you are looking for
		*/
		virtual GearInputInterface* queryInterface(GearInputInterface::IType type) = 0;

		/**	@remarks Internal... Do not call this directly. */
		virtual void _initialize() = 0;

	protected:

		GearInputObject(const std::string &vendor, Type iType, bool buffered,
			   int devID, GearInputManager* creator) :
					mVendor(vendor),
					mType(iType),
					mBuffered(buffered),
					mDevID(devID),
					mCreator(creator) {}

		//! Vendor name if applicable/known
		std::string mVendor;

		//! Type of controller object
		Type mType;

		//! Buffered flag
		bool mBuffered;

		//! Not fully implemented yet
		int mDevID;

		//! The creator who created this object
		GearInputManager* mCreator;
	};
}
