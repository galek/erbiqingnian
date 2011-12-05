//----------------------------------------------------------------------------
// dxC_rchandler.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_RCHANDLER__
#define __DXC_RCHANDLER__

#include "dxC_target.h"
#include "dxC_meshlist.h"
//#include "dxC_commandbuffer.h"

namespace FSSE
{

struct RCHDATA
{
	RenderCommand::Type		eType;

	virtual PRESULT IsValid( RenderCommand::Type eExpectedType ) const
	{
		ASSERT_RETINVALIDARG( eType == eExpectedType );
		return S_OK;
	}
	RCHDATA( RenderCommand::Type eInitType )
		: eType(eInitType)
	{};
};

struct RCHDATA_DRAWMESHLIST : public RCHDATA
{
	//const MeshList * pMeshList;
	int nMeshListID;

	RCHDATA_DRAWMESHLIST()
		: RCHDATA(RenderCommand::DRAW_MESH_LIST),
		  nMeshListID(INVALID_ID)
	{}
};

struct RCHDATA_SETTARGET : public RCHDATA
{
	RENDER_TARGET	eColor;
	ZBUFFER_TARGET	eDepth;
	UINT			nLevel;

	RCHDATA_SETTARGET()
		: RCHDATA(RenderCommand::SET_TARGET)
	{}
};

struct RCHDATA_CLEARTARGET : public RCHDATA
{
	DWORD		dwFlags;
	D3DCOLOR	tColor;
	float		fDepth;
	DWORD		nStencil;

	RCHDATA_CLEARTARGET()
		: RCHDATA(RenderCommand::CLEAR_TARGET),
		  dwFlags(0)
	{}
};

struct RCHDATA_COPYBUFFER : public RCHDATA
{
	SPD3DCSHADERRESOURCEVIEW pSrc;
	SPD3DCSHADERRESOURCEVIEW pDest;

	RCHDATA_COPYBUFFER()
		: RCHDATA(RenderCommand::COPY_BUFFER)
	{}
};

struct RCHDATA_SETVIEWPORT : public RCHDATA
{
	D3DC_VIEWPORT tVP;

	RCHDATA_SETVIEWPORT()
		: RCHDATA(RenderCommand::SET_VIEWPORT)
	{}
};

struct RCHDATA_SETCLIP : public RCHDATA
{
	PLANE tPlane;

	RCHDATA_SETCLIP()
		: RCHDATA(RenderCommand::SET_CLIP)
	{}
};

struct RCHDATA_SETSCISSOR : public RCHDATA
{
	RECT tRect;

	RCHDATA_SETSCISSOR()
		: RCHDATA(RenderCommand::SET_SCISSOR)
	{}
};

struct RCHDATA_SETFOGENABLED : public RCHDATA
{
	BOOL bEnabled;
	BOOL bForce;

	RCHDATA_SETFOGENABLED()
		: RCHDATA(RenderCommand::SET_FOG_ENABLED),
		bForce(FALSE)
	{}
};

struct RCHDATA_CALLBACK : public RCHDATA
{
	RenderCommand::PFN_CALLBACK pfnCallback;
	void * pUserData;

	RCHDATA_CALLBACK() 
		: RCHDATA(RenderCommand::CALLBACK_FUNC),
		  pfnCallback(NULL)
	{}
};

}; // namespace FSSE

#endif // __DXC_RCHANDLER__