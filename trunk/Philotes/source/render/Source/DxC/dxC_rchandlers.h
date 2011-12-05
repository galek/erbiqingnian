//----------------------------------------------------------------------------
// dxC_rchandlers.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_RCHANDLERS__
#define __DXC_RCHANDLERS__

#include "dxC_target.h"
#include "dxC_meshlist.h"
#include "dxC_commandbuffer.h"
#include "dxC_pass.h"

namespace FSSE
{

PRESULT dxC_RenderCommandInit();
PRESULT dxC_RenderCommandDestroy();
PRESULT dxC_RenderCommandClearLists();

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


struct RCHDATA
{
	RenderCommand::Type		eType;

	virtual PRESULT IsValid( RenderCommand::Type eExpectedType ) const
	{
		ASSERT_RETINVALIDARG( eType == eExpectedType );
		return S_OK;
	}
#if ISVERSION(DEVELOPMENT)
	virtual void GetStats( WCHAR * pwszStats, int nBufLen ) const {};
#endif // DEVELOPMENT
	RCHDATA( RenderCommand::Type eInitType )
		: eType(eInitType)
	{};
};

struct RCHDATA_DEBUG_TEXT : public RCHDATA
{
	static const int cnTextLen = 32;
	char szText[ cnTextLen ];

	RCHDATA_DEBUG_TEXT()
		: RCHDATA(RenderCommand::DEBUG_TEXT)
	{
		szText[0] = NULL;
	}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const;
};

struct RCHDATA_DRAW_MESH_LIST : public RCHDATA
{
	//const MeshList * pMeshList;
	int nMeshListID;
	BOOL bDepthOnly;

	RCHDATA_DRAW_MESH_LIST()
		: RCHDATA(RenderCommand::DRAW_MESH_LIST),
		  nMeshListID(INVALID_ID),
		  bDepthOnly(FALSE)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const;
};

struct RCHDATA_SET_TARGET : public RCHDATA
{
	RENDER_TARGET_INDEX	eColor[ MAX_MRTS_ALLOWED ];
	DEPTH_TARGET_INDEX	eDepth;
	UINT				nLevel;
	UINT				nRTCount;

	RCHDATA_SET_TARGET()
		: RCHDATA(RenderCommand::SET_TARGET),
		  nLevel(0),
		  nRTCount(0)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		const int cnNameLen = 64;
		char szRTName[cnNameLen];
		char szDTName[cnNameLen];
		dxC_GetRenderTargetName( eColor[ 0 ], szRTName, cnNameLen );
		dxC_GetDepthTargetName( eDepth, szDTName, cnNameLen );

		PStrPrintf( pwszStats, nBufLen, L"RT: [%d] %S   DS: [%d] %S   Level: %d   RTCount: %d",
			eColor[ 0 ],
			szRTName,
			eDepth,
			szDTName,
			nLevel,
			nRTCount );
	}
};

struct RCHDATA_CLEAR_TARGET : public RCHDATA
{
	DWORD		dwFlags;
#ifdef ENGINE_TARGET_DX10
	D3DXVECTOR4	tColor[ MAX_MRTS_ALLOWED ];
#else
	D3DCOLOR	tColor[ MAX_MRTS_ALLOWED ];
#endif
	float		fDepth;
	DWORD		nStencil;
	UINT		nRTCount;

	RCHDATA_CLEAR_TARGET()
		: RCHDATA(RenderCommand::CLEAR_TARGET),
		  dwFlags(0),
		  nRTCount(0)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"Flags: 0x%08X  Color: 0x%08X  Depth: %3.2f  Stencil: %d  nRTCount: %d",
			dwFlags,
			tColor,
			fDepth,
			nStencil,
			nRTCount );
	}
};

struct RCHDATA_COPY_BUFFER : public RCHDATA
{
	//SPD3DCSHADERRESOURCEVIEW pSrc;
	//SPD3DCSHADERRESOURCEVIEW pDest;
	RENDER_TARGET_INDEX eSrc;
	RENDER_TARGET_INDEX eDest;
	
	RCHDATA_COPY_BUFFER()
		: RCHDATA(RenderCommand::COPY_BUFFER)
	{}
};

struct RCHDATA_SET_VIEWPORT : public RCHDATA
{
	D3DC_VIEWPORT tVP;

	RCHDATA_SET_VIEWPORT()
		: RCHDATA(RenderCommand::SET_VIEWPORT)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"X: %4d  Y: %4d  W: %4d  H: %4d  MinZ: %2.1f  MaxZ: %2.1f",
			tVP.TopLeftX,
			tVP.TopLeftY,
			tVP.Width,
			tVP.Height,
			tVP.MinDepth,
			tVP.MaxDepth );
	}
};

struct RCHDATA_SET_CLIP : public RCHDATA
{
	PLANE tPlane;

	RCHDATA_SET_CLIP()
		: RCHDATA(RenderCommand::SET_CLIP)
	{}
};

struct RCHDATA_SET_SCISSOR : public RCHDATA
{
	E_RECT tRect;

	RCHDATA_SET_SCISSOR()
		: RCHDATA(RenderCommand::SET_SCISSOR)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"L: %4d  T: %4d  R: %4d  B: %4d",
			tRect.left,
			tRect.top,
			tRect.right,
			tRect.bottom );
	}
};

struct RCHDATA_SET_FOG_ENABLED : public RCHDATA
{
	BOOL bEnabled;
	BOOL bForce;

	RCHDATA_SET_FOG_ENABLED()
		: RCHDATA(RenderCommand::SET_FOG_ENABLED),
		bForce(FALSE)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"Enable: %s  Force: %s",
			bEnabled ? L"true" : L"false",
			bForce   ? L"true" : L"false" );
	}
};

struct RCHDATA_CALLBACK_FUNC : public RCHDATA
{
	RenderCommand::PFN_CALLBACK pfnCallback;
	void * pUserData;

	RCHDATA_CALLBACK_FUNC() 
		: RCHDATA(RenderCommand::CALLBACK_FUNC),
		  pfnCallback(NULL)
	{}
};

struct RCHDATA_DRAW_PARTICLES : public RCHDATA
{
	DRAW_LAYER eDrawLayer;
	MATRIX mView;
	MATRIX mProj;
	MATRIX mProj2;
	VECTOR vEyePos;
	VECTOR vEyeDir;
	BOOL bDepthOnly;

	RCHDATA_DRAW_PARTICLES() 
		: RCHDATA(RenderCommand::DRAW_PARTICLES),
		bDepthOnly(FALSE)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
#if ISVERSION(DEVELOPMENT)
		PStrPrintf( pwszStats, nBufLen, L"Draw layer: %S",
			gszDrawLayerNames[ eDrawLayer ] );
#endif
	}
};

struct RCHDATA_SET_COLORWRITE : public RCHDATA
{
	UINT nRT;
	DWORD dwValue;

	RCHDATA_SET_COLORWRITE()
		: RCHDATA(RenderCommand::SET_COLORWRITE),
		nRT(0), dwValue(0)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"RT: %d  Color: 0x%08X",
			nRT,
			dwValue );
	}
};

struct RCHDATA_SET_DEPTHWRITE : public RCHDATA
{
	BOOL bDepthWriteEnable;

	RCHDATA_SET_DEPTHWRITE()
		: RCHDATA(RenderCommand::SET_DEPTHWRITE),
		bDepthWriteEnable(0)
	{}

	void GetStats( WCHAR * pwszStats, int nBufLen ) const
	{
		PStrPrintf( pwszStats, nBufLen, L"DepthWriteEnable: %d",
			bDepthWriteEnable );
	}
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define INCLUDE_RCD_DATA_ARRAY_TYPEDEF
#include "dxC_rendercommand_def.h"

#define INCLUDE_RCD_DATA_EXTERN
#include "dxC_rendercommand_def.h"

}; // namespace FSSE

#endif // __DXC_RCHANDLERS__