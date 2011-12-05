//----------------------------------------------------------------------------
// dxC_commandbuffer.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_meshlist.h"
#include "dxC_target.h"
#include "dxC_commandbuffer.h"

#include "dxC_rchandlers.h"

namespace FSSE
{;

//----------------------------------------------------------------------------
// Global command buffer vector
//CommandBufferVector * g_CommandBuffers;



extern RenderCommand::PFN_RENDERCOMMAND_HANDLER g_RenderCommandHandlers[ RenderCommand::_NUM_TYPES ];

//----------------------------------------------------------------------------

PRESULT CommandBuffer::Commit()
{
	if ( m_Buffer.empty() )
		return S_OK;

	for ( CommandList::iterator i = m_Buffer.begin(); i != m_Buffer.end(); ++i )
	{
		RenderCommand & tCommand = *i;

		ASSERTV_CONTINUE( g_RenderCommandHandlers[ tCommand.m_Type ], "Missing command handler for type %d", tCommand.m_Type );
		V( g_RenderCommandHandlers[ tCommand.m_Type ]( &tCommand ) );
	}

	// Always clear on completion
	Clear();

	return S_OK;
}

PRESULT CommandBuffer::Clear()
{
	m_Buffer.clear();
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)

#define MAX_RC_TYPENAME_LEN	32
static const char sgszTypeName[ RenderCommand::_NUM_TYPES ][ MAX_RC_TYPENAME_LEN ] = 
{
	"INVALID",
#	define INCLUDE_RCD_ENUM_STRING
#	include "dxC_rendercommand_def.h"
};


PRESULT RenderCommand::GetDataStats( WCHAR * pwszStats, int nBufLen ) const
{
	ASSERT_RETINVALIDARG( pwszStats );

#define CASE_GET_STATS( eType )										\
	case eType:														\
		{															\
			const RCD_TYPE(eType) * pData;							\
			RCD_ARRAY_GET( eType, m_nDataID, pData );				\
			pData->GetStats( pwszStats, nBufLen );			\
			break;													\
		}

	switch ( m_Type )
	{
	CASE_GET_STATS(DRAW_MESH_LIST)
	CASE_GET_STATS(SET_TARGET)
	CASE_GET_STATS(CLEAR_TARGET)
	CASE_GET_STATS(COPY_BUFFER)
	CASE_GET_STATS(SET_VIEWPORT)
	CASE_GET_STATS(SET_CLIP)
	CASE_GET_STATS(SET_SCISSOR)
	CASE_GET_STATS(SET_FOG_ENABLED)
	CASE_GET_STATS(CALLBACK_FUNC)
	CASE_GET_STATS(DRAW_PARTICLES)
	CASE_GET_STATS(DEBUG_TEXT)
	default:
		PStrPrintf( pwszStats, nBufLen, L"No data" );
		break;
	}

#undef CASE_GET_STATS

	return S_OK;
}


PRESULT dxC_CommandGetStats( WCHAR * pszStr, int nBufLen, const RenderCommand & tRC )
{
	ASSERT_RETINVALIDARG( pszStr );

	const int cnStatsLen = 256;
	WCHAR wszStats[ cnStatsLen ] = L"";

	V( tRC.GetDataStats( wszStats, cnStatsLen ) );

	PStrPrintf( pszStr, nBufLen, L"%-20S: %s\n",
		sgszTypeName[ tRC.m_Type ],
		wszStats );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT CommandBuffer::GetStats( WCHAR * pwszStats, int nBufLen )
{
	ASSERT_RETINVALIDARG( pwszStats );

	const int cnCmd = 1024;
	WCHAR wszCmd[ cnCmd ];

	pwszStats[0] = NULL;

	for ( CommandList::iterator i = m_Buffer.begin();
		i != m_Buffer.end();
		++i )
	{
		V_CONTINUE( dxC_CommandGetStats( wszCmd, 1024, *i ) );
		PStrCat( pwszStats, wszCmd, nBufLen );
	}

	return S_OK;
}


#endif // DEVELOPMENT

}; // namespace FSSE