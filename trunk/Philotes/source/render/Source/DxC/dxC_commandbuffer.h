//----------------------------------------------------------------------------
// dxC_commandbuffer.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_COMMANDBUFFER__
#define __DXC_COMMANDBUFFER__

#include "safe_vector.h"
//#include "dxC_rchandlers.h"

namespace FSSE
{


struct RenderCommand
{
	typedef PRESULT (*PFN_RENDERCOMMAND_HANDLER)( const struct RenderCommand * pCommand );
	typedef	PRESULT (*PFN_CALLBACK)( void * pUserData );

	enum Type
	{
		INVALID = 0,		// sentinel to protect against uninitialized commands
#		define INCLUDE_RCD_ENUM
#		include "dxC_rendercommand_def.h"
		// count
		_NUM_TYPES
	};

#if ISVERSION(DEVELOPMENT)
	PRESULT GetDataStats( WCHAR * pwszStats, int nBufLen ) const;
#endif

	Type					m_Type;
	int						m_nDataID;
};


class CommandBuffer
{
	typedef safe_vector<RenderCommand> CommandList;

	CommandList m_Buffer;

public:
	PRESULT Commit();
	PRESULT Clear();
	void Add( const RenderCommand & tRC )
	{
		m_Buffer.push_back( tRC );
	}
#if ISVERSION(DEVELOPMENT)
	PRESULT GetStats( WCHAR * pwszStats, int nBufLen );
#endif // DEVELOPMENT
};


typedef safe_vector<CommandBuffer>	CommandBufferVector;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


inline void dxC_CommandAdd( CommandBuffer & tCB, RenderCommand::Type eType, int nDataID )
{
	RenderCommand tRC;
	tRC.m_Type			= eType;
	tRC.m_nDataID		= nDataID;

	tCB.Add( tRC );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CommandGetStats( WCHAR * pszStr, int nBufLen, const RenderCommand & tRC );

}; // namespace FSSE

#endif // __DXC_COMMANDBUFFER__