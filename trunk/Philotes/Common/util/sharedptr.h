
#pragma once

#include <utility>

namespace Philo 
{
	template<class T> class SharedPtr
	{
	protected:
		T* pRep;
		unsigned int* pUseCount;
	public:
		
		SharedPtr() : pRep(0), pUseCount(0)
        {
        }

        template< class Y>
		explicit SharedPtr(Y* rep) 
			: pRep(rep)
			, pUseCount(rep ? new unsigned int(1) : 0)
		{
		}
		SharedPtr(const SharedPtr& r)
            : pRep(0), pUseCount(0)
		{
			// lock & copy other mutex pointer
            
			pRep = r.pRep;
			pUseCount = r.pUseCount; 

			if(pUseCount)
			{
				++(*pUseCount); 
			}
		}
		SharedPtr& operator=(const SharedPtr& r) {
			if (pRep == r.pRep)
				return *this;
			// Swap current data into a local copy
			// this ensures we deal with rhs and this being dependent
			SharedPtr<T> tmp(r);
			swap(tmp);
			return *this;
		}
		
		template< class Y>
		SharedPtr(const SharedPtr<Y>& r)
            : pRep(0), pUseCount(0)
		{
			// lock & copy other mutex pointer
			pRep = r.getPointer();
			pUseCount = r.useCountPointer();
			// Handle zero pointer gracefully to manage STL containers
			if(pUseCount)
			{
				++(*pUseCount);
			}
		}
		template< class Y>
		SharedPtr& operator=(const SharedPtr<Y>& r) {
			if (pRep == r.getPointer())
				return *this;
			// Swap current data into a local copy
			// this ensures we deal with rhs and this being dependent
			SharedPtr<T> tmp(r);
			swap(tmp);
			return *this;
		}
		virtual ~SharedPtr() {
            release();
		}


		inline T& operator*() const { ph_assert(pRep); return *pRep; }
		inline T* operator->() const { ph_assert(pRep); return pRep; }
		inline T* get() const { return pRep; }

		/** Binds rep to the SharedPtr.
			@remarks
				Assumes that the SharedPtr is uninitialised!
		*/
		void bind(T* rep) 
		{
			ph_assert(!pRep && !pUseCount);
			pUseCount = new unsigned int(1);
			pRep = rep;
		}

		inline bool unique() const 
		{ 
			ph_assert(pUseCount); 
			return *pUseCount == 1;
		}

		inline unsigned int useCount() const 
		{ 
			ph_assert(pUseCount); 
			return *pUseCount; 
		}

		inline unsigned int* useCountPointer() const { return pUseCount; }

		inline T* getPointer() const { return pRep; }
		
		inline bool isNull(void) const { return pRep == 0; }

        inline void setNull(void) { 
			if (pRep)
			{
				release();
				pRep = 0;
				pUseCount = 0;
			}
        }

    protected:

        inline void release(void)
        {
			bool destroyThis = false;

            /* If the mutex is not initialized to a non-zero value, then
               neither is pUseCount nor pRep.
             */
			if (pUseCount)
			{
				if (--(*pUseCount) == 0) 
				{
					destroyThis = true;
				}
			}

			if (destroyThis)
				destroy();
        }

        virtual void destroy(void)
        {
			delete pRep;
			delete pUseCount;
        }

		virtual void swap(SharedPtr<T> &other) 
		{
			std::swap(pRep, other.pRep);
			std::swap(pUseCount, other.pUseCount);
		}
	};

	template<class T, class U> inline bool operator==(SharedPtr<T> const& a, SharedPtr<U> const& b)
	{
		return a.get() == b.get();
	}

	template<class T, class U> inline bool operator!=(SharedPtr<T> const& a, SharedPtr<U> const& b)
	{
		return a.get() != b.get();
	}

	template<class T, class U> inline bool operator<(SharedPtr<T> const& a, SharedPtr<U> const& b)
	{
		return std::less<const void*>()(a.get(), b.get());
	}
}

#endif
