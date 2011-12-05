//----------------------------------------------------------------------------
// uix_debug.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_debug.h"
#include "uix_components.h"
#include "uix_scheduler.h"
#include "uix_chat.h"
#include "e_profile.h"
#include "e_particle.h"
#include "e_model.h"
#include "e_settings.h"
#include "e_particles_priv.h"
#include "e_dpvs.h"
#include "e_primitive.h"
#include "c_particles.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_appearance_priv.h" // also includes units.h, which includes game.h
#include "c_music.h"
#include "c_music_priv.h"
#include "c_backgroundsounds_priv.h"
#include "c_sound_priv.h"
#include "c_sound_memory_debug.h"
#include "c_input.h"
#include "appcommontimer.h"
#include "debugbars_priv.h"
#include "dbg_file_line_table.h"
#include "prime.h"
#include "gamelist.h"
#include "level.h"
#include "perfhier.h"
#include "statspriv.h"
#include "skills.h"
#include "dictionary.h"
#include "unittag.h"
#include "imports.h"
#include "room_layout.h"
#include "fontcolor.h"
#include "c_font.h"
#include "imports.h"
#include "e_viewer.h"
#include "unittag.h"
#include "script.h"
#include "GlobalIndex.h"
#include "inventory.h"
#include "unit_priv.h"
#include "chat.h"
#include "console.h"
#include "consolecmd.h"

#if USE_MEMORY_MANAGER
#include "memorymanager_i.h"
#include "memoryallocatorinternals_i.h"
using namespace FSCommon;
#endif

using namespace FSSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This is ugly, but I need the debug info
// It IS ugly
#if !ISVERSION(SERVER_VERSION)
#include "../3rd Party/fmodapi40820win/api/inc/fmod.hpp"
// End ugly

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void drbStepDebug( int & clt, int & srv, int & paths, int & max_paths, float & avg_paths );

BOOL sUIUpdateFPS(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	WCHAR str[512];

	if (!bVisible)
	{
		return FALSE;
	}
	
#if ISVERSION(DEVELOPMENT)
	//int nRoomsDrawn = c_RoomGetDrawCount();
	int nBatches = e_MetricsGetTotalBatchCount();
	int nParticlesDrawn   = e_ParticlesGetDrawnTotal();
	int nParticlesUpdated = c_ParticlesGetUpdateTotal();
	float fParticleWorldArea  = e_ParticlesGetTotalWorldArea();
	float fParticleScreenAreaMeasured = e_ParticlesGetTotalScreenArea( PARTICLE_DRAW_SECTION_INVALID, FALSE );
	float fParticleScreenAreaPredicted = e_ParticlesGetTotalScreenArea( PARTICLE_DRAW_SECTION_INVALID, TRUE );
	int nNumSounds = c_SoundGetNumActive();
	float fDrawFrameRate = AppCommonGetDrawFrameRate();
	DWORD dwSlowestFrameMs = AppCommonGetDrawLongestFrameMs();
	float fAnimationPercent = c_AppearanceGetDebugDisplayAnimationPercent();
	int cltSteps, srvSteps, numPaths, max_paths;
	float avg_paths;
	
	drbStepDebug( cltSteps, srvSteps, numPaths, max_paths, avg_paths );

	PStrPrintf(str, 512, L"Game fps:%3.2f  Draw fps:%3.2f  Longest:%4dms  batches:%4d  \nparticles:%5d/%5d  particle delta:%3.2f area:(w:%6.1f  s:%6.1f/%6.1f)  sounds:%i\nstep:%i/%i paths:%i/%i/%0.2f  animation percent:%0.2f",
		AppCommonGetSimFrameRate(),
		fDrawFrameRate,
		dwSlowestFrameMs,
		nBatches,
		nParticlesDrawn,
		nParticlesUpdated,
		c_ParticleGetDelta(),
		fParticleWorldArea, fParticleScreenAreaMeasured, fParticleScreenAreaPredicted,
		nNumSounds,
		cltSteps, srvSteps, numPaths, max_paths, avg_paths,
		fAnimationPercent);
#else
	PStrPrintf(str, 512, L"fps:%3.2f",	AppCommonGetDrawFrameRate());
#endif
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdatePing(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
	{
		return FALSE;
	}

	WCHAR str[512];

	const WCHAR *puszFormat = GlobalStringGet( GS_PING_TEXT );
	
	extern MSG_SCMD_PING c_GetPingMsg();
	MSG_SCMD_PING msg = c_GetPingMsg();
	PStrPrintf(str, 512, 
		puszFormat,
		msg.timeOfReceive, msg.timeStamp1, msg.timeStamp2);
	
	// TRAVIS: OK, we still want it going to the pingmeter, where it belongs.		
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");
	//LOCALIZED_CMD_OUTPUT(str);
	//LOCALIZED_CMD_OUTPUT(L"");

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateBars(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	BOOL bUpdate = FALSE;

	if ( bVisible )
	{
		c_UpdateUIBars();
		bUpdate = TRUE;
	}

	return bUpdate;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateBatches(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];
	e_GetBatchString(str, 512);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateDPVSStats(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[1024];
	e_DPVS_GetStats(str, 1024);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEngineStatesStats(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[2048];
	e_GetStatesStatsString(str, 2048);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateParticleStats(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[2048];
	e_ParticleDrawStatsGetString(str, 2048);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEngineStats(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[2048];
	e_GetStatsString(str, 2048);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateHashMetrics(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[2048];
	e_GetHashMetricsString(str, 2048);
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateMemory(
	GAME * game,
	UI_TEXTBOX * textbox,
	BOOL bVisible)
{
	if (!bVisible)
	{
		return FALSE;
	}

	WCHAR str[512];

	MEMORY_INFO memory_info;
	MemoryGetMetrics(NULL, &memory_info);

	PStrPrintf(str, 512, L"IO:  handles: %d", memory_info.dwHandleCount);
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"Memory:  physical: %I64d/%I64d MB   page: %I64d/%I64d MB  virtual: %I64d/%I64d MB",
		memory_info.ullAvailPhys/MEGA, memory_info.ullTotalPhys/MEGA, 
		memory_info.ullAvailPageFile/MEGA, memory_info.ullTotalPageFile/MEGA,
		memory_info.ullAvailVirtual/MEGA, memory_info.ullTotalVirtual/MEGA);
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"Memory:  total allocated:%d  total allocations:%d", 
		memory_info.m_nTotalTotalAllocated, memory_info.m_nTotalAllocations);
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"Memory:  size of UNIT structure:%d", 
		sizeof(UNIT));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"    %-30s  %5s  %12s  %8s  %12s",
		L"file", L"line", L"bytes", L"allocs", L"recent");
	UITextBoxAddLine(textbox, str);

	for (int ii = 0; ii < FLIDX_WINDOW_SIZE; ii++)
	{
		if (!memory_info.m_LineWindow[ii].m_csFile)
		{
			UITextBoxAddLine(textbox, L"");
			continue;
		}
		char szFile[128];
		int len = PStrLen(memory_info.m_LineWindow[ii].m_csFile);
		PStrCopy(szFile, memory_info.m_LineWindow[ii].m_csFile + (len < 30 ? 0 : len - 29), 128);
		WCHAR wszFile[128];
		PStrCvt(wszFile, szFile, 128);
		PStrPrintf(str, 512, L"    %-30s  %5d  %12d  %8d  %12d",
			wszFile, memory_info.m_LineWindow[ii].m_nLine, memory_info.m_LineWindow[ii].m_nCurBytes, 
			memory_info.m_LineWindow[ii].m_nCurAllocs, memory_info.m_LineWindow[ii].m_nRecentBytes);
		UITextBoxAddLine(textbox, str);
	}
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
extern int g_MemorySystemInfoCmd;
extern int g_MemorySystemInfoSetAverageIndex;
extern int g_MemorySystemInfoMode;
BOOL sUIUpdateMemoryPoolInfo(
	GAME * game,
	UI_TEXTBOX * textbox,
	BOOL bVisible)
{
	if (!bVisible)
	{
		return FALSE;
	}
	
#if USE_MEMORY_MANAGER	
	
	WCHAR str[512];
	REF(str);
	VECTOR2 vMin;
	VECTOR2 vMax;
	VECTOR vTopLeft;
	VECTOR vBottomRight;
	VECTOR vTextPos;
	
	static int expandedPoolIndex = 0;
	
	
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	// Draw the background
	//
	vTopLeft.x = vMin.x = nWidth * 0.1f;
	vTopLeft.y = vMin.y = nHeight * 0.1f;	
	vBottomRight.x = vMax.x = nWidth * 0.9f;
	vBottomRight.y = vMax.y = nHeight * 0.8f;
	
	e_PrimitiveAddBox(PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xC0000000);

	// Top Border
	//
	vMin.x = vTopLeft.x - 2;
	vMin.y = vTopLeft.y - 2;
	vMax.x = vBottomRight.x + 2;
	vMax.y = vTopLeft.y;	
	e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);

	// Left Border
	//
	vMin.x = vTopLeft.x - 2;
	vMin.y = vTopLeft.y - 2;
	vMax.x = vTopLeft.x;
	vMax.y = vBottomRight.y + 2;	
	e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);

	// Right Border
	//
	vMin.x = vBottomRight.x;
	vMin.y = vTopLeft.y - 2;
	vMax.x = vBottomRight.x + 2;
	vMax.y = vBottomRight.y + 2;	
	e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);

	// Bottom Border
	//
	vMin.x = vTopLeft.x - 2;
	vMin.y = vBottomRight.y;
	vMax.x = vBottomRight.x + 2;
	vMax.y = vBottomRight.y + 2;
	e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);

	// Add title
	//
	vTextPos.x = 0.0f;
	vTextPos.y = 0.79f;
	e_UIAddLabel(L"Memory Allocator Info", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOP, UIALIGN_TOPLEFT);
	
	// Line under title
	//
	vMin.x = vTopLeft.x;
	vMin.y = vTopLeft.y + 23;
	vMax.x = vBottomRight.x;
	vMax.y = vTopLeft.y + 25;	
	e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);

	// Header
	//
	vTextPos.y = 0.73f;
	vTextPos.x = -0.76f;
	
	e_UIAddLabel(L"Allocator Name", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.38f;
	e_UIAddLabel(L"IntSize", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.22f;	
	e_UIAddLabel(L"IntFrag", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.14f;	
	e_UIAddLabel(L"ExtFrag", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
	
	vTextPos.x += 0.24f;	
	e_UIAddLabel(L"LargestFreeBlock", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
	
	vTextPos.x += 0.16f;	
	e_UIAddLabel(L"Overhead", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.19f;	
	e_UIAddLabel(L"ExtSize", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.18f;	
	e_UIAddLabel(L"ExtCount", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	// Line under header
	//
	vMin.x = vTopLeft.x + 15;
	vMin.y = vTopLeft.y + 48;
	vMax.x = vBottomRight.x - 15;
	vMax.y = vTopLeft.y + 49;	
	//e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);
	
	// Pool allocation counts and sizes
	//
	unsigned int internalSizeTotal = 0;
	unsigned int internalFragmentationTotal = 0;
	unsigned int internalOverheadTotal = 0;
	unsigned int totalExternalAllocationSizeTotal = 0;
	unsigned int totalExternalAllocationCountTotal = 0;
	
	unsigned int allocatorCount = IMemoryManager::GetInstance().GetAllocatorCount();

	for(unsigned int i = 0; i < allocatorCount; ++i)
	{
		IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator(i);
		if(allocator == NULL)
		{
			break;
		}
	
		MEMORY_ALLOCATOR_METRICS metrics;
		allocator->GetInternals()->GetMetrics(metrics);
		
		internalSizeTotal += metrics.InternalSize;
		internalFragmentationTotal += metrics.InternalFragmentation;
		internalOverheadTotal += metrics.InternalOverhead;
		totalExternalAllocationSizeTotal += metrics.TotalExternalAllocationSize;
		totalExternalAllocationCountTotal += metrics.TotalExternalAllocationCount;
		
		vTextPos.y -= 0.05f;
		
		vTextPos.x = -0.79f;		
		PStrPrintf(str, arrsize(str), L"(%d)%s", i, metrics.Name);
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_TOPLEFT);						

		vTextPos.x += 0.41f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.InternalSize);	
		PStrGroupThousands(str, arrsize(str));
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
		
		vTextPos.x += 0.22f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.InternalFragmentation);					
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
		
		vTextPos.x += 0.14f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.ExternalFragmentation);					
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);		
		
		vTextPos.x += 0.24f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.LargestFreeBlock);	
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
		
		vTextPos.x += 0.16f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.InternalOverhead);
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
		
		vTextPos.x += 0.19f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.TotalExternalAllocationSize);
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
		
		vTextPos.x += 0.18f;		
		PStrPrintf(str, arrsize(str), L"%d", metrics.TotalExternalAllocationCount);	
		PStrGroupThousands(str, arrsize(str));			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	}
		
	unsigned int expandedAllocatorIndex = 0xFFFFFFFF;
	int setAverageIndex = 0xFFFFFFFF;
			
	// Handle commands
	//
	if(g_MemorySystemInfoCmd != -1)
	{
		// Allocator expansion
		//
		expandedAllocatorIndex = g_MemorySystemInfoCmd;			
	}	
	
	setAverageIndex = g_MemorySystemInfoSetAverageIndex;
	g_MemorySystemInfoSetAverageIndex = -1;
	
	// Expanded allocator info
	//	
	DWORD now = GetTickCount();		
	
	static const unsigned int originCount = 8;
	static BYTE originMemory[originCount * (sizeof(CMemoryAllocationOrigin*) + sizeof(unsigned int))];
	CSortedArray<unsigned int, CMemoryAllocationOrigin*, false>* origins = NULL;
	CSortedArray<unsigned int, CMemoryAllocationOrigin*, false> originsDecreasing(NULL, false);
	static CSortedArray<unsigned int, CMemoryAllocationOrigin*, false> originsIncreasing(NULL);
	
	if(g_MemorySystemInfoMode == 0)
	{	
		origins = &originsDecreasing;
		origins->Init(originMemory, originCount, 0);
			
		if(expandedAllocatorIndex < allocatorCount)
		{
			IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator(expandedAllocatorIndex);
			allocator->GetInternals()->GetFastestGrowingOrigins(*origins);
		}
		else
		{
			for(unsigned int i = 0; i < allocatorCount; ++i)
			{
				IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator(i);
				
				if(allocator)
				{
					allocator->GetInternals()->GetFastestGrowingOrigins(*origins);
				}
			}
		}
	}
	else if(g_MemorySystemInfoMode == 1)
	{
		static DWORD lastSample = 0;
		static unsigned int lastSampleCount = 0;
		
		origins = &originsIncreasing;
		if(lastSample == 0)
		{
			origins->Init(originMemory, originCount, 0);
		}
		
		
		if(now - lastSample > 2000)
		{
			lastSample = now;
			
			origins->RemoveAll();
		
			if(expandedAllocatorIndex < allocatorCount)
			{
				IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator(expandedAllocatorIndex);
				allocator->GetInternals()->GetRecentlyChangedOrigins(*origins);
			}
			else
			{
				for(unsigned int i = 0; i < allocatorCount; ++i)
				{
					IMemoryAllocator* allocator = IMemoryManager::GetInstance().GetAllocator(i);
					
					if(allocator)
					{
						allocator->GetInternals()->GetRecentlyChangedOrigins(*origins);
					}
				}
			}		
		}
	}	
	
	vTextPos.y -= 0.05f;
	vTextPos.y -= 0.05f;
			
	vTextPos.x = -0.78f;
	WCHAR fileLineHeader[32];
	if(expandedAllocatorIndex < allocatorCount)
	{
		swprintf_s(fileLineHeader, L"(%d)File(Line)", expandedAllocatorIndex);		
	}
	else
	{
		swprintf_s(fileLineHeader, L"(-)File(Line)");		
	}
	
	e_UIAddLabel(fileLineHeader, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.69f;			
	e_UIAddLabel(L"ExternalSize", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);			
	
	vTextPos.x += 0.23f;	
	e_UIAddLabel(L"AverageSize", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
	
	vTextPos.x += 0.12f;			
	e_UIAddLabel(L"Count", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	

	vTextPos.x += 0.15f;			
	e_UIAddLabel(L"Age", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
	
	vTextPos.x += 0.17f;			
	e_UIAddLabel(L"LastChange", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
	
	vTextPos.x += 0.17f;			
	e_UIAddLabel(L"GrowRate", &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);	
	
	// Line under header
	//
	vMin.x = vTopLeft.x + 15;
	vMin.y = vTopLeft.y + 340;
	vMax.x = vBottomRight.x - 15;
	vMax.y = vTopLeft.y + 341;	
	//e_PrimitiveAddBox(PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL, &vMin, &vMax, 0xFFFFFFFF);
	
	for(unsigned int i = 0; i < origins->GetCount(); ++i)
	{
		CMemoryAllocationOrigin* origin = *(*origins)[i];
		
		if((signed)i == setAverageIndex || setAverageIndex == -2)
		{
			origin->SetAverage();
		}
		
		vTextPos.y -= 0.05f;
	
		WCHAR fileName[64];
		PStrCvt(fileName, origin->GetFileName(), arrsize(fileName));				
		WCHAR line[16];
		swprintf_s(line, L"%u", origin->m_Line);
		PStrGroupThousands(line, arrsize(line));																
		swprintf_s(str, L"(%d)%s(%s)", i, fileName, line);
		vTextPos.x = -0.79f;
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPLEFT, UIALIGN_TOPLEFT);
		
		swprintf_s(str, L"%u", origin->m_TotalExternalAllocationSize);
		PStrGroupThousands(str, arrsize(str));								
		vTextPos.x += 0.70f;			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);			
		
		swprintf_s(str, L"%u", origin->GetAverage());
		PStrGroupThousands(str, arrsize(str));												
		vTextPos.x += 0.23f;	
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);
		
		swprintf_s(str, L"%u", origin->m_TotalExternalAllocationCount);
		PStrGroupThousands(str, arrsize(str));																
		vTextPos.x += 0.12f;			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);					

		DWORD allocationAge = (now - origin->m_FirstAllocationTimestamp) / 1000;
		swprintf_s(str, L"%02d:%02d:%02d", allocationAge / 3600, (allocationAge / 60) % 60, allocationAge % 60);
		vTextPos.x += 0.15f;			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);					

		DWORD lastChange = (now - origin->m_CurrentSizeChangeTimestamp) / 1000;
		swprintf_s(str, L"%02d:%02d:%02d", lastChange / 3600, (lastChange / 60) % 60, lastChange % 60);
		vTextPos.x += 0.17f;			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);					

		swprintf_s(str, L"%u", origin->GetGrowRate());
		PStrGroupThousands(str, arrsize(str));																
		vTextPos.x += 0.17f;			
		e_UIAddLabel(str, &vTextPos, FALSE, 1.0f, 0xffffffff, UIALIGN_TOPRIGHT, UIALIGN_TOPLEFT);					
		
	}
		


