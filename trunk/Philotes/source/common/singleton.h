// ---------------------------------------------------------------------------
// FILE:		singleton.h
// DESCRIPTION:	singleton pattern
// ---------------------------------------------------------------------------
#ifndef _SINGLETON_H_
#define _SINGLETON_H_


// ---------------------------------------------------------------------------
// DESC:	M-T singleton pattern
// ---------------------------------------------------------------------------
template <typename T>
class CSingleton
{
public:
	static T* GetInstance(
		void)
	{
		if (CSingleton::instance_ == NULL)
		{
			CSingleton::cs_.Enter();
			if (CSingleton::instance_ == NULL)
			{
				CSingleton::instance_ = CreateInstance();
				ScheduleForDestruction(CSingleton::Destroy);
			}
			CSingleton::cs_.Leave();
		}
		return (T*)CSingleton::instance_;
	}

protected:
	CSingleton(
		void)
	{
		ASSERT(CSingleton::instance_ == NULL);
		CSingleton::instance_ = static_cast<T*>(this);
	}

	~CSingleton(
		void)
	{
		CSingleton::instance_ = NULL;
		CSingleton::cs_.Free();
	}

	static void * StaticInit(
		void)
	{
		CSingleton::cs_.Init();
		return NULL;
	}

private:
	static volatile T*				instance_;
	static CCritSectLite			cs_;

private:
	static T* CreateInstance(
		void)
	{
		return new T();
	};

	static void ScheduleForDestruction(
		void (*fp)())
	{
		atexit(fp);
	}

	static void Destroy(
		void)
	{
		if (CSingleton::instance_ != NULL)
		{
			DestroyInstance((T*)CSingleton::instance_);
			CSingleton::instance_ = NULL;
		}
	}

	static void DestroyInstance(
		T *p)
	{
		delete p;
	}
};


template<typename T>
CCritSectLite CSingleton<T>::cs_;

template<typename T>
typename volatile T* CSingleton<T>::instance_ = (T*)CSingleton<T>::StaticInit();


#endif  // _SINGLETON_H_