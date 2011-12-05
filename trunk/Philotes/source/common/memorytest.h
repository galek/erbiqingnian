//----------------------------------------------------------------------------
// memorytest.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "memorytypes.h"

namespace FSCommon
{

bool MemoryTest(MEMORY_ALLOCATOR_TYPE allocatorType, unsigned int threadCount);

} // end namespace FSCommon