#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSynch(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	WCHAR str[512];

	UNIT* c_unit = GameGetControlUnit(game);
	if (!c_unit)
	{
		return FALSE;
	}
	GAME* srvGame = AppGetSrvGame();
	if (!srvGame)
	{
		return FALSE;
	}
	UNIT* s_unit = UnitGetById(srvGame, UnitGetId(c_unit));
	if (!s_unit)
	{
		return FALSE;
	}

	PStrPrintf(str, 512, L"[CLIENT]  X:%7.2f  Y:%6.2f  Z:%6.2f  Angle:%3d", 
		c_unit->vPosition.fX, c_unit->vPosition.fY, c_unit->vPosition.fZ, UnitGetPlayerAngle(c_unit));
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"[SERVER]  X:%7.2f  Y:%6.2f  Z:%6.2f  Angle:%3d", 
		s_unit->vPosition.fX, s_unit->vPosition.fY, s_unit->vPosition.fZ, UnitGetPlayerAngle(s_unit));
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateRoomDebugIntArray(
	UI_TEXTBOX* textbox,
	const WCHAR * wszCaption,
	SIMPLE_DYNAMIC_ARRAY<int> & pnArray )
{
	int nCounts[ NUM_ATTACHMENT_TYPES ];
	WCHAR str[512];
	PStrPrintf(str, 512, L"%s: %d", wszCaption, pnArray.Count() ); 
	UITextBoxAddLine(textbox, str);
	for ( unsigned int i = 0; i < pnArray.Count(); i++ )
	{
		ZeroMemory( nCounts, sizeof(int) * NUM_ATTACHMENT_TYPES );
		c_AttachmentCountAttachedByType( pnArray[ i ], nCounts );

		PStrPrintf(str, 512, L"  id[%3d], attached: model[%2d] light[%2d] psystem[%2d] sound[%2d] ropeend[%2d]",
			pnArray[ i ],
			nCounts[ ATTACHMENT_MODEL ],
			nCounts[ ATTACHMENT_LIGHT ],
			nCounts[ ATTACHMENT_PARTICLE ],
			nCounts[ ATTACHMENT_SOUND ],
			nCounts[ ATTACHMENT_PARTICLE_ROPE_END ] ); 
		UITextBoxAddLine(textbox, str);
	}
	UITextBoxAddLine(textbox, L"");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateRoomDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	ROOM* room = NULL;

	UNIT* unit = GameGetControlUnit(game);
	if (unit)
	{
		room = UnitGetRoom(unit);
	}
	else if ( c_GetToolMode() )
	{
		// for tools (like Hammer), get first room in level
		LEVEL* level = LevelGetByID(game, (LEVELID)0);
		if( level )
			room = LevelGetFirstRoom(level);
	}
	if (!room)
	{
		return FALSE;
	}

	enum
	{
		TEMP_STR_LEN = 128,
	};
	WCHAR uszTemp[TEMP_STR_LEN];
	int nRootModel = RoomGetRootModel( room );
	PStrPrintf(str, 512, L"%s (id %d) Root Model:%3d",
		PStrCvt(uszTemp, room->pDefinition->tHeader.pszName, TEMP_STR_LEN),
		room->idRoom,
		nRootModel);
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%S",
		room->pLayout ? room->pLayout->tHeader.pszName : "default");
	UITextBoxAddLine(textbox, str);

	// add level info
	LEVEL *pLevel = RoomGetLevel( room );
	PStrPrintf( uszTemp, TEMP_STR_LEN, L"Level Info: %S", pLevel->m_szLevelName );
	UITextBoxAddLine(textbox, uszTemp );	
	
	// spacer
	UITextBoxAddLine(textbox, L"");

	sUIUpdateRoomDebugIntArray( textbox, L"Static Models", room->pnRoomModels );
	sUIUpdateRoomDebugIntArray( textbox, L"Dynamic Models", room->pnModels );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateLayoutRoomDisplay(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	ROOM* room = NULL;

	ASSERT_RETFALSE( c_GetToolMode() );
	// for tools (like Hammer), get first room in level
	LEVEL* level = LevelGetByID(game, (LEVELID)0);
	if( level )
		room = LevelGetFirstRoom(level);
	if (!room)
	{
		return FALSE;
	}

	PStrPrintf(str, 512, L"%S", 
		room->pDefinition->tHeader.pszName);
	UITextBoxAddLine(textbox, str);
	
	PStrPrintf(str, 512, L"%S", 
		room->pLayout ? room->pLayout->tHeader.pszName : "default");
	UITextBoxAddLine(textbox, str);
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateModelDebugAttachments(
	UI_TEXTBOX* textbox,
	const WCHAR * wszCaption,
	ATTACHMENT_TYPE eType,
	int nOwnerID )
{
	WCHAR str[512], temp[32];
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nOwnerID );
	ASSERT_RETURN( pHolder );
	PStrPrintf(str, 512, L"%s: ", wszCaption ); 
	for ( int nIndex = 0; nIndex < pHolder->nCount; nIndex++ )
	{
		if ( pHolder->pAttachments[ nIndex ].eType == eType )
		{
			ASSERT( pHolder->pAttachments[ nIndex ].nAttachedId >= 0 );
			PStrPrintf(temp, 32, L"%d ", pHolder->pAttachments[ nIndex ].nAttachedId );
			PStrCat( str, temp, 512 );
		}
	}
	UITextBoxAddLine(textbox, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateModelDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	enum
	{
		TEMP_STR_LEN = 128,
	};
	int nModelID = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	if ( nModelID == INVALID_ID )
		return FALSE;

	int nCounts[ NUM_ATTACHMENT_TYPES ];
	ZeroMemory( nCounts, sizeof(int) * NUM_ATTACHMENT_TYPES );
	c_AttachmentCountAttachedByType( nModelID, nCounts );

	PStrPrintf(str, 512, L"Model [%d], attached: model[%2d] light[%2d] psystem[%2d] sound[%2d] ropeend[%2d]",
		nModelID,
		nCounts[ ATTACHMENT_MODEL ],
		nCounts[ ATTACHMENT_LIGHT ],
		nCounts[ ATTACHMENT_PARTICLE ],
		nCounts[ ATTACHMENT_SOUND ],
		nCounts[ ATTACHMENT_PARTICLE_ROPE_END ] ); 
	UITextBoxAddLine(textbox, str);
	UITextBoxAddLine(textbox, L"");

	sUIUpdateModelDebugAttachments( textbox, L"Models", ATTACHMENT_MODEL, nModelID );
	sUIUpdateModelDebugAttachments( textbox, L"Light", ATTACHMENT_LIGHT, nModelID );
	sUIUpdateModelDebugAttachments( textbox, L"PSystem", ATTACHMENT_PARTICLE, nModelID );
	sUIUpdateModelDebugAttachments( textbox, L"Sound", ATTACHMENT_SOUND, nModelID );
	sUIUpdateModelDebugAttachments( textbox, L"RopeEnd", ATTACHMENT_PARTICLE_ROPE_END, nModelID );
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareParticleSystems ( 
	const void * p1,
	const void * p2)
{
	PARTICLE_SYSTEM_DEFINITION * pFirst  = *(PARTICLE_SYSTEM_DEFINITION **) p1;
	PARTICLE_SYSTEM_DEFINITION * pSecond = *(PARTICLE_SYSTEM_DEFINITION **) p2;
	if ( pFirst->nRuntimeParticleUpdatedCount > pSecond->nRuntimeParticleUpdatedCount )
		return -1;
	if ( pFirst->nRuntimeParticleUpdatedCount < pSecond->nRuntimeParticleUpdatedCount )
		return 1;
	return 0;
}


//----------------------------------------------------------------------------
static BOOL sUIUpdateAppearance(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if ( ! bVisible )
		return TRUE;

	APPEARANCE_METRICS tMetrics;
	c_AppearanceGetDebugMetrics( tMetrics );

	WCHAR strOutput[512];
	PStrPrintf( strOutput, 512, L"Skeleton Step: %5d\tSample Anims: %5d\tWorld Update:%5d", 
		tMetrics.nSkeletonStepCountPerFrame,
		tMetrics.nSampleAndCombineCountPerFrame,
		tMetrics.nWorldStepCountPerFrame );
	UITextBoxAddLine(textbox, strOutput);

	UITextBoxAddLine(textbox, L"");
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sgnDPSTimer = 0;
void UIResetDPSTimer()
{
	GAME * pClientGame = AppGetCltGame();
	UNIT * pClientUnit = GameGetControlUnit( pClientGame );
	if ( ! pClientUnit )
		return;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME * pServerGame = AppGetSrvGame();
	if ( ! pServerGame )
		return;

	UNIT * pServerUnit = UnitGetById( pServerGame, UnitGetId( pClientUnit ) );
	if ( ! pServerUnit )
		return;

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(pServerUnit, TAG_SELECTOR_DAMAGE_DONE);
	if ( ! pTag )
		return;

	pTag->m_nTotalMeasured = 0;

	if ( pClientGame )
		sgnDPSTimer = GameGetTick( pClientGame );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUIUpdateDPS(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if ( ! bVisible )
		return TRUE;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	UNIT * pClientUnit = GameGetControlUnit( game );

	GAME * pServerGame = AppGetSrvGame();
	if ( ! pServerGame )
		return TRUE;

	UNIT * pServerUnit = UnitGetById( pServerGame, UnitGetId( pClientUnit ) );
	if ( ! pServerUnit )
		return TRUE;

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(pServerUnit, TAG_SELECTOR_DAMAGE_DONE);
	if ( ! pTag )
		return TRUE;

	int pnDamageDoneValues[ 4 ];
	int pnTimeValues[ 4 ];
	pnDamageDoneValues[ 0 ] = VMDamageDone( pServerUnit, 5  );	pnTimeValues[ 0 ] = 5  * GAME_TICKS_PER_SECOND;
	pnDamageDoneValues[ 1 ] = VMDamageDone( pServerUnit, 10 );	pnTimeValues[ 1 ] = 10 * GAME_TICKS_PER_SECOND;
	pnDamageDoneValues[ 2 ] = VMDamageDone( pServerUnit, 30 );	pnTimeValues[ 2 ] = 30 * GAME_TICKS_PER_SECOND;
	pnDamageDoneValues[ 3 ] = pTag->m_nTotalMeasured;			pnTimeValues[ 3 ] = GameGetTick( game ) - sgnDPSTimer;

	float pfDPS[ 4 ];
	for ( int i = 0; i < 4; i++ )
	{
		if ( pnTimeValues[ i ] == 0 )
			pfDPS[ i ] = 0.0f;
		else
		{
			pfDPS[ i ] = (float)( (pnDamageDoneValues[ i ] >> StatsGetShift( game, STATS_HP_CUR )) * GAME_TICKS_PER_SECOND ) / pnTimeValues[ i ];
		}
		WCHAR strOutput[512];
		PStrPrintf( strOutput, 512, L"DPS %5.2f %3d seconds", pfDPS[ i ], pnTimeValues[ i ] / GAME_TICKS_PER_SECOND );
		UITextBoxAddLine(textbox, strOutput);
	}

	UITextBoxAddLine(textbox, L"");
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sUIUpdateParticleDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if ( ! bVisible )
		return FALSE;

	UITextBoxAddLine(textbox, AppTestFlag( AF_CENSOR_PARTICLES ) ? L"Particles Censored" : L"Particles Not Censored" );

#define MAX_PARTICLE_SYSTEMS_FOR_DEBUG 20
	PARTICLE_SYSTEM_DEFINITION * pSystemsToDisplay[ MAX_PARTICLE_SYSTEMS_FOR_DEBUG ];
	int nSystemCount = 0;
	{
		int nDef = DefinitionGetFirstId( DEFINITION_GROUP_PARTICLE );
		while ( nDef != INVALID_ID )
		{
			PARTICLE_SYSTEM_DEFINITION * pDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nDef );
			if ( pDef->nRuntimeParticleUpdatedCount || pDef->nRuntimeRopeUpdatedCount )
			{
				int nWeight = pDef->nRuntimeParticleUpdatedCount + 5 * pDef->nRuntimeRopeUpdatedCount;
				if ( nSystemCount < MAX_PARTICLE_SYSTEMS_FOR_DEBUG )
					pSystemsToDisplay[ nSystemCount++ ] = pDef;
				else 
				{
					for ( int i = 0; i < nSystemCount; i++ )
					{
						int nWeightCurr = pSystemsToDisplay[ i ]->nRuntimeParticleUpdatedCount + 5 * pSystemsToDisplay[ i ]->nRuntimeRopeUpdatedCount;
						if ( nWeightCurr < nWeight )
						{
							pSystemsToDisplay[ i ] = pDef;
							break;
						}
					}
				}
			}
			nDef = DefinitionGetNextId( DEFINITION_GROUP_PARTICLE, nDef );
		}
	}

	qsort(pSystemsToDisplay, nSystemCount, sizeof(PARTICLE_SYSTEM_DEFINITION *), sCompareParticleSystems);
 
	WCHAR strName[512];
	WCHAR strCombined[512];
	for ( int i = 0; i < nSystemCount; i++ )
	{
		PARTICLE_SYSTEM_DEFINITION * pDef = pSystemsToDisplay[ i ];
		PStrCvt( strName, pDef->tHeader.pszName, 512 );
		if ( pDef->nRuntimeRopeUpdatedCount )
			PStrPrintf(strCombined, 512, L"%40s\t%d Rope", strName, pDef->nRuntimeRopeUpdatedCount );
		else
			PStrPrintf(strCombined, 512, L"%40s\t%d", strName, pDef->nRuntimeParticleUpdatedCount );
		UITextBoxAddLine(textbox, strCombined);
	}

	UITextBoxAddLine(textbox, L"");

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetSoundHeaderText(
	UI_TEXTBOX* textbox,
	int (*pPlayingFunc)(),
	int (*pActiveFunc)())
{
	ASSERT(pPlayingFunc && pActiveFunc);
	WCHAR strLeft[512];

	VECTOR vListener = c_SoundGetListenerPosition();
	float dsp, stream, update, total;
	c_SoundGetCPUUsage(&dsp, &stream, &update, &total);
	int memory;
	c_SoundGetMemoryUsage(&memory, NULL);
	PStrPrintf(strLeft, 512,
		L"dsp: %f%% stream: %f%% update: %f%% total: %f%%\n"
		L"Mem: %d Sound Count: %d/%d Listener: (%.2f, %.2f, %.2f)",
		dsp, stream, update, total, memory,
		(*pPlayingFunc)(), (*pActiveFunc)(),
		vListener.fX, vListener.fY, vListener.fZ );
	UITextBoxAddLine(textbox, strLeft);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetSoundInfoText(WCHAR* str, const char * pszName, int nId)
{
	WCHAR pwName[512];
	if ( pszName )
	{
		BOOL bIsVirtual   = c_SoundGetIsVirtual(nId);
		float fAmplitude  = c_SoundGetAmplitude(nId);
		BOOL bIsFinished  = c_SoundGetIsFinished(nId);
		BOOL bIsMuted     = c_SoundGetMute(nId);
		const VECTOR vPos = c_SoundGetPosition(nId);
		float fOcclusion  = c_SoundGetOcclusion(nId);
		float minimum, maximum;
		c_SoundGetMinMaxDistance(nId, &minimum, &maximum);
		PStrCvt( pwName, pszName, 512 );
		PStrPrintf(str, 512, L"Sound %d (%s%s%s) %d(%.3f)-%.2f (%.2f, %.2f, %.2f) (%.2f - %.2f) %s\t",
			nId,
			bIsVirtual ? L"v" : L" ",
			bIsFinished ? L"d" : L" ",
			bIsMuted ? L"m" : L" ",
			(fAmplitude > 0.0f) ? int(VolumeTodB(fAmplitude)) : 0,
			fAmplitude,
			fOcclusion,
			vPos.fX, vPos.fY, vPos.fZ,
			minimum, maximum,
			pwName );
	}
	else
	{
		PStrPrintf(str, 512, L"");
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetSoundDebug(
	UI_TEXTBOX* textbox,
	int (*pFirstFunc)(),
	const char * (*pNextFunc)(SOUND_ID,int&))
{
	ASSERT(pFirstFunc);
	ASSERT(pNextFunc);

	WCHAR strLeft[512];
	WCHAR strRight[512];
	WCHAR strCombined[512];

	int nId = (*pFirstFunc)();
	while ( nId != INVALID_ID )
	{
		int nIdNext;
		const char * pszName = (*pNextFunc)( nId, nIdNext );
		sGetSoundInfoText(strLeft, pszName, nId);
		nId = nIdNext;

		if (nId != INVALID_ID)
		{
			PStrPrintf(strRight, 512, L"");
			/*
			pszName = c_SoundGetNextName( nId, nIdNext );
			sGetSoundInfoText(strRight, pszName, nId);
			nId = nIdNext;
			// */
		}
		else
		{
			PStrPrintf(strRight, 512, L"");
		}

		PStrPrintf(strCombined, 512, L"%s\t%s", strLeft, strRight);
		UITextBoxAddLine(textbox, strCombined);
	}
	UITextBoxAddLine(textbox, L"");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSoundDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if !ISVERSION(SERVER_VERSION)
	if (!bVisible)
		return FALSE;

	sGetSoundHeaderText(textbox, c_SoundGetNumPlaying, c_SoundGetNumActive);

	sGetSoundDebug(textbox, c_SoundGetFirstInstance, c_SoundGetNextName);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateVirtualSoundDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if !ISVERSION(SERVER_VERSION)
	if (!bVisible)
		return FALSE;

	sGetSoundHeaderText(textbox, c_SoundGetNumVirtualActive, c_SoundGetNumVirtualActive);

	sGetSoundDebug(textbox, c_SoundGetFirstVirtualInstance, c_SoundGetNextVirtualName);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateStoppedSoundDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if !ISVERSION(SERVER_VERSION)
	if (!bVisible)
		return FALSE;

	sGetSoundHeaderText(textbox, c_SoundGetNumStoppedActive, c_SoundGetNumStoppedActive);

	sGetSoundDebug(textbox, c_SoundGetFirstStoppedInstance, c_SoundGetNextStoppedName);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSoundMemory(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	textbox->m_nFontSize = 6;
	char pszRow[MAX_XML_STRING_LENGTH];
	WCHAR wszRow[MAX_XML_STRING_LENGTH];

	int memory = 0;
	c_SoundGetMemoryUsage(&memory, NULL);
	PStrPrintf(pszRow, MAX_XML_STRING_LENGTH, "Memory usage: %d / %d\n", memory, c_SoundMemoryDebugGetAllocated());
	PStrCvt(wszRow, pszRow, MAX_XML_STRING_LENGTH);
	UITextBoxAddLine(textbox, wszRow);

	PStrPrintf(pszRow, MAX_XML_STRING_LENGTH, "%-60s%-60s%-60s%-60s", "Requested", "Allocated", "Loading", "Playing");
	PStrCvt(wszRow, pszRow, MAX_XML_STRING_LENGTH);
	UITextBoxAddLine(textbox, wszRow);

	int nMaxRowCount = c_SoundMemoryDebugGetMaxRows();
	for(int i=0; i<nMaxRowCount; i+=2)
	{
		c_SoundMemoryDebugGetRow(i, pszRow, MAX_XML_STRING_LENGTH);

		PStrCvt(wszRow, pszRow, MAX_XML_STRING_LENGTH);
		UITextBoxAddLine(textbox, wszRow);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetMusicInfo(
	UI_TEXTBOX* textbox,
	const MUSIC_TRACK * pMusicTrack)
{
	ASSERT(pMusicTrack);

	WCHAR str[512];
	char pszName[128];
	int nUnits = 0;
	if (pMusicTrack->pMusicData)
	{
		PStrCopy(pszName, pMusicTrack->pMusicData->pszName, 128);
	}
	else
	{
		PStrCopy(pszName, "None", 128);
	}

	int nActiveTrack = pMusicTrack->nCurrentActiveTrack;
	int nPendingTrack = !nActiveTrack;

	char pszGrooveName[128];
	if (pMusicTrack->pGrooveData[nActiveTrack])
	{
		PStrCopy(pszGrooveName, pMusicTrack->pGrooveData[nActiveTrack]->pszName, 128);
	}
	else
	{
		PStrCopy(pszGrooveName, "None", 128);
	}

	char pszGrooveNamePending[128];
	if (pMusicTrack->pGrooveData[nPendingTrack])
	{
		PStrCopy(pszGrooveNamePending, pMusicTrack->pGrooveData[nPendingTrack]->pszName, 128);
	}
	else
	{
		PStrCopy(pszGrooveNamePending, "None", 128);
	}
	
	PStrPrintf(str, 512, L"Track: %S  Groove Level: %S", pszName, pszGrooveName);
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"Measure: %d %d/%d", (pMusicTrack->nCurrentMeasure+1), (pMusicTrack->nCurrentBeat+1), nUnits);
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"Pending Groove Level: %S", pszGrooveNamePending);
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"Groove Change Reason History:");
	UITextBoxAddLine(textbox, str);
	for(int i=0; i<MUSIC_CONDITION_HISTORY; i++)
	{
		PStrPrintf(str, 512, L"  %d: %S [%S, %d]", (i+1), pszMusicGrooveChangeReasons[pMusicTrack->pTrackers[i].eReason], pMusicTrack->pTrackers[i].pReasonMusicCondition ? pMusicTrack->pTrackers[i].pReasonMusicCondition->pszName : "", (pMusicTrack->pTrackers[i].nReasonConditionIndex+1));
		UITextBoxAddLine(textbox, str);
	}
	PStrPrintf(str, 512, L"State: %S - %.2f | %.2f | %d | %d", pszMusicStateNames[int(pMusicTrack->eState)], pMusicTrack->fCurrentBeat, pMusicTrack->fBeatsPerTick);
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"Currently Active: %d", pMusicTrack->nCurrentActiveTrack);
	UITextBoxAddLine(textbox, str);
	const int MAX_VOLUMES = 16;
	float fVolumes[MAX_VOLUMES];
	int nCount = c_MusicGetMusicChannelGroupVolumes(pMusicTrack, fVolumes);
	ASSERT(nCount <= MAX_VOLUMES);
	PStrPrintf(str, 512, L"Volumes: ");
	for(int i=0; i<nCount; i++)
	{
		PStrPrintf(str, 512, L"%s%.3f, ", str, fVolumes[i]);
	}
	UITextBoxAddLine(textbox, str);

	for(int i=0; i<MAX_ACTIVE_TRACKS; i++)
	{
		MUSIC_GROOVE_LEVEL_DATA * pGrooveData = pMusicTrack->pGrooveData[i];
		float fVolume = pGrooveData ? pGrooveData->fVolume : 1.0f;
		float fActualVolume = c_MusicGetMusicVolume(pMusicTrack, i);

		char pszMusicName[128];
		if (pMusicTrack->pMusicData && pMusicTrack->pMusicData->pMusicRef && pGrooveData && pGrooveData->nTrackNumber >= 0)
		{
			PStrCopy(pszMusicName, pMusicTrack->pMusicData->pMusicRef->pSoundGroup->pSoundData[pGrooveData->nTrackNumber].pszFilename, 128);
		}
		else
		{
			PStrCopy(pszMusicName, "None", 128);
		}
		PStrPrintf(str, 512, L"%S - %.2f/%.2f", pszMusicName, fActualVolume, fVolume);
		UITextBoxAddLine(textbox, str);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateMusicDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	UITextBoxAddLine(textbox, L"Track 1:");
	const MUSIC_TRACK * pMusicTrack = c_MusicGetMusicTrack();
	sGetMusicInfo(textbox, pMusicTrack);

	WCHAR pszExtraText[2048];
	c_MusicGetChannelStrings(pMusicTrack, pszExtraText, 2048);
	UITextBoxAddLine(textbox, pszExtraText);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateMusicScriptDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	if(!game || !IS_CLIENT(game))
		return FALSE;

	UNIT * pControlUnit = GameGetControlUnit(game);
	if(!pControlUnit)
		return FALSE;

	const MUSIC_TRACK * pMusicTrack = c_MusicGetMusicTrack();
	int nActiveTrack = pMusicTrack->nCurrentActiveTrack;
	int nPendingTrack = !nActiveTrack;

	char pszGrooveName[128];
	if (pMusicTrack->pGrooveData[nActiveTrack])
	{
		PStrCopy(pszGrooveName, pMusicTrack->pGrooveData[nActiveTrack]->pszName, 128);
	}
	else
	{
		PStrCopy(pszGrooveName, "None", 128);
	}

	char pszGrooveNamePending[128];
	if (pMusicTrack->pGrooveData[nPendingTrack])
	{
		PStrCopy(pszGrooveNamePending, pMusicTrack->pGrooveData[nPendingTrack]->pszName, 128);
	}
	else
	{
		PStrCopy(pszGrooveNamePending, "None", 128);
	}

	WCHAR str[512];
	PStrPrintf(str, 512, L"Current Groove: [%S] Pending Groove: [%S]", pszGrooveName, pszGrooveNamePending);
	UITextBoxAddLine(textbox, str);
	PStrPrintf(str, 512, L"Groove Change Reason History:");
	UITextBoxAddLine(textbox, str);
	for(int i=0; i<MUSIC_CONDITION_HISTORY; i++)
	{
		PStrPrintf(str, 512, L"  %d: %S [%S, %d]", (i+1), pszMusicGrooveChangeReasons[pMusicTrack->pTrackers[i].eReason], pMusicTrack->pTrackers[i].pReasonMusicCondition ? pMusicTrack->pTrackers[i].pReasonMusicCondition->pszName : "", (pMusicTrack->pTrackers[i].nReasonConditionIndex+1));
		UITextBoxAddLine(textbox, str);
	}

	int nCount = ExcelGetCount(game, DATATABLE_MUSIC_SCRIPT_DEBUG);
	for(int i=0; i<nCount; i++)
	{
		const MUSIC_SCRIPT_DEBUG_DATA * pMusicScriptDebugData = (const MUSIC_SCRIPT_DEBUG_DATA *)ExcelGetData(game, DATATABLE_MUSIC_SCRIPT_DEBUG, i);
		if(!pMusicScriptDebugData)
			continue;

		int codelen = 0;
		BYTE * code = ExcelGetScriptCode(EXCEL_CONTEXT(game), DATATABLE_MUSIC_SCRIPT_DEBUG, pMusicScriptDebugData->codeScript, &codelen);
		if(code)
		{
			int nResult = VMExecI(game, pControlUnit, code, codelen);
			PStrPrintf(str, 512, L"%-32S - %8d", pMusicScriptDebugData->pszName, nResult);
			UITextBoxAddLine(textbox, str);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateReverbDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if !ISVERSION(SERVER_VERSION)
	if (!bVisible)
		return FALSE;

	SOUND_REVERB_DEFINITION * pCurrent = NULL;
	SOUND_REVERB_DEFINITION * pTarget = NULL;
	SOUND_REVERB_DEFINITION * pOverride = NULL;
	SOUND_REVERB_DEFINITION * pInstant = NULL;
	SOUND_REVERB_DEFINITION * pDefault = NULL;
	SOUND_REVERB_DEFINITION * pFMOD = NULL;
	float fFadePCt = 0.0f;
	c_SoundGetReverbState(&pCurrent, &pTarget, &pOverride, &pInstant, &pDefault, &pFMOD, &fFadePCt);

	WCHAR str[512];
	PStrPrintf(str, 512, L"Fade Percent: %.2f", fFadePCt);
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10S%10S%10S%10S%10S%10S", "Value", "Cur", "Tar", "Ovr", "Ins", "Def", "FMOD");
	UITextBoxAddLine(textbox, str);

#define VALUEOF(reverb, value) ((reverb && reverb->pReverb) ? reverb->pReverb->value : 0)
	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "Environment", VALUEOF(pCurrent, Environment), VALUEOF(pTarget, Environment), VALUEOF(pOverride, Environment), VALUEOF(pInstant, Environment), VALUEOF(pDefault, Environment), VALUEOF(pFMOD, Environment));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "EnvSize", VALUEOF(pCurrent, EnvSize), VALUEOF(pTarget, EnvSize), VALUEOF(pOverride, EnvSize), VALUEOF(pInstant, EnvSize), VALUEOF(pDefault, EnvSize), VALUEOF(pFMOD, EnvSize));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "EnvDiffusion", VALUEOF(pCurrent, EnvDiffusion), VALUEOF(pTarget, EnvDiffusion), VALUEOF(pOverride, EnvDiffusion), VALUEOF(pInstant, EnvDiffusion), VALUEOF(pDefault, EnvDiffusion), VALUEOF(pFMOD, EnvDiffusion));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "Room", VALUEOF(pCurrent, Room), VALUEOF(pTarget, Room), VALUEOF(pOverride, Room), VALUEOF(pInstant, Room), VALUEOF(pDefault, Room), VALUEOF(pFMOD, Room));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "RoomHF", VALUEOF(pCurrent, RoomHF), VALUEOF(pTarget, RoomHF), VALUEOF(pOverride, RoomHF), VALUEOF(pInstant, RoomHF), VALUEOF(pDefault, RoomHF), VALUEOF(pFMOD, RoomHF));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "RoomLF", VALUEOF(pCurrent, RoomLF), VALUEOF(pTarget, RoomLF), VALUEOF(pOverride, RoomLF), VALUEOF(pInstant, RoomLF), VALUEOF(pDefault, RoomLF), VALUEOF(pFMOD, RoomLF));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "DecayTime", VALUEOF(pCurrent, DecayTime), VALUEOF(pTarget, DecayTime), VALUEOF(pOverride, DecayTime), VALUEOF(pInstant, DecayTime), VALUEOF(pDefault, DecayTime), VALUEOF(pFMOD, DecayTime));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "DecayHFRatio", VALUEOF(pCurrent, DecayHFRatio), VALUEOF(pTarget, DecayHFRatio), VALUEOF(pOverride, DecayHFRatio), VALUEOF(pInstant, DecayHFRatio), VALUEOF(pDefault, DecayHFRatio), VALUEOF(pFMOD, DecayHFRatio));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "DecayLFRatio", VALUEOF(pCurrent, DecayLFRatio), VALUEOF(pTarget, DecayLFRatio), VALUEOF(pOverride, DecayLFRatio), VALUEOF(pInstant, DecayLFRatio), VALUEOF(pDefault, DecayLFRatio), VALUEOF(pFMOD, DecayLFRatio));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "Reflections", VALUEOF(pCurrent, Reflections), VALUEOF(pTarget, Reflections), VALUEOF(pOverride, Reflections), VALUEOF(pInstant, Reflections), VALUEOF(pDefault, Reflections), VALUEOF(pFMOD, Reflections));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReflectionsDelay", VALUEOF(pCurrent, ReflectionsDelay), VALUEOF(pTarget, ReflectionsDelay), VALUEOF(pOverride, ReflectionsDelay), VALUEOF(pInstant, ReflectionsDelay), VALUEOF(pDefault, ReflectionsDelay), VALUEOF(pFMOD, ReflectionsDelay));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReflectionsPan[0]", VALUEOF(pCurrent, ReflectionsPan[0]), VALUEOF(pTarget, ReflectionsPan[0]), VALUEOF(pOverride, ReflectionsPan[0]), VALUEOF(pInstant, ReflectionsPan[0]), VALUEOF(pDefault, ReflectionsPan[0]), VALUEOF(pFMOD, ReflectionsPan[0]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReflectionsPan[1]", VALUEOF(pCurrent, ReflectionsPan[1]), VALUEOF(pTarget, ReflectionsPan[1]), VALUEOF(pOverride, ReflectionsPan[1]), VALUEOF(pInstant, ReflectionsPan[1]), VALUEOF(pDefault, ReflectionsPan[1]), VALUEOF(pFMOD, ReflectionsPan[1]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReflectionsPan[2]", VALUEOF(pCurrent, ReflectionsPan[2]), VALUEOF(pTarget, ReflectionsPan[2]), VALUEOF(pOverride, ReflectionsPan[2]), VALUEOF(pInstant, ReflectionsPan[2]), VALUEOF(pDefault, ReflectionsPan[2]), VALUEOF(pFMOD, ReflectionsPan[2]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10d%10d%10d%10d%10d%10d", "Reverb", VALUEOF(pCurrent, Reverb), VALUEOF(pTarget, Reverb), VALUEOF(pOverride, Reverb), VALUEOF(pInstant, Reverb), VALUEOF(pDefault, Reverb), VALUEOF(pFMOD, Reverb));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReverbDelay", VALUEOF(pCurrent, ReverbDelay), VALUEOF(pTarget, ReverbDelay), VALUEOF(pOverride, ReverbDelay), VALUEOF(pInstant, ReverbDelay), VALUEOF(pDefault, ReverbDelay), VALUEOF(pFMOD, ReverbDelay));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReverbPan[0]", VALUEOF(pCurrent, ReverbPan[0]), VALUEOF(pTarget, ReverbPan[0]), VALUEOF(pOverride, ReverbPan[0]), VALUEOF(pInstant, ReverbPan[0]), VALUEOF(pDefault, ReverbPan[0]), VALUEOF(pFMOD, ReverbPan[0]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReverbPan[1]", VALUEOF(pCurrent, ReverbPan[1]), VALUEOF(pTarget, ReverbPan[1]), VALUEOF(pOverride, ReverbPan[1]), VALUEOF(pInstant, ReverbPan[1]), VALUEOF(pDefault, ReverbPan[1]), VALUEOF(pFMOD, ReverbPan[1]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ReverbPan[2]", VALUEOF(pCurrent, ReverbPan[2]), VALUEOF(pTarget, ReverbPan[2]), VALUEOF(pOverride, ReverbPan[2]), VALUEOF(pInstant, ReverbPan[2]), VALUEOF(pDefault, ReverbPan[2]), VALUEOF(pFMOD, ReverbPan[2]));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "EchoTime", VALUEOF(pCurrent, EchoTime), VALUEOF(pTarget, EchoTime), VALUEOF(pOverride, EchoTime), VALUEOF(pInstant, EchoTime), VALUEOF(pDefault, EchoTime), VALUEOF(pFMOD, EchoTime));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "EchoDepth", VALUEOF(pCurrent, EchoDepth), VALUEOF(pTarget, EchoDepth), VALUEOF(pOverride, EchoDepth), VALUEOF(pInstant, EchoDepth), VALUEOF(pDefault, EchoDepth), VALUEOF(pFMOD, EchoDepth));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ModulationTime", VALUEOF(pCurrent, ModulationTime), VALUEOF(pTarget, ModulationTime), VALUEOF(pOverride, ModulationTime), VALUEOF(pInstant, ModulationTime), VALUEOF(pDefault, ModulationTime), VALUEOF(pFMOD, ModulationTime));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "ModulationDepth", VALUEOF(pCurrent, ModulationDepth), VALUEOF(pTarget, ModulationDepth), VALUEOF(pOverride, ModulationDepth), VALUEOF(pInstant, ModulationDepth), VALUEOF(pDefault, ModulationDepth), VALUEOF(pFMOD, ModulationDepth));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "AirAbsorptionHF", VALUEOF(pCurrent, AirAbsorptionHF), VALUEOF(pTarget, AirAbsorptionHF), VALUEOF(pOverride, AirAbsorptionHF), VALUEOF(pInstant, AirAbsorptionHF), VALUEOF(pDefault, AirAbsorptionHF), VALUEOF(pFMOD, AirAbsorptionHF));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "HFReference", VALUEOF(pCurrent, HFReference), VALUEOF(pTarget, HFReference), VALUEOF(pOverride, HFReference), VALUEOF(pInstant, HFReference), VALUEOF(pDefault, HFReference), VALUEOF(pFMOD, HFReference));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "LFReference", VALUEOF(pCurrent, LFReference), VALUEOF(pTarget, LFReference), VALUEOF(pOverride, LFReference), VALUEOF(pInstant, LFReference), VALUEOF(pDefault, LFReference), VALUEOF(pFMOD, LFReference));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%10.2f%10.2f%10.2f%10.2f%10.2f%10.2f", "RoomRolloffFactor", VALUEOF(pCurrent, RoomRolloffFactor), VALUEOF(pTarget, RoomRolloffFactor), VALUEOF(pOverride, RoomRolloffFactor), VALUEOF(pInstant, RoomRolloffFactor), VALUEOF(pDefault, RoomRolloffFactor), VALUEOF(pFMOD, RoomRolloffFactor));
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"%-20S%#10x%#10x%#10x%#10x%#10x%#10x", "Flags", VALUEOF(pCurrent, Flags), VALUEOF(pTarget, Flags), VALUEOF(pOverride, Flags), VALUEOF(pInstant, Flags), VALUEOF(pDefault, Flags), VALUEOF(pFMOD, Flags));
	UITextBoxAddLine(textbox, str);

