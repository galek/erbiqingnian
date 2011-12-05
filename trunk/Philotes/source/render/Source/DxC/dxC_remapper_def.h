//----------------------------------------------------------------------------
// dx10_remapper_def
// DX10 structures extended with DX9 accessors
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __DX10_REMAPPER_DEF__
#define __DX10_REMAPPER_DEF__

//Remapper variable overrides assignment and conversion operators to point back to mapped variable
//this allows us to use multiple names for the same variable
template <typename mappedType>
class remapperVar 
{
private:
	mappedType* hidden;
public:
	void set( mappedType* map )
	{
		hidden = map;
	}
	mappedType operator=( const mappedType &input )
	{
		ASSERTX( hidden,  "Mapped address is NULL!  Did you call ZeroMemory on this struct?  This struct is remapped you must call SetRemappers() after zeroing it" );
		return *hidden = input;
	}
	operator const mappedType() const	// CHB 2007.01.25 - Allow conversions for both const and mutable objects.
	{
		ASSERTX( hidden, "Mapped address is NULL!  Did you call ZeroMemory on this struct?  This struct is remapped you must call SetRemappers() after zeroing it" );
		return *hidden;
	}
};

//This defines the default constructor, copy constructor and assignment operator so that the remapper variables are properly configured
// you must define void setRemappers() and set all your remapper variables to the variable they are to map
#define REMAPPER_CONSTRUCTOR( type )\
	type()\
	{\
		setRemappers();\
	}\
	type( const type& input )\
	{\
		*this = input;\
	}\
	type& operator = ( const type& input )\
	{\
		memcpy( this, &input, sizeof( type ) );\
		setRemappers();\
		return *this;\
	}

#ifdef ENGINE_TARGET_DX9
#endif

#ifdef ENGINE_TARGET_DX10
 
struct D3DPRESENT_PARAMETERS : public DXGI_SWAP_CHAIN_DESC
{
public:
//	remapperVar<UINT> BackBufferHeight;
//	remapperVar<UINT> BackBufferWidth;
	remapperVar<DXGI_FORMAT> BackBufferFormat;
	remapperVar<UINT> MultiSampleType;
	remapperVar<UINT> MultiSampleQuality;
//	remapperVar<UINT> FullScreen_RefreshRateInHz;
	remapperVar<HWND> hDeviceWindow;

	REMAPPER_CONSTRUCTOR( D3DPRESENT_PARAMETERS )

	void setRemappers()
	{
//		BackBufferHeight.set( &BufferDesc.Height );
//		BackBufferWidth.set( &BufferDesc.Width );
		BackBufferFormat.set( &BufferDesc.Format );
		MultiSampleType.set( &SampleDesc.Count );
		MultiSampleQuality.set( &SampleDesc.Quality );
//		FullScreen_RefreshRateInHz.set( &BufferDesc.RefreshRate.Numerator );
//		BufferDesc.RefreshRate.Denominator = 1;
		hDeviceWindow.set( &OutputWindow );
	}
};

struct D3DLOCKED_RECT : public D3D10_MAPPED_TEXTURE2D
{
public:
	remapperVar<UINT> Pitch;
	remapperVar<void*> pBits;

	REMAPPER_CONSTRUCTOR( D3DLOCKED_RECT )

	void setRemappers()
	{
		Pitch.set( &RowPitch );
		pBits.set( &pData );
	}
};

#endif

#endif
