//----------------------------------------------------------------------------
// UserServerPrivateAPI.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include <MuxClient.h>
#include <MuxServer.h>


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
extern const MUXCLIENTID MUX_MULTIPOINT_ID;


//----------------------------------------------------------------------------
// PRIVATE METHODS FOR PRIVILAGED SYSTEMS
//----------------------------------------------------------------------------

NET_COMMAND_TABLE *	
	SvrGetRequestCommandTableForUserServer(
		SVRTYPE svr );

struct NETCOM *
	SvrGetNetconForUserServer(
		void );

IMuxClient *SvrNetGetMuxClientForSvr(
		SVRID id);
