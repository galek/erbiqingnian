//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DWMDISABLER_H_
#define __DWMDISABLER_H_

// This class is used to disable desktop composition by the
// Windows Vista Desktop Window Manager (DWM).
//
// See:
//
// http://msdn2.microsoft.com/en-us/library/aa969538.aspx
// http://msdn2.microsoft.com/en-us/library/aa969510.aspx

class DWMDisabler
{
	public:
		DWMDisabler(void);
		~DWMDisabler(void);
};

#endif
