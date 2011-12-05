// Copyright (c) 2006 Flagship Studios, Inc.

#ifndef __SHADERVERSIONCONSTANTS_FX__
#define __SHADERVERSIONCONSTANTS_FX__

#define FX_COMPILE

#ifdef FXC10_PATH
#include "../../../source/dx9/dx9_shaderversionconstants.h"
#else
#include "../../source/dx9/dx9_shaderversionconstants.h"
#endif

#undef FX_COMPILE

#endif
