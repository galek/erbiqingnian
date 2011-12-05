//----------------------------------------------------------------------------
// cmdline.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CMDLINE_H_
#define _CMDLINE_H_

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define DEFAULT_CLOSED_SERVER_IP				NULL
#define DEFAULT_MY_IP							""

// drb - temp for play day!
#define DEFAULT_OPEN_SERVER_IP					"127.0.0.1"

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void AppParseCommandLine(
	const char * cmdline);


#endif // _CMDLINE_H_
