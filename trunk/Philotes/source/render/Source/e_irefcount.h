//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#ifndef __E_IREFCOUNT_H__
#define __E_IREFCOUNT_H__

#pragma warning(push)
#pragma warning(disable:4481)	// warning C4481: nonstandard extension used: override specifier 'override'

// COM-based reference counting base object.
class IRefCount : public IUnknown
{
	public:
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void * * ppvObject);
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
		virtual ULONG STDMETHODCALLTYPE Release(void) override;

	protected:
		IRefCount(void) throw();
		virtual ~IRefCount(void);

	private:
		LONG nRefs;
};

#pragma warning(pop)

#endif
