//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"

// External functions.
PRESULT e_FeatureLine_Shader_Init(void);
PRESULT e_FeatureLine_Shadow_Init(void);
PRESULT e_FeatureLine_MultiSample_Init(void);
PRESULT e_FeatureLine_Model_LOD_Init(void);
PRESULT e_FeatureLine_Texture_LOD_Init(void);
PRESULT e_FeatureLine_Distance_Cull_Init(void);
PRESULT e_FeatureLine_Wardrobe_Limit_Init(void);

// Initialize settings for all feature lines.
static
PRESULT e_FeatureLines_InitializeLines(void)
{
	// Get working state objects.
	CComPtr<OptionState> pState;
	V_RETURN(e_OptionState_CloneActive(&pState))
	ASSERT_RETVAL(pState != 0, E_FAIL);

	CComPtr<FeatureLineMap> pMap;
	V_RETURN(e_FeatureLine_OpenMap(pState, &pMap))
	ASSERT_RETVAL(pMap != 0, E_FAIL);

	// Initialize (to maximums).
	DWORD nLineName;
	for (unsigned i = 0; e_FeatureLine_EnumerateLines(i, nLineName); ++i)
	{
		V(pMap->SelectPoint(nLineName, 1.0f));
	}

	// Make active.
	V(e_FeatureLine_CommitToActive(pMap));
	V(e_OptionState_CommitToActive(pState));

	// Great.
	return S_OK;
}

// Central initializer.
// This function is intentionally segregated.
void e_FeatureLines_Init(void)
{
	// Initialize the feature line system manager.
	V(e_FeatureLine_Init());

	if ( e_IsNoRender() )
		return;

	// Initialize & register all feature lines.
	V(e_FeatureLine_Shader_Init());
	V(e_FeatureLine_Shadow_Init());
	V(e_FeatureLine_MultiSample_Init());
	V(e_FeatureLine_Model_LOD_Init());
	V(e_FeatureLine_Texture_LOD_Init());
	V(e_FeatureLine_Distance_Cull_Init());
	V(e_FeatureLine_Wardrobe_Limit_Init());

	// Initial settings for all feature lines.
	V(e_FeatureLines_InitializeLines());
}

void e_FeatureLines_Deinit(void)
{
	// Deinitialize the feature line system manager.
	V(e_FeatureLine_Deinit());
}