#undef VALUEOF
#else
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(textbox);
	UNREFERENCED_PARAMETER(bVisible);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateSoundBusDebug(
	SOUND_BUS * pSoundBus,
	float fVolume,
	void * userdata)
{
	ASSERT_RETURN(pSoundBus);
	ASSERT_RETURN(userdata);
	UI_TEXTBOX* textbox = (UI_TEXTBOX*)userdata;
	
	WCHAR str[512];
	PStrPrintf(str, 512, L"%-16S - %8.2f - %8.0f", pSoundBus->pszName, fVolume, VolumeTodB(fVolume));
	UITextBoxAddLine(textbox, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSoundBuses(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];
	PStrPrintf(str, 512, L"%-16S - %8S - %8S", "Bus", "Volume", "dB");
	UITextBoxAddLine(textbox, str);

	c_SoundGetBusDebug(sUIUpdateSoundBusDebug, textbox);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateSoundMixStateDebug(
	SOUND_MIXSTATE * pSoundMixState,
	void * userdata)
{
	ASSERT_RETURN(pSoundMixState);
	ASSERT_RETURN(userdata);
	UI_TEXTBOX* textbox = (UI_TEXTBOX*)userdata;
	
	WCHAR str[512];
	PStrPrintf(str, 512, L"%-16S - %d", pSoundMixState->pszName, pSoundMixState->nPriority);
	UITextBoxAddLine(textbox, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSoundMixStates(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];
	PStrPrintf(str, 512, L"%-16S - %S", "Mix State", "Priority");
	UITextBoxAddLine(textbox, str);

	c_SoundGetMixStateDebug(sUIUpdateSoundMixStateDebug, textbox);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateTagDisplay(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if (ISVERSION(DEVELOPMENT))				
	if(!bVisible)
		return FALSE;

	if(game->m_nDebugTagDisplay < 0)
		return FALSE;

	const TAG_DEFINITION * pTagDef = (const TAG_DEFINITION *)ExcelGetData(game, DATATABLE_TAG, game->m_nDebugTagDisplay);
	if(!pTagDef || !pTagDef->bIsValueTimeTag)
		return FALSE;

	UNIT * pUnit = GameGetDebugUnit(game);
	if(!pUnit)
	{
		pUnit = GameGetControlUnit(game);
	}
	if(!pUnit)
		return FALSE;

	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(pUnit, game->m_nDebugTagDisplay);
	if(!pTag)
		return FALSE;

	textbox->m_nFontSize = 10;

	WCHAR str[512];
	PStrPrintf(str, 512, L"%-16S - %-16S - %-16S - %-16S - %-16S", "", "Seconds", "Seconds Sum", "Minutes", "Minutes Sum");
	UITextBoxAddLine(textbox, str);

	int nCount = MAX(VALUE_TIME_HISTORY, VALUE_TIME_HISTORY_MINUTES);
	int nSmaller = MIN(VALUE_TIME_HISTORY, VALUE_TIME_HISTORY_MINUTES);
	for(int i=0; i<nCount; i++)
	{
		if(i < nSmaller)
		{
			PStrPrintf(str, 512, L"%-16d - %-16d - %-16d - %-16d - %-16d", i, 
				GetValueTimeTagValue(pUnit, i, (TAG_SELECTOR)game->m_nDebugTagDisplay), 
				GetValueTimeTagSum(pUnit, (i+1), (TAG_SELECTOR)game->m_nDebugTagDisplay), 
				GetValueTimeTagMinutesValue(pUnit, i, (TAG_SELECTOR)game->m_nDebugTagDisplay), 
				GetValueTimeTagMinutesSum(pUnit, (i+1), (TAG_SELECTOR)game->m_nDebugTagDisplay));
		}
		else
		{
			PStrPrintf(str, 512, L"%-16d - %-16S - %-16S - %-16d - %-16d", i, 
				"", 
				"", 
				GetValueTimeTagMinutesValue(pUnit, i, (TAG_SELECTOR)game->m_nDebugTagDisplay), 
				GetValueTimeTagMinutesSum(pUnit, (i+1), (TAG_SELECTOR)game->m_nDebugTagDisplay));
		}
		UITextBoxAddLine(textbox, str);
	}
#else
	REF(game);
	REF(textbox);
	REF(bVisible);
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateBGSoundDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	const BG_SOUND * pBGSound = c_BGSoundGetState();
	PStrPrintf(str, 512, L"Current State: %S - %S", pszBGSoundStateNames[pBGSound->eMainState], pBGSound->pBGSoundData ? pBGSound->pBGSoundData->pszName : "");
	UITextBoxAddLine(textbox, str);

	PStrPrintf(str, 512, L"Pending State: %S", pBGSound->pBGSoundData ? pBGSound->pBGSoundDataPending->pszName : "");
	UITextBoxAddLine(textbox, str);

	UITextBoxAddLine(textbox, L"");

	UITextBoxAddLine(textbox, L"2D Sounds:");
	for(int i=0; i<MAX_BACKGROUND_SOUNDS_2D; i++)
	{
		if(pBGSound->tSound2D[i].pBGSoundData && pBGSound->tSound2D[i].pBGSoundData->pszName[0])
		{
			char * pszSoundName = "";
			int nGroup = c_SoundGetGroup(pBGSound->tSound2D[i].nSoundId);
			if(nGroup != INVALID_ID)
			{
				SOUND_GROUP* pGroup = (SOUND_GROUP*)ExcelGetData(game, DATATABLE_SOUNDS, nGroup);
				if(pGroup)
				{
					pszSoundName = (char*)pGroup->pszName;
				}
			}
			int nCurrentVolume = pBGSound->tSound2D[i].tFade.GetCurrent();
			int nDesiredVolume = pBGSound->tSound2D[i].tFade.GetDesired();
			PStrPrintf(str, 512, L"%S: %S %S Ticks(%d/%d) Vol(%d/%d) Silent(%.2f)",
				pBGSound->tSound2D[i].pBGSoundData->pszName,
				pszBGSound2DStateNames[pBGSound->eSound2DState[i]],
				pszSoundName,
				pBGSound->tSound2D[i].nTicksRemaining,
				pBGSound->tSound2D[i].nFadeTicksRemaining,
				nCurrentVolume,
				nDesiredVolume,
				pBGSound->tSound2D[i].pBGSoundData->fSilentChance);
			UITextBoxAddLine(textbox, str);
		}
	}
	UITextBoxAddLine(textbox, L"");

	UITextBoxAddLine(textbox, L"3D Sounds:");
	for(int i=0; i<MAX_BACKGROUND_SOUNDS_3D; i++)
	{
		if(pBGSound->tSound3D[i].pBGSoundData && pBGSound->tSound3D[i].pBGSoundData->pszName[0])
		{
			PStrPrintf(str, 512, L"%S: %S Ticks(%d/%d) Set(%d) Pos(%.2f,%.2f,%.2f) Silent(%.2f)",
				pBGSound->tSound3D[i].pBGSoundData->pszName,
				pszBGSound3DStateNames[pBGSound->eSound3DState[i]],
				pBGSound->tSound3D[i].nInterSetTicksRemaining,
				pBGSound->tSound3D[i].nIntraSetTicksRemaining,
				pBGSound->tSound3D[i].nSetCountRemaining,
				pBGSound->tSound3D[i].vPosition.fX, pBGSound->tSound3D[i].vPosition.fY, pBGSound->tSound3D[i].vPosition.fZ,
				(100.0f - pBGSound->tSound3D[i].pBGSoundData->fSetChance));
			UITextBoxAddLine(textbox, str);
		}
	}

	//UITextBoxAddLine(textbox, L"");
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateUIDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR wstr[512];
//	CHAR str[512];

	float x, y;
	UIGetCursorPosition(&x, &y);

	PStrPrintf(wstr, 512, L"UIPos (%0.2f, %0.2f) Left - (%d) Right - (%d)",
		x, y,
		InputGetMouseButtonDown(MOUSE_BUTTON_LEFT),
		InputGetMouseButtonDown(MOUSE_BUTTON_RIGHT));
//	PStrCvt(wstr, str, 512);

	UITextBoxAddLine(textbox, wstr);

	// get the immediate state of the mouse buttons
	BYTE rgbButtons[4] = {0, 0, 0, 0};
	GetMouseButtonStateImmediate(rgbButtons);

	PStrPrintf(wstr, 512, L"GetDeviceState - (%3d) - (%3d) - (%3d) - (%3d)",
		rgbButtons[0],
		rgbButtons[1],
		rgbButtons[2],
		rgbButtons[3]);
	UITextBoxAddLine(textbox, wstr);

	if (g_UI.m_Cursor)
	{
		UNITID idCursorUnit = INVALID_ID;
		UNIT *pCursorUnit = NULL;
		int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
		UNIT * pPlayer = GameGetControlUnit(game);
		if (pPlayer)
		{
			pCursorUnit = UnitInventoryGetByLocation(pPlayer, nCursorLocation);
			if (pCursorUnit)
			{
				idCursorUnit = UnitGetId(pCursorUnit);
			}
		}

		PStrPrintf(wstr, 512, L"Cursor.m_idUnit = %d Cursor Inv Unit = %d from WeaponConfig = %d Last Inv = %d",
			g_UI.m_Cursor->m_idUnit,
			idCursorUnit,
			g_UI.m_Cursor->m_nFromWeaponConfig,
			g_UI.m_Cursor->m_nLastInvLocation);
		UITextBoxAddLine(textbox, wstr);
	}

	if (g_UI.m_pCurrentMouseOverComponent)
	{
		WCHAR szName[256];
		PStrCvt(szName, g_UI.m_pCurrentMouseOverComponent->m_szName, 256);
		PStrPrintf(wstr, 512, L"Current Mouse Over - %d : %s", g_UI.m_pCurrentMouseOverComponent->m_idComponent, szName);
	}
	else
	{
		PStrPrintf(wstr, 512, L"Current Mouse Over - (NONE)");
	}

	UI_COMPONENT *pOverride = UIGetMouseOverrideComponent();
	if (pOverride)
	{
		WCHAR szName[256];
		PStrCvt(szName, pOverride->m_szName, 256);
		PStrPrintf(wstr, 512, L"Override Mouse Component - %d : %s", pOverride->m_idComponent, szName);
	}

	UITextBoxAddLine(textbox, wstr);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateUIGfxDebug(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR wstr[512];
//	CHAR str[512];

	float x, y;
	UIGetCursorPosition(&x, &y);

	PStrPrintf(wstr, 512, L"TextureDraws (garbage) - %4d (%4d)",
		UIDebugGfxCountTextureDraws(),
		UIDebugGfxCountTextureDrawsGarbage());

	UITextBoxAddLine(textbox, wstr);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateDebugUIEdit(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if ISVERSION(DEVELOPMENT)
	if (!bVisible)
		return FALSE;

	WCHAR wstr[512];

	if (g_UI.m_idDebugEditComponent != INVALID_ID)
	{
		UI_COMPONENT *pComponent = UIComponentGetById(g_UI.m_idDebugEditComponent);
		ASSERTONCE_RETVAL(pComponent, FALSE);

		WCHAR szName[256];
		PStrCvt(szName, pComponent->m_szName, 256);

		UIX_TEXTURE *pTexture = UIComponentGetTexture(pComponent);
		WCHAR szTextureName[256];
		PStrCvt(szTextureName, (pTexture ? pTexture->m_pszFileName : "(NULL)"), 256);

		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pComponent);
		WCHAR szFontName[256];
		PStrCvt(szFontName, (pFont ? pFont->m_szSystemName : "(NULL)"), 256);

		PStrPrintf(wstr, 512, L"%d: %s\n(%0.2f, %0.2f) - (%0.2f, %0.2f) - scrollpos = %f\nvisible=%d  active=%d  rendersection=%d\n useractive=%d texture=%s font=%s\nfading=%0.2f",
			pComponent->m_idComponent,
			szName,
			pComponent->m_Position.m_fX,
			pComponent->m_Position.m_fY,
			pComponent->m_fWidth,
			pComponent->m_fHeight,
			pComponent->m_ScrollPos.m_fY,
			pComponent->m_bVisible, pComponent->m_eState, pComponent->m_nRenderSection,
			pComponent->m_bUserActive,
			szTextureName,
			szFontName,
			pComponent->m_fFading
			);

		UITextBoxAddLine(textbox, wstr);
		
		UISetNeedToRender(pComponent);
	}
#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEventsDebugIter(
	DbgFileLineNode* node,
	DWORD_PTR data1,
	DWORD_PTR data2)
{
	if (node->count <= 0)
	{
		return TRUE;
	}
	UI_TEXTBOX* textbox = (UI_TEXTBOX*)data1;

	WCHAR str[512];
	char szFile[128];
	int len = PStrLen(node->file);
	PStrCopy(szFile, node->file + (len < 30 ? 0 : len - 29), 128);
	WCHAR wszFile[128];
	PStrCvt(wszFile, szFile, 128);
	PStrPrintf(str, 512, L"  %-30s  %5d  count: %d", wszFile, node->line, node->count);
	UITextBoxAddLine(textbox, str);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUIUpdateEventsDebug(
	GAME* game,
	UI_TEXTBOX* textbox)
{
#if GAME_EVENTS_DEBUG		
	WCHAR name[256];
	WCHAR str[512];

	if (game)
	{
		PStrCopy(name, L"---", arrsize(name));
		UNIT* unit = NULL;
		if (game->m_idUnitWithAbsurdNumberOfEventHandlers != INVALID_ID)
		{
			unit = UnitGetById(game, game->m_idUnitWithAbsurdNumberOfEventHandlers);
			if (unit)
			{
				UnitGetName(unit, name, arrsize(name), 0);
			}
		}
		EVENT_SYSTEM_DEBUG debug_data;
		EventSystemGetDebugData(game, &debug_data);

		PStrPrintf(str, arrsize(str), L"%s:  UnitEventHandlers[%s: %d events]  Timer Events:%d", (IS_SERVER(game) ? L"SRV" : L"CLT"), name, unit ? unit->m_nEventHandlerCount : 0, debug_data.num_events);
		UITextBoxAddLine(textbox, str);

		debug_data.fltbl->Iterate(sUIUpdateEventsDebugIter, (DWORD)CAST_PTR_TO_INT(textbox), 0);
	}
	UITextBoxAddLine(textbox, L"");
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEventsDebugSrv(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	sUIUpdateEventsDebug(AppGetSrvGame(), textbox);
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEventsDebugClt(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	sUIUpdateEventsDebug(AppGetCltGame(), textbox);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSkill(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	if(!game)
		return FALSE;

	UNIT * pUnit = GameGetDebugUnit(game);
	if(!pUnit)
		return FALSE;

	int nCount;

	if(IS_SERVER(game))
	{
		UITextBoxAddLine(textbox, L"Server:");
	}
	else
	{
		UITextBoxAddLine(textbox, L"Client:");
	}
	UITextBoxAddLine(textbox, L"Requested Skills:");
	nCount = SkillsGetSkillInfoArrayLength(pUnit, SIA_REQUEST);
	for(int i=0; i<nCount; i++)
	{
		WCHAR str[512];
		SkillsGetSkillInfoItemText(pUnit, SIA_REQUEST, i, 512, str);
		UITextBoxAddLine(textbox, str);
	}
	
	UITextBoxAddLine(textbox, L"");
	UITextBoxAddLine(textbox, L"Skills On:");
	nCount = SkillsGetSkillInfoArrayLength(pUnit, SIA_IS_ON);
	for(int i=0; i<nCount; i++)
	{
		WCHAR str[512];
		SkillsGetSkillInfoItemText(pUnit, SIA_IS_ON, i, 512, str);
		UITextBoxAddLine(textbox, str);
	}

	UITextBoxAddLine(textbox, L"");
	UITextBoxAddLine(textbox, L"Skill Targets:");
	nCount = SkillsGetSkillInfoArrayLength(pUnit, SIA_TARGETS);
	for(int i=0; i<nCount; i++)
	{
		WCHAR str[512];
		SkillsGetSkillInfoItemText(pUnit, SIA_TARGETS, i, 512, str);
		UITextBoxAddLine(textbox, str);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSkillSrv(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	return sUIUpdateSkill(AppGetSrvGame(), textbox, bVisible);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateSkillClt(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	return sUIUpdateSkill(AppGetCltGame(), textbox, bVisible);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateEventsDebugUI(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

#ifdef _DEBUG
	WCHAR str[512];

	EVENT_SYSTEM_DEBUG debug_data;
	CSchedulerGetDebugData(game, &debug_data);

	PStrPrintf(str, 512, L"UI Scheduler:  Events:%d", debug_data.num_events);
	UITextBoxAddLine(textbox, str);

	debug_data.fltbl->Iterate(sUIUpdateEventsDebugIter, (DWORD)CAST_PTR_TO_INT(textbox), 0);
	UITextBoxAddLine(textbox, L"");
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PerfZeroAll(
	void)
{
	for (int ii = 0; ii < PERF_LAST; ii++)
	{
		gPerfTable[ii].display = gPerfTable[ii].total;
		gPerfTable[ii].total = 0;
		gPerfTable[ii].reset = TRUE;
		gPerfTable[ii].hitdisplay = gPerfTable[ii].hits;
		gPerfTable[ii].hits = 0;
	}
	gqwPerfFramesDisp = gqwPerfFrames;
	gqwPerfFrames = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdatePerformance(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	static UINT64 div = 0;
	if (!div)
	{
		div = PGetPerformanceFrequency() / 10000;
	}
	static TIME lasttick = TIME_ZERO;
	TIME curtick = AppCommonGetCurTime();
	if (curtick - lasttick > 1000)
	{
		lasttick = lasttick + 1000;
		if (curtick - lasttick > 1000)
		{
			lasttick = curtick;
		}
		PerfZeroAll();
	}
	if (gqwPerfFramesDisp == 0 || div == (__int64)0)
	{
		return FALSE;
	}

	int hide_level = 99;
	for (int ii = 0; ii < PERF_LAST; ii++)
	{
		if (gPerfTable[ii].spaces > hide_level)
		{
			gPerfTable[ii].drawn = FALSE;
			continue;
		}
		else if (gPerfTable[ii].spaces <= hide_level)
		{
			hide_level = 99;
		}
		if (!gPerfTable[ii].expand)
		{
			hide_level = gPerfTable[ii].spaces;
		}
		gPerfTable[ii].drawn = TRUE;

		WCHAR label[32];
		PStrCvt(label, gPerfTable[ii].label, 32);
		int pct;
		if (gPerfTable[0].display == 0)
		{
			pct = 0;
		}
		else 
		{
			pct = (int)(gPerfTable[ii].display * 100/gPerfTable[0].display);
		}
		pct = PIN(pct, 0, 100);

		WCHAR expand = L' ';
		if (ii < (int)PERF_LAST - 1)
		{
			if (gPerfTable[ii].spaces < gPerfTable[ii+1].spaces)
			{
				expand = (gPerfTable[ii].expand ? L'-' : L'+');
			}
		}
		PStrPrintf(str, 512, L"%*s%c%-s: %*s%5I64d   %3d%% %d", 
			gPerfTable[ii].spaces, L"", expand, label, 26-PStrLen(label)-gPerfTable[ii].spaces, 
			L"", gPerfTable[ii].display / div / gqwPerfFramesDisp, pct,
			gPerfTable[ ii ].hitdisplay );
		UITextBoxAddLine(textbox, str);
	}
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void HitCountZeroAll(
	void)
{
	for (int ii = 0; ii < HITCOUNT_LAST; ii++)
	{
		gHitCountTable[ii].display = gHitCountTable[ii].hits;
		gHitCountTable[ii].hits = 0;
	}
	gdwHitCountFramesDisp = gdwHitCountFrames;
	gdwHitCountFrames = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateHitCount(
	GAME* game,
	UI_TEXTBOX* textbox,
	BOOL bVisible)
{
	if (!bVisible)
		return FALSE;

	WCHAR str[512];

	static TIME lasttick = TIME_ZERO;
	TIME curtick = AppCommonGetCurTime();
	if (curtick - lasttick > 1000)
	{
		lasttick = lasttick + 1000;
		if (curtick - lasttick > 1000)
		{
			lasttick = curtick;
		}
		HitCountZeroAll();
	}
	if (gdwHitCountFramesDisp == 0)
	{
		return FALSE;
	}

	int hide_level = 99;
	for (int ii = 0; ii < HITCOUNT_LAST; ii++)
	{
		if (gHitCountTable[ii].spaces > hide_level)
		{
			gHitCountTable[ii].drawn = FALSE;
			continue;
		}
		else if (gHitCountTable[ii].spaces <= hide_level)
		{
			hide_level = 99;
		}
		if (!gHitCountTable[ii].expand)
		{
			hide_level = gHitCountTable[ii].spaces;
		}
		gHitCountTable[ii].drawn = TRUE;

		WCHAR label[32];
		PStrCvt(label, gHitCountTable[ii].label, 32);

		WCHAR expand = L' ';
		if (ii < (int)HITCOUNT_LAST - 1)
		{
			if (gHitCountTable[ii].spaces < gHitCountTable[ii+1].spaces)
			{
				expand = (gHitCountTable[ii].expand ? L'-' : L'+');
			}
		}
		PStrPrintf(str, 512, L"%*s%c%-s: %*s%12d", gHitCountTable[ii].spaces, L"", expand, label, 26-PStrLen(label)-gHitCountTable[ii].spaces, L"", 
			gHitCountTable[ii].display);
		UITextBoxAddLine(textbox, str);
	}
	UITextBoxAddLine(textbox, L"");

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void sUIUpdateUnitStatsGetKeyStr(
	GAME* game,
	WCHAR* str,
	int len,
	int table,
	int value)
{
	if (table >= 0)
	{
		if(value < (int)ExcelGetCount(game, (EXCELTABLE)table))
		{
			const char * key = ExcelGetStringIndexByLine(game, (EXCELTABLE)table, value);
			if (key)
			{
				PStrCvt(str, key, len);
				return;
			}
		}
	}
	PStrPrintf(str, len, L"%d", value);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void sUIUpdateUnitStatsGetValueStr(
	GAME* game,
	WCHAR* str,
	int len,
	const STATS_DATA* stats_data,
	int value)
{
	if (stats_data->m_nValTable >= 0)
	{
		if(value >= 0 && value < (int)ExcelGetCount(game, (EXCELTABLE)stats_data->m_nValTable))
		{
			const char* key = ExcelGetStringIndexByLine(game, (EXCELTABLE)stats_data->m_nValTable, value);
			if (key)
			{
				PStrCvt(str, key, len);
				return;
			}
		}
	}
	if (StatsDataTestFlag(stats_data, STATS_DATA_FLOAT))
	{
		PStrPrintf(str, len, L"%1.3f", float(*((float*)&value)));
		return;
	}
	if (stats_data->m_nShift > 0)
	{
		PStrPrintf(str, len, L"%d.%d", value >> stats_data->m_nShift, value & ((1 << stats_data->m_nShift) - 1));
		return;
	}
	PStrPrintf(str, len, L"%d", value);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sUIStatsDebugAddLine(
	UI_TEXTBOX* pTextBox,
	const WCHAR* str,
	DWORD color = GFXCOLOR_WHITE,
	QWORD nData = 0 )
{
	UITextBoxAddLine(pTextBox, str, color, nData);
	GAME *pGame = AppGetCltGame();
	if (GameGetDebugFlag(pGame, DEBUGFLAG_STATS_DUMP))
	{
		char buf[512];
		PStrCvt(buf, str, 512);
		trace(buf);	
		trace("\n");
	}
}
#endif //ISVERSION(DEVELOPMENT)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static void sAddStatsDebug(
	GAME * game,
	STATS_LIST * base,
	STATS_LIST * accr,
	STATS_LIST * calc,
	UI_TEXTBOX * textbox)
{
	WCHAR str[512];

	const STATS_ENTRY * cur_base = base ? base->m_Stats : NULL;
	const STATS_ENTRY * cur_accr = accr ? accr->m_Stats : NULL;
	const STATS_ENTRY * cur_calc = calc ? calc->m_Stats : NULL;
	const STATS_ENTRY * end_base = base ? cur_base + base->m_Count : NULL;
	const STATS_ENTRY * end_accr = accr ? cur_accr + accr->m_Count : NULL;
	const STATS_ENTRY * end_calc = calc ? cur_calc + calc->m_Count : NULL;

	while (cur_base < end_base || cur_accr < end_accr || cur_calc < end_calc)
	{
		WCHAR strStat[256];
		WCHAR strParam[256];
		WCHAR strBase[256];
		WCHAR strAccr[256];
		WCHAR strCalc[256];
		strStat[0] = strParam[0] = strBase[0] = strAccr[0] = strCalc[0] = 0;

		// compute minimum stat out of base, accr, calc
		DWORD dwStat = 0xffffffff;
		const STATS_ENTRY * disp_base = (cur_base < end_base ? cur_base : NULL);
		dwStat = (disp_base ? (disp_base->stat < dwStat ? disp_base->stat : dwStat) : dwStat);
		const STATS_ENTRY * disp_accr = (cur_accr < end_accr ? cur_accr : NULL);
		dwStat = (disp_accr ? (disp_accr->stat < dwStat ? disp_accr->stat : dwStat) : dwStat);
		const STATS_ENTRY * disp_calc = (cur_calc < end_calc ? cur_calc : NULL);
		dwStat = (disp_calc ? (disp_calc->stat < dwStat ? disp_calc->stat : dwStat) : dwStat);
		
		WORD wStat = (WORD)STAT_GET_STAT(dwStat);
		const STATS_DATA * stats_data = StatsGetData(game, wStat);
		ASSERT_BREAK(stats_data);
		PStrCvt(strStat, stats_data->m_szName, 256);
		strStat[26] = 0;
		sUIUpdateUnitStatsGetKeyStr(game, strParam, 256, stats_data->m_nParamTable[0], STAT_GET_PARAM(dwStat));
		strParam[26] = 0;
		if (disp_base && disp_base->stat == dwStat)
		{
			sUIUpdateUnitStatsGetValueStr(game, strBase, 256, stats_data, disp_base->value - stats_data->m_nStatOffset);
			strBase[12] = 0;
			++cur_base;
		}
		if (disp_accr && disp_accr->stat == dwStat)
		{
			sUIUpdateUnitStatsGetValueStr(game, strAccr, 256, stats_data, disp_accr->value - stats_data->m_nStatOffset);
			strAccr[12] = 0;
			++cur_accr;
		}
		if (disp_calc && disp_calc->stat == dwStat)
		{
			sUIUpdateUnitStatsGetValueStr(game, strCalc, 256, stats_data, disp_calc->value - stats_data->m_nStatOffset);
			strCalc[12] = 0;
			++cur_calc;
		}

		if (!TESTBIT(game->pdwDebugStatsTrace, wStat))
		{
			continue;
		}

		PStrPrintf(str, 512, L"%-26s  %-26s  %-12s  %-12s  %-12s", strStat, strParam, strBase, strAccr, strCalc);
		sUIStatsDebugAddLine(textbox, str);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static const WCHAR * sUIUpdateUnitStatsGetSelectorString(
	int selector)
{
	struct SELECTOR_STRING
	{
		int				selector;
		const WCHAR *	string;
	};
	static const SELECTOR_STRING selectorStrings[] =
	{
		{ SELECTOR_NONE,			L"none" },
		{ SELECTOR_DAMAGE_RIDER,	L"damage" },
		{ SELECTOR_PROC_RIDER,		L"proc" },
		{ SELECTOR_UNIT,			L"unit" },
		{ SELECTOR_BUFF,			L"buff" },
		{ SELECTOR_CURSE,			L"curse" },
		{ SELECTOR_AFFIX,			L"affix" },
		{ SELECTOR_SKILL,			L"skill" },
		{ SELECTOR_STATE,			L"state" },
		{ SELECTOR_MISSILETAG,		L"misstag" },
		{ SELECTOR_SKILL_COOLDOWN,	L"cooldown" },
	};
	static const WCHAR * unknown = L"unknown";

	for (unsigned int ii = 0; ii < arrsize(selectorStrings); ++ii)
	{
		if (selector == selectorStrings[ii].selector)
		{
			return (selectorStrings[ii].string ? selectorStrings[ii].string : unknown);
		}
	}
	return unknown;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUIUpdateUnitStats(
	GAME * game,
	UI_TEXTBOX * textbox,
	BOOL bVisible)
{
#if ISVERSION(DEVELOPMENT)
	if (!bVisible)
	{
		return FALSE;
	}

	if (!game)
	{
		return FALSE;
	}

	UNIT * unit = GameGetDebugUnit(game);
	if (!unit)
	{
		return FALSE;
	}

	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_SERVER))
	{
		GAME * srvGame = AppGetSrvGame();
		if (srvGame)
		{
			UNIT * srvUnit = UnitGetById(srvGame, UnitGetId(unit));
			if (srvUnit)
			{
				unit = srvUnit;
			}
		}
	}

	WCHAR name[256];
	WCHAR str[512];

	PStrCvt(name, UnitGetDevName(unit), 256);
	PStrPrintf(str, 512, L"[%s] ID:%d GUID:%I64x  %s %s%s", IS_SERVER(unit->pGame) ? L"SERVER" : L"CLIENT", (DWORD)UnitGetId(unit), UnitGetGuid(unit), name,
		(GameGetDebugFlag(game, DEBUGFLAG_STATS_MODLISTS) ? L" +m" : L""), (GameGetDebugFlag(game, DEBUGFLAG_STATS_RIDERS) ? L" +r" : L""));
	sUIStatsDebugAddLine(textbox, str, GetFontColor(FONTCOLOR_GREEN));

	UNITSTATS * stats = unit->stats;
	ASSERT_RETFALSE(stats);

	STATS_LIST * base = &(stats->m_List);
	STATS_LIST * accr = &(stats->m_Accr);
	STATS_LIST * calc = &(stats->m_Calc);

	PStrPrintf(str, 512, L"%-26s  %-26s  %-12s  %-12s  %-12s", L"stat", L"param", L"base", L"accr", L"calc");
	sUIStatsDebugAddLine(textbox, str, GetFontColor(FONTCOLOR_GREEN));

	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_BASEUNIT))
	{
		sAddStatsDebug(game, base, accr, calc, textbox);
		sUIStatsDebugAddLine(textbox, L" \n");
	}

	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_MODLISTS))
	{
		STATS * modlist = StatsGetModList(unit);
		int modlistCount = 0;
		while (modlist)
		{
			WCHAR strTimer[256];
			strTimer[0] = 0;

			int selector = StatsGetSelector(modlist);
			int timer = StatsGetTimer(unit->pGame, modlist);
			if (timer)
			{
				PStrPrintf(strTimer, arrsize(strTimer), L"  [Time Left: %d %s]", (timer > GAME_TICKS_PER_SECOND * 2 ? ( ( timer + GAME_TICKS_PER_SECOND - 1 ) / GAME_TICKS_PER_SECOND ) : timer), 
					(timer > GAME_TICKS_PER_SECOND * 2 ? L"seconds" : L"ticks"));
			}
			PStrPrintf(str, 512, L"Modlist %d  [Selector: %s]%s", ++modlistCount, sUIUpdateUnitStatsGetSelectorString(selector), strTimer);
			sUIStatsDebugAddLine(textbox, str, GetFontColor(FONTCOLOR_BLUE));

			base = &(modlist->m_List);
			sAddStatsDebug(game, base, NULL, NULL, textbox);
			sUIStatsDebugAddLine(textbox, L" \n");

			modlist = StatsGetModNext(modlist);
		}
	}

	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_RIDERS))
	{
		STATS * rider = NULL;
		int riderCount = 0;
		while ((rider = UnitGetRider(unit, rider)) != NULL)
		{
			PStrPrintf(str, 512, L"Rider %d", ++riderCount);
			sUIStatsDebugAddLine(textbox, str, GetFontColor(FONTCOLOR_LIGHT_RED));

			base = &(rider->m_List);
			sAddStatsDebug(game, base, NULL, NULL, textbox);
			sUIStatsDebugAddLine(textbox, L" \n");
		}
	}

	sUIStatsDebugAddLine(textbox, L"");

	if (GameGetDebugFlag(game, DEBUGFLAG_STATS_DUMP))
	{
		GameSetDebugFlag(game, DEBUGFLAG_STATS_DUMP, FALSE);
	}
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef BOOL (*PFN_UIDEBUG_DRAW)(GAME* game, struct UI_TEXTBOX* textbox, BOOL bVisible);

PFN_UIDEBUG_DRAW gUIDebugFuncTbl[] =
{
	sUIUpdatePing,				// UIDEBUG_PING
	sUIUpdateFPS,				// UIDEBUG_FPS
	sUIUpdateBatches,			// UIDEBUG_BATCHES
	sUIUpdateEngineStatesStats,	// UIDEBUG_STATES
	sUIUpdateParticleStats,		// UIDEBUG_PARTICLESTATS
	sUIUpdateEngineStats,		// UIDEBUG_ENGINESTATS
	sUIUpdateHashMetrics,		// UIDEBUG_HASHMETRICS
	sUIUpdateSynch,				// UIDEBUG_SYNCH
	sUIUpdateMemory,			// UIDEBUG_MEMORY
	sUIUpdateMemoryPoolInfo,	// UIDEBUG_MEMORY_POOL_INFO
	sUIUpdateRoomDebug,			// UIDEBUG_ROOMDEBUG
	sUIUpdateModelDebug,		// UIDEBUG_MODELDEBUG
	sUIUpdateSoundDebug,		// UIDEBUG_SOUND
	sUIUpdateEventsDebugSrv,	// UIDEBUG_EVENTS_SRV
	sUIUpdateEventsDebugClt,	// UIDEBUG_EVENTS_CLT
	sUIUpdateEventsDebugUI,		// UIDEBUG_EVENTS_UI
	sUIUpdatePerformance,		// UIDEBUG_PERFORMANCE
	sUIUpdateHitCount,			// UIDEBUG_HITCOUNT
	sUIUpdateUnitStats,			// UIDEBUG_STATS
	sUIUpdateVirtualSoundDebug,	// UIDEBUG_VIRTUALSOUND
	sUIUpdateBars,				// UIDEBUG_BARS
	sUIUpdateMusicDebug,		// UIDEBUG_MUSIC
	sUIUpdateParticleDebug,		// UIDEBUG_PARTICLES
	sUIUpdateBGSoundDebug,		// UIDEBUG_BGSOUND
	sUIUpdateUIDebug,			// UIDEBUG_UI
	sUIUpdateUIGfxDebug,		// UIDEBUG_UIGFX
	sUIUpdateAppearance,		// UIDEBUG_APPEARANCE
	sUIUpdateDPS,				// UIDEBUG_DPS
	sUIUpdateSkillSrv,			// UIDEBUG_SKILL_SRV
	sUIUpdateSkillClt,			// UIDEBUG_SKILL_CLT
	sUIUpdateSoundMemory,		// UIDEBUG_SOUNDMEMORY
	sUIUpdateDebugUIEdit,		// UIDEBUG_UIEDIT
	sUIUpdateStoppedSoundDebug,	// UIDEBUG_STOPPEDSOUND
	sUIUpdateLayoutRoomDisplay,	// UIDEBUG_LAYOUTROOMDISPLAY
	sUIUpdateDPVSStats,			// UIDEBUG_DPVS
	sUIUpdateMusicScriptDebug,	// UIDEBUG_MUSICSCRIPT
	sUIUpdateReverbDebug,		// UIDEBUG_REVERB
	sUIUpdateSoundBuses,		// UIDEBUG_SOUNDBUSES
	sUIUpdateSoundMixStates,	// UIDEBUG_SOUNDMIXSTATES
	sUIUpdateTagDisplay,		// UIDEBUG_TAGDISPLAY
};
BOOL g_bUIDebugStatus[UIDEBUG_LAST];


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIDebugDisplayGetStatus(
	DWORD eDebugDisplay)
{
	ASSERT_RETFALSE(eDebugDisplay < UIDEBUG_LAST);
	
	return g_bUIDebugStatus[eDebugDisplay];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDebugDisplayUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	PERF(DEBUG_DISPLAY);
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}
	UI_TEXTBOX* textbox = UICastToTextBox(component);
	ASSERT_RETURN(textbox);

	if (!game)
	{
		return;
	}
	//if (!UIComponentGetVisible(textbox))
	//{
	//	return;
	//}
	//if (!textbox->m_dwParam)
	//{
	//	return;
	//}


	if (textbox->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(textbox->m_tMainAnimInfo.m_dwAnimTicket);
	}

	UITextBoxClear(textbox);

	BOOL bUpdate = FALSE;
	BOOL bOneVisible = FALSE;
	for (int ii = 0; ii < UIDEBUG_LAST; ii++)
	{
		if (g_bUIDebugStatus[ii])
		{
			bOneVisible = TRUE;
		}

		ASSERT_CONTINUE(ii < arrsize(gUIDebugFuncTbl));
		if (!gUIDebugFuncTbl[ii])
		{
			continue;
		}
		bUpdate |= gUIDebugFuncTbl[ii](game, textbox, g_bUIDebugStatus[ii]);
	}

	if (bUpdate)
	{
		UIComponentHandleUIMessage(textbox, UIMSG_PAINT, 0, 0);
	}

	if (bOneVisible)
	{
		textbox->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIDebugDisplayUpdate, CEVENT_DATA((DWORD_PTR)component));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleDebugDisplay(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(wParam >= 0 && wParam < UIDEBUG_LAST, UIMSG_RET_NOT_HANDLED);

	if ((int)lParam < 0)
	{
		lParam = !g_bUIDebugStatus[wParam];
	}
	g_bUIDebugStatus[wParam] = (lParam != 0);

//	if (component->m_dwParam && !UIComponentGetVisible(component))
	{
		CSchedulerRegisterEventImm(UIDebugDisplayUpdate, CEVENT_DATA((DWORD_PTR)component));
	}

	if (lParam != 0)
		UIComponentSetVisible(component, TRUE);
	return UIMSG_RET_HANDLED;
}


#endif
