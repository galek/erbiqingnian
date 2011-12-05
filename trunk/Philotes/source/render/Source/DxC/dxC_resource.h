//----------------------------------------------------------------------------
// dxC_resource.h
//
// - Header for resource functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_RESOURCE__
#define __DXC_RESOURCE__

#include "array.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DXCRHANDLE_TYPE_BITS			4
#define DXCRHANDLE_TYPE_MASK			0xf0000000

#define MAKE_DXCRHANDLE(type,id)		(DXCRHANDLE)( ( type << (32-DXCRHANDLE_TYPE_BITS) ) + ( id & (~DXCRHANDLE_TYPE_MASK) ) )

#define INVALID_DXCRHANDLE				-1L

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

// values map to D3DRESOURCETYPE
enum DXCRESOURCE_TYPE
{
	DXCRTYPE_INVALID			= -1,
	_DXCRTYPE_SURFACE			=  1,	// for mapping and explanatory purposes only
	_DXCRTYPE_VOLUME			=  2,
	DXCRTYPE_TEXTURE2D			=  3,
	DXCRTYPE_TEXTURE3D			=  4,
	DXCRTYPE_TEXTURECUBE		=  5,
	DXCRTYPE_VERTEXBUFFER		=  6,
	DXCRTYPE_INDEXBUFFER		=  7,
	// count
	NUM_DXC_RESOURCE_TYPES
};

enum DXCRESOURCE_STATE
{
	DXCRSTATE_INVALID		= -1,
	DXCRSTATE_NOT_LOADED	= 0,
	DXCRSTATE_LOADED,
};

enum DXCRESOURCE_PRIORITY
{
#if defined(ENGINE_TARGET_DX9)
	DXCRPRIORITY_NORMAL		= 0,
	DXCRPRIORITY_HIGH		= 1,
#else
	DXCRPRIORITY_NORMAL		= DXGI_RESOURCE_PRIORITY_NORMAL,
	DXCRPRIORITY_HIGH		= DXGI_RESOURCE_PRIORITY_MAXIMUM,
#endif
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef DWORD DXCRHANDLE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

template < class T >
struct DXC_RESOURCE
{
	// for CHash
	int						nId;
	DXC_RESOURCE<T>*		pNext;

protected:
	CComPtr<T>				m_pResource;

	DXCRHANDLE				m_hThis;
	DXCRESOURCE_TYPE		m_eType;
	DXCRESOURCE_STATE		m_eState;
	DXCRESOURCE_PRIORITY	m_ePriority;
	DXCRHANDLE				m_hDefault;

public:
	void Init( DXCRESOURCE_TYPE eType )
	{
		m_hThis			= INVALID_DXCRHANDLE;
		m_hDefault		= INVALID_DXCRHANDLE;
		m_eType			= eType;
		m_eState		= DXCRSTATE_INVALID;
		m_ePriority		= DXCRPRIORITY_NORMAL;
	}

	void SetPriority( DXCRESOURCE_PRIORITY ePriority )
	{
		m_ePriority = ePriority;
		DX9_BLOCK( m_pResource->SetPriority( m_ePriority ); )
		DX10_BLOCK( V( m_pResource->SetEvictionPriority( m_ePriority ) ); )
	}
	void PreLoad()
	{
		DX9_BLOCK( m_pResource->PreLoad(); )
	}

	void SetState( DXCRESOURCE_STATE eState )	{ m_eState = eState; };
	void SetHandle( DXCRHANDLE hNew )			{ m_hThis = hNew; }
	void SetDefault( DXCRHANDLE hDefault )		{ m_hDefault = hDefault; }
	void SetPointer( T* pNew )					{ m_pResource = pNew; }

	DXCRHANDLE GetHandle()						{ return m_hThis; }
	DXCRESOURCE_TYPE GetType()					{ return m_eType; }
	DXCRESOURCE_STATE GetState()				{ return m_eState; }
	DXCRESOURCE_PRIORITY GetPriority()			{ return m_ePriority; }
	DXCRHANDLE GetDefault()						{ return m_hDefault; }
	T* GetPointer()								{ return m_pResource; }

