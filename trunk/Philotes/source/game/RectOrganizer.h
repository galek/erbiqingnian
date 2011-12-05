//----------------------------------------------------------------------------
// RectOrganizer.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _RECTORGANIZER_H_
#define _RECTORGANIZER_H_

class RECT_ORGANIZE_SYSTEM;

RECT_ORGANIZE_SYSTEM * RectOrganizeInit(
	MEMORYPOOL * pool);

BOOL RectOrganizeDestroy(
	MEMORYPOOL * pool,
	RECT_ORGANIZE_SYSTEM * pSystem);

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float *pfRight,
	float *pfBottom);

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float *pfLeft,
	float *pfTop,
	float fWidth,
	float fHeight);

DWORD RectOrganizeAddRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fIdealX,
	float fIdealY,
	float fLeft,
	float fTop,
	float fWidth,
	float fHeight);

void RectOrganizeClear(
	RECT_ORGANIZE_SYSTEM * pSystem);

void RectOrganizeRemoveRect(
	RECT_ORGANIZE_SYSTEM * pSystem,
	DWORD idRect);

BOOL RectOrganizeCompute(
	RECT_ORGANIZE_SYSTEM * pSystem,
	float fFrameLeft,
	float fFrameTop,
	float fFrameRight,
	float fFrameBottom,
	unsigned int maxTries);

BOOL RectOrganizeCompute(
	RECT_ORGANIZE_SYSTEM * pSystem,
	unsigned int maxTries);

#endif

