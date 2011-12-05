//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_GAMMA__
#define __E_GAMMA__

class OptionState;

bool e_CanDoGamma(const OptionState * pState, bool bWindowed);
bool e_CanDoManualGamma(const OptionState * pState);
PRESULT e_RenderManualGamma(void);

PRESULT e_SetGammaPercent( float fGammaPct );
PRESULT e_SetGamma( float fGamma );
float e_MapGammaPctToPower( float fGammaPct );
float e_MapGammaPowerToPct( float fGammaPower );
PRESULT e_UpdateGamma();

#endif
