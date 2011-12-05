//----------------------------------------------------------------------------
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __LOD_H__
#define __LOD_H__

// Must be in order from lowest LOD to highest LOD.
enum
{
	LOD_NONE = -1,		// to indicate that a LOD has not been selected
	LOD_LOW,
	LOD_HIGH,
	LOD_COUNT,			// not a LOD value
	LOD_ANY,			// special value to select first available LOD
	LOD_HIGH_AS_LOW,	// load the high LOD as a low LOD (for debugging purposes)
};

#endif
