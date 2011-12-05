//----------------------------------------------------------------------------
// e_budget.h
//
// - Header for resource budget functions/structures
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_BUDGET__
#define __E_BUDGET__

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define TEXTURE_BUDGET_EST_GEOM_TEX_RATIO	( 1.f / 7.f )

#define UI_TEXTURE_MAX_MIP_DOWN				1
#define TEXTURE_MAX_MIP_DOWN				2

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------


enum REAPER_ALERT_LEVEL
{
	REAPER_ALERT_LEVEL_NONE,
	REAPER_ALERT_LEVEL_LOW,
	REAPER_ALERT_LEVEL_MEDIUM,
	REAPER_ALERT_LEVEL_HIGH,
	NUM_REAPER_ALERT_LEVELS
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct TEXTURE_BUDGET_MIP_GROUP
{
	TEXTURE_GROUP	eGroup;
	//float			fRate[ NUM_TEXTURE_TYPES ];
	float			fMIP[ NUM_TEXTURE_TYPES ];
};

struct TEXTURE_BUDGET
{
	int		nMIPDown        [ NUM_TEXTURE_GROUPS ][ NUM_TEXTURE_TYPES ];		// MIP down for each group/type
	int		nMIPDownUnreaped[ NUM_TEXTURE_GROUPS ][ NUM_TEXTURE_TYPES ];		// same, without input from Reaper

	// cached values
	float	fMIPFactorNormalizer;										// Multiplier against MIP factor to get final coef
	float	fMIPFactor;													// 0..1 factor to set MIP coef
	float	fQualityBias;												// Bias multiplier to adjust user-set texture quality
};

struct MODEL_BUDGET_GROUP
{
	MODEL_GROUP		eGroup;
	float			fLODRate;
};

struct MODEL_BUDGET
{
	int		nMaxLOD[ NUM_MODEL_GROUPS ];
	float	fQualityBias;												// Bias multiplier to adjust user-set model quality
};

struct ENGINE_RESOURCE : public LRUNode
{
	enum
	{
		FLAGBIT_UPLOADED = 0,											// has this resource been set on the device
		FLAGBIT_MANAGED,												// is this resource's upload managed by D3D
	};
	int nId;
	float fQuality;
	float fDistance;
	DWORD dwFlagBits;
};

struct RESOURCE_UPLOAD_MANAGER
{
	enum UPLOAD_TYPE
	{
		TEXTURE = 0,
		VBUFFER,
		IBUFFER,
		// count
		NUM_TYPES
	};

	struct Metrics
	{
		RunningAverage<DWORD,60>	dwFrameBytes;
		DWORD dwTotalBytes;
	};
	Metrics tMetrics[ NUM_TYPES ];

	DWORD dwCurFrameBytesPerType[ NUM_TYPES ];
	DWORD dwCurFrameBytesTotal;

	DWORD dwUploadFrameMaxBytes;

	void SetLimit( DWORD dwBytes );
	BOOL CanUpload();
	void MarkUpload( DWORD dwBytes, UPLOAD_TYPE eType );
	void ZeroCur();
	void NewFrame();
	void Init();

	DWORD GetLimit()					{ return dwUploadFrameMaxBytes; }
	DWORD GetTotal();
	DWORD GetAvgRecentForType( UPLOAD_TYPE eType );
	DWORD GetAvgRecentAll();
	DWORD GetMaxRecentForType( UPLOAD_TYPE eType );
	DWORD GetMaxRecentAll();
};

struct RESOURCE_REAPER
{
	LRUList tModels;
	LRUList tTextures;
	REAPER_ALERT_LEVEL eAlertLevel;
	DWORD dwAlertLevelSetTime;
	unsigned int nAlertLevelUpdates;
	BOOL bAlertLevelOverride;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern TEXTURE_BUDGET gtTextureBudget;
extern MODEL_BUDGET gtModelBudget;
extern RESOURCE_REAPER gtReaper;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline RESOURCE_UPLOAD_MANAGER & e_UploadManagerGet()
{
	extern RESOURCE_UPLOAD_MANAGER gtUploadManager;
	return gtUploadManager;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_TextureBudgetInit();
int e_TextureBudgetGetMIPDown( TEXTURE_GROUP eGroup, TEXTURE_TYPE eType, BOOL bIgnoreReaper = FALSE );
int e_TextureBudgetAdjustWidthAndHeight( TEXTURE_GROUP eGroup, TEXTURE_TYPE eType, int & nWidth, int & nHeight, BOOL bIgnoreReaper = FALSE );
void e_TextureBudgetDebugOutputMIPLevels( BOOL bConsole, BOOL bTrace, BOOL bLog );
PRESULT e_TextureBudgetUpdate();

PRESULT e_ModelBudgetInit();
PRESULT e_ModelBudgetUpdate();
int e_ModelBudgetGetMaxLOD( MODEL_GROUP eGroup );

PRESULT e_ReaperInit();
PRESULT e_ReaperReset();
PRESULT e_ReaperUpdate();
void e_ReaperSetAlertLevel( REAPER_ALERT_LEVEL eAlertLevel, BOOL bOverride = FALSE );
REAPER_ALERT_LEVEL e_ReaperGetAlertLevel();

PRESULT e_MarkEngineResourcesUnloaded();

#endif // __E_BUDGET__