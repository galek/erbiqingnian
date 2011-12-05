//----------------------------------------------------------------------------
// e_irradiance.cpp
//
// - Implementation for SH or anything else representing irradiance
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "boundingbox.h"
#include "e_settings.h"
#include "e_environment.h"
#include "e_model.h"
#include "e_definition.h"
#include "e_shadow.h"
#include "performance.h"
#include "e_profile.h"
#include "e_main.h"

#include "e_irradiance.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL gbCubeMapGenerate = FALSE;
int gnCubeMapFace = 0;
int gnCubeMapSize = 128;
char gszCubeMapFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
int gnEnvMapTextureID = INVALID_ID;
DWORD gdwCubeMapFlags = 0;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------