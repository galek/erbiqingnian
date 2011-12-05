//----------------------------------------------------------------------------
// dx10_format_conversion.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_FORMAT_CONVERSION__
#define __DX10_FORMAT_CONVERSION__

//Because dx10 is so beautiful a resource of a given format is not necessarily
//able to be used as a shader resource view without type conversion
static DXC_FORMAT dx10_ResourceFormatToSRVFormat( DXC_FORMAT resFormat )
{
	switch( resFormat )
	{
#define DX10_FORMAT_CONVERSION_SRV
#include "dx10_format_conversion_priv.h"
#undef DX10_FORMAT_CONVERSION_SRV
	};

	return resFormat;
}

static DXC_FORMAT dx10_ResourceFormatToDSVFormat( DXC_FORMAT resFormat )
{
	switch( resFormat )
	{
#define DX10_FORMAT_CONVERSION_DSV
#include "dx10_format_conversion_priv.h"
#undef DX10_FORMAT_CONVERSION_DSV
	};

	return resFormat;
}

static DXC_FORMAT dx10_ResourceFormatToRTVFormat( DXC_FORMAT resFormat )
{
	switch( resFormat )
	{
#define DX10_FORMAT_CONVERSION_RTV
#include "dx10_format_conversion_priv.h"
#undef DX10_FORMAT_CONVERSION_RTV
	};

	return resFormat;
}

#endif //__DX10_FORMAT_CONVERSION__