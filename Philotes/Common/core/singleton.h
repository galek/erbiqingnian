#pragma once
//------------------------------------------------------------------------------
/**
    @class Core::Singleton
  
    Implements a system specific Singleton
    
    Provides helper macros to implement singleton objects:
    
    - __DeclareSingleton      put this into class declaration
    - __ImplementSingleton    put this into the implemention file
    - __ConstructSingleton    put this into the constructor
    - __DestructSingleton     put this into the destructor

    Get a pointer to a singleton object using the static Instance() method:

    Core::Server* coreServer = Core::Server::Instance();
    
    (C)2012 PhiloLabs
*/

#include "core/types.h"

namespace Core
{

	template <typename T> class Singleton
	{
	private:

		Singleton(const Singleton<T> &);

		Singleton& operator=(const Singleton<T> &);

	protected:

		static T* msSingleton;

	public:
		Singleton()
		{
			ph_assert( !msSingleton );
			msSingleton = static_cast< T* >( this );
		}

		~Singleton()
		{  ph_assert( msSingleton );  msSingleton = 0;  }

		static T* Instance()
		{ return msSingleton; }

		static bool HasInstance()
		{ return 0 != msSingleton; }

	};

}

//------------------------------------------------------------------------------
#define __DeclareSingleton(type) \
public: \
    ThreadLocal static type * Singleton; \
    static type * Instance() { ph_assert(0 != Singleton); return Singleton; }; \
    static bool HasInstance() { return 0 != Singleton; }; \
private:

#define __DeclareInterfaceSingleton(type) \
public: \
    static type * Singleton; \
    static type * Instance() { ph_assert(0 != Singleton); return Singleton; }; \
    static bool HasInstance() { return 0 != Singleton; }; \
private:

#define __ImplementSingleton(type) \
    ThreadLocal type * type::Singleton = 0;

#define __ImplementInterfaceSingleton(type) \
    type * type::Singleton = 0;

#define __ConstructSingleton \
    ph_assert(0 == Singleton); Singleton = this;

#define __ConstructInterfaceSingleton \
    ph_assert(0 == Singleton); Singleton = this;

#define __DestructSingleton \
    ph_assert(Singleton); Singleton = 0;

#define __DestructInterfaceSingleton \
    ph_assert(Singleton); Singleton = 0;
//------------------------------------------------------------------------------
