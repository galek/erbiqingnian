//----------------------------------------------------------------------------
// FILE: unitfiletooold.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __UNITFILETOOOLD_H_
#define __UNITFILETOOOLD_H_

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

// any units with a unit save version below this number will be automatically wiped,
// when you want to do a controlled character wipe for development, bump
// up value of USV_CURRENT_VERSION, and set USV_TOO_OLD_TO_LOAD
// to the old value of UNITSAVE_VERSION_CURRENT
const unsigned int USV_TOO_OLD_TO_LOAD = 173;

#endif