	void Release()								{ m_pResource = NULL; }
};


#if defined(ENGINE_TARGET_DX9)
typedef struct DXC_RESOURCE<IDirect3DTexture9>			DXC_RESOURCE_TEXTURE2D;
typedef struct DXC_RESOURCE<IDirect3DCubeTexture9>		DXC_RESOURCE_TEXTURECUBE;
typedef struct DXC_RESOURCE<IDirect3DVolumeTexture9>	DXC_RESOURCE_TEXTURE3D;
typedef struct DXC_RESOURCE<IDirect3DVertexBuffer9>		DXC_RESOURCE_VERTEXBUFFER;
typedef struct DXC_RESOURCE<IDirect3DIndexBuffer9>		DXC_RESOURCE_INDEXBUFFER;
#elif defined(ENGINE_TARGET_DX10)
typedef struct DXC_RESOURCE<ID3D10Texture2D>			DXC_RESOURCE_TEXTURE2D;
typedef struct DXC_RESOURCE<ID3D10Texture2D>			DXC_RESOURCE_TEXTURECUBE;
typedef struct DXC_RESOURCE<ID3D10Texture3D>			DXC_RESOURCE_TEXTURE3D;
typedef struct DXC_RESOURCE<ID3D10Buffer>				DXC_RESOURCE_VERTEXBUFFER;
typedef struct DXC_RESOURCE<ID3D10Buffer>				DXC_RESOURCE_INDEXBUFFER;
#endif


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline DXCRESOURCE_TYPE dxC_ResHandleGetType( DXCRHANDLE hRes )
{
	return (DXCRESOURCE_TYPE)( hRes >> (32-DXCRHANDLE_TYPE_BITS) );
}

inline DXCRESOURCE_TYPE dxC_ResHandleGetIndex( DXCRHANDLE hRes )
{
	return (DXCRESOURCE_TYPE)( hRes & (~DXCRHANDLE_TYPE_MASK) );
}

//----------------------------------------------------------------------------

inline DXC_RESOURCE_TEXTURE2D * dxC_ResourceGetTexture2D( DXCRHANDLE hRes )
{
	extern CHash<DXC_RESOURCE_TEXTURE2D> gtResTexture2D;
#ifdef _DEBUG
	ASSERT( dxC_ResHandleGetType( hRes ) == DXCRTYPE_TEXTURE2D );
#endif
	return HashGet( gtResTexture2D, dxC_ResHandleGetIndex( hRes ) );
}


inline DXC_RESOURCE_TEXTURECUBE * dxC_ResourceGetTextureCube( DXCRHANDLE hRes )
{
	extern CHash<DXC_RESOURCE_TEXTURECUBE> gtResTextureCube;
#ifdef _DEBUG
	ASSERT( dxC_ResHandleGetType( hRes ) == DXCRTYPE_TEXTURECUBE );
#endif
	return HashGet( gtResTextureCube, dxC_ResHandleGetIndex( hRes ) );
}


inline DXC_RESOURCE_TEXTURE3D * dxC_ResourceGetTexture3D( DXCRHANDLE hRes )
{
	extern CHash<DXC_RESOURCE_TEXTURE3D> gtResTexture3D;
#ifdef _DEBUG
	ASSERT( dxC_ResHandleGetType( hRes ) == DXCRTYPE_TEXTURE3D );
#endif
	return HashGet( gtResTexture3D, dxC_ResHandleGetIndex( hRes ) );
}


inline DXC_RESOURCE_VERTEXBUFFER * dxC_ResourceGetVertexBuffer( DXCRHANDLE hRes )
{
	extern CHash<DXC_RESOURCE_VERTEXBUFFER> gtResVertexBuffer;
#ifdef _DEBUG
	ASSERT( dxC_ResHandleGetType( hRes ) == DXCRTYPE_VERTEXBUFFER );
#endif
	return HashGet( gtResVertexBuffer, dxC_ResHandleGetIndex( hRes ) );
}


inline DXC_RESOURCE_INDEXBUFFER * dxC_ResourceGetIndexBuffer( DXCRHANDLE hRes )
{
	extern CHash<DXC_RESOURCE_INDEXBUFFER> gtResIndexBuffer;
#ifdef _DEBUG
	ASSERT( dxC_ResHandleGetType( hRes ) == DXCRTYPE_INDEXBUFFER );
#endif
	return HashGet( gtResIndexBuffer, dxC_ResHandleGetIndex( hRes ) );
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_ResourceSystemInit();
PRESULT dxC_ResourceSystemDestroy();

void dxC_ResourceDestroy( DXCRHANDLE hDestroy );

DXCRHANDLE dxC_ResourceCreate( DXCRESOURCE_TYPE eType );

#endif //__DXC_RESOURCE__