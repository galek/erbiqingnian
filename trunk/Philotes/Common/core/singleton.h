#pragma once

_NAMESPACE_BEGIN

template <typename T> class Singleton
{
private:
	/** \brief Explicit private copy constructor. This is a forbidden operation.*/
	Singleton(const Singleton<T> &);

	/** \brief Private operator= . This is a forbidden operation. */
	Singleton& operator=(const Singleton<T> &);

protected:

	static T* msSingleton;

public:
	Singleton( void )
	{
		ph_assert( !msSingleton );
		msSingleton = static_cast< T* >( this );
	}

	~Singleton( void )
	{  
		ph_assert( msSingleton ); 
		msSingleton = 0;  
	}
	
	static T* getSingleton( void )
	{
		return msSingleton; 
	}
};

#define _DECLARE_SINGLETON(cls) static cls* getSingleton(void)

#define _IMPLEMENT_SINGLETON(cls) template<> cls* Singleton<cls>::msSingleton = 0;\
cls* cls::getSingleton(void){return msSingleton;}

_NAMESPACE_END