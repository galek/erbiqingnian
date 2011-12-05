//----------------------------------------------------------------------------
// dx10_common.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_COMMON__
#define __DX10_COMMON__

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------
#include <d3d10.h>
#include <d3dx10.h>
#include <d3dx10tex.h>
#include <DXGI.h>

#include "dx10_types.h"
#include "dx10_format_conversion.h"
#include "dx10_device.h"

inline UINT dx10_TopologyVertexCount( D3DC_PRIMITIVE_TOPOLOGY topology, UINT count )
{
	switch( topology )
	{
	//Classical types	
	case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		return count * 3; break;
	case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
		return count + 2; break;
	case D3D10_PRIMITIVE_TOPOLOGY_LINELIST:
		return count * 2; break;
	case D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP:
		return count + 1; break;
	case D3D10_PRIMITIVE_TOPOLOGY_POINTLIST:
		return count; break;

	//Adjacency types
	case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
		return 6 * count; break;
	case D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
		return 2 * ( count + 2 ); break;
	case D3D10_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
		return 4 * count; break;
	case D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
		return count + 3; break;
	default :
		ASSERTX( 0, "Unsupported primitive type!" );
		break;
	};
	return 0;
}

void dx10_DrawFullscreenQuadXY( D3D_EFFECT* pEffect );

//Borrowed these from d3d9types.h
// pixel shader version token
#define D3DPS_VERSION(_Major,_Minor) (0xFFFF0000|((_Major)<<8)|(_Minor))

// vertex shader version token
#define D3DVS_VERSION(_Major,_Minor) (0xFFFE0000|((_Major)<<8)|(_Minor))

// extract major/minor from version cap
#define D3DSHADER_VERSION_MAJOR(_Version) (((_Version)>>8)&0xFF)
#define D3DSHADER_VERSION_MINOR(_Version) (((_Version)>>0)&0xFF)

#endif //__DX10_COMMON__
