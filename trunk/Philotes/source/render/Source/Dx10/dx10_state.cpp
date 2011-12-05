//----------------------------------------------------------------------------
// dx10_state.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

D3D10_CPU_ACCESS_FLAG dx10_GetCPUAccess( D3DC_USAGE usage )
{
	switch( usage )
	{
	case D3DC_USAGE_2DTEX_SCRATCH:
	case D3DC_USAGE_2DTEX_STAGING:
		return (D3D10_CPU_ACCESS_FLAG)(D3D10_CPU_ACCESS_WRITE | D3D10_CPU_ACCESS_READ); break;	
	case D3DC_USAGE_BUFFER_MUTABLE:
		return (D3D10_CPU_ACCESS_FLAG)(D3D10_CPU_ACCESS_WRITE); break;
	case D3DC_USAGE_1DTEX:
	case D3DC_USAGE_2DTEX:
	case D3DC_USAGE_2DTEX_DEFAULT:
	case D3DC_USAGE_2DTEX_RT:
	case D3DC_USAGE_2DTEX_DS:
	case D3DC_USAGE_2DTEX_SM:
	case D3DC_USAGE_BUFFER_REGULAR:
	case D3DC_USAGE_BUFFER_CONSTANT:
	case D3D10_USAGE_BUFFER_STREAMOUT:
	case D3DC_USAGE_CUBETEX_DYNAMIC: //Hack for some reason cubemaps can't be dynamic in DX10
		return (D3D10_CPU_ACCESS_FLAG)0x00000000; break;
	};
	ASSERTX(0, "Unhandled D3DC_USAGE!");
	return (D3D10_CPU_ACCESS_FLAG)0x00000000;
}

D3D10_USAGE dx10_GetUsage( D3DC_USAGE usage )
{
	switch( usage )
	{
	case D3DC_USAGE_2DTEX_SCRATCH:
	case D3DC_USAGE_2DTEX_STAGING:
		return D3D10_USAGE_STAGING; break;	
	case D3DC_USAGE_BUFFER_MUTABLE:
		return D3D10_USAGE_DYNAMIC; break; 
	case D3DC_USAGE_1DTEX:
	case D3DC_USAGE_2DTEX:
	case D3DC_USAGE_2DTEX_DEFAULT:
	case D3DC_USAGE_CUBETEX_DYNAMIC: //Hack for some reason cubemaps can't be dynamic in DX10
	case D3DC_USAGE_BUFFER_REGULAR:
	case D3DC_USAGE_BUFFER_CONSTANT:
    case D3D10_USAGE_BUFFER_STREAMOUT:
	case D3DC_USAGE_2DTEX_RT:
	case D3DC_USAGE_2DTEX_DS:
	case D3DC_USAGE_2DTEX_SM:
		return D3D10_USAGE_DEFAULT; break;
		//return D3D10_USAGE_IMMUTABLE; break;  //RTs can't be immutable???
		return (D3D10_USAGE)0x00000000; break;
	};
	ASSERTX(0, "Unhandled D3DC_USAGE!");
	return (D3D10_USAGE)0x00000000;
}

D3D10_BIND_FLAG dx10_GetBindFlags( D3DC_USAGE usage )
{
	switch( usage )
	{
	case D3DC_USAGE_2DTEX_SCRATCH:
	case D3DC_USAGE_2DTEX_STAGING:
		return (D3D10_BIND_FLAG)0x00000000; break;
	case D3DC_USAGE_1DTEX:
	case D3DC_USAGE_2DTEX:
	case D3DC_USAGE_2DTEX_DEFAULT:
	case D3DC_USAGE_CUBETEX_DYNAMIC: //Hack for some reason cubemaps can't be dynamic in DX10
		return D3D10_BIND_SHADER_RESOURCE; break;
	case D3DC_USAGE_2DTEX_RT:
		return (D3D10_BIND_FLAG)(D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET); break;
	case D3DC_USAGE_2DTEX_DS:
		return D3D10_BIND_DEPTH_STENCIL; break;
	case D3DC_USAGE_2DTEX_SM:
		return (D3D10_BIND_FLAG)(D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_DEPTH_STENCIL); break;
	
	case D3DC_USAGE_BUFFER_MUTABLE:
	case D3DC_USAGE_BUFFER_REGULAR:
	case D3DC_USAGE_BUFFER_CONSTANT:
		return (D3D10_BIND_FLAG)0x00000000; break;
	case D3D10_USAGE_BUFFER_STREAMOUT:
		return (D3D10_BIND_FLAG) D3D10_BIND_STREAM_OUTPUT; break;
	};
	ASSERTX(0, "Unhandled D3DC_USAGE!");
	return (D3D10_BIND_FLAG)0x00000000;
}

PRESULT dxC_UpdateBufferEx(
	LPD3DCBASEBUFFER pBuffer,
	void* pSysData,
	UINT offset,
	UINT size,
	D3DC_USAGE uSage,
	BOOL bDiscard )
{
	REF( bDiscard );

	if( dx10_GetCPUAccess( uSage ) & D3D10_CPU_ACCESS_WRITE )
	{
		void* pData = NULL;
		V_RETURN( pBuffer->Map( bDiscard ? D3D10_MAP_WRITE_DISCARD : D3D10_MAP_WRITE, 0, &pData ) );
		ASSERT_RETFAIL( pData );
		memcpy( (char*)pData + offset, pSysData, size );
		pBuffer->Unmap();
	}
	else
	{
		D3D10_BOX box;
		box.front = 0;
		box.back = 1;
		box.top = 0;
		box.bottom = 1;
		box.left = offset;
		box.right = offset + size;

		dxC_GetDevice()->UpdateSubresource( pBuffer, 0, &box, pSysData, size, 0 );  //KMNV Hack: Box wasn't working so setting to NULL for now
	}

	return S_OK;
}