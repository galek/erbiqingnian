//----------------------------------------------------------------------------
// c_adclient.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_ADCLIENT_H_
#define _C_ADCLIENT_H_

#include "e_adclient.h"

void c_AdClientInit();
void c_AdClientShutdown();

void c_AdClientEnterLevel(struct GAME * pGame, int nLevelIndex);
void c_AdClientExitLevel(struct GAME * pGame, int nLevelIndex);

void c_AdClientUpdate();

int  c_AdClientAddAd(const char * pszName, AD_DOWNLOADED_CALLBACK pfnDownloadCallback, AD_CALCULATE_IMPRESSION_CALLBACK pfnImpressionCallback);

void c_AdClientReportImpression(int nAdInstance, AD_CLIENT_IMPRESSION & tAdClientImpression);

void c_AdClientFixupSKUs();

BOOL c_AdClientIsDisabled();

#endif // _C_ADCLIENT_H_