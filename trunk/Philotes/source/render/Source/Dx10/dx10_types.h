//----------------------------------------------------------------------------
// dx10_types.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_TYPES__
#define __DX10_TYPES__

#include <atlcomcli.h>
#include <dxgi.h>
#include "dx10_state_def.h"

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

// DXGI


typedef CComPtr<IDXGIFactory>				SPDXGIFACTORY;

typedef CComPtr<IDXGIOutput>				SPDXGIOUTPUT;
typedef IDXGIOutput*						LPDXGIOUTPUT;
#endif //__DX10_TYPES__
