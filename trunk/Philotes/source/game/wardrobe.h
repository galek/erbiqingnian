//----------------------------------------------------------------------------
// wardrobe.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _WARDROBE_H_
#define _WARDROBE_H_


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _DATATABLES_H_
#include "..\\datatables.h"
#endif


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
enum 
{
	WARDROBE_BODY_COLOR_SKIN,
	WARDROBE_BODY_COLOR_HAIR,
	WARDROBE_BODY_COLOR_UNUSED,

	NUM_WARDROBE_BODY_COLORS,
	NUM_WARDROBE_BODY_COLORS_USED = 2,
};

struct WARDROBE;
struct WARDROBE_BODY;
struct WARDROBE_CLIENT;
struct GAME;
struct UNIT;
struct GAMECLIENT;
struct UNIT_FILE_HEADER;


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void c_WardrobeTest ( 
	BOOL bTestLowLOD );

BOOL c_WardrobeIsUpdated ( 
	int nWardrobe );

struct WARDROBE_INIT;
int WardrobeInit ( 
	GAME * pGame,
	const WARDROBE_INIT &Init );

void WardrobeInitInitializeBody(
	struct WARDROBE_BODY & tBody,
	int nWardrobeAppearanceGroup,
	BOOL bForce );

void WardrobeHashFree( GAME* pGame );

int c_WardrobeGetCount( 					   
	int* pnTemplateCount = NULL, 
	int* pnInstancedCount = NULL,
	int* pnTotalCount = NULL );

void WardrobeRandomizeInitStruct(
	GAME * pGame,
	WARDROBE_INIT & tWardrobeInit,
	int nWardrobeAppearanceGroup,
	BOOL bForce );

void WardrobeFree ( 
	GAME * pGame,
	int nWardrobe );

int WardrobeInitStructWrite(
	WARDROBE_INIT & tWardrobeInit,
	BYTE * pbBuffer,
	int nMaxSize);

BOOL WardrobeInitStructRead(
	WARDROBE_INIT & tWardrobeInit,
	BYTE * pbBuffer,
	int nMaxSize,
	unsigned int nClass = INVALID_ID);

void s_WardrobeInitCreateFromUnit(
	UNIT *pUnit,
	WARDROBE_INIT &tWardrobeInit);
	
template <XFER_MODE mode>
BOOL WardrobeXfer(
	UNIT *pUnit,
	int nWardrobe,
	XFER<mode> & tXfer,
	UNIT_FILE_HEADER &tHeader,
	BOOL bIsWardrobeUpdate,
	BOOL *pbWeaponsChanged);

void c_WardrobeResetWeaponAttachments ( 
	int nWardrobe );

void WardrobeApplyBodyDataToInit(
	GAME * pGame,
	WARDROBE_INIT & tWardrobeInit,
	int nBodyData,
	int nWardrobeAppearanceGroup );

void c_WardrobeUpdateLayersFromMsg (
	UNIT * pUnit,
	struct MSG_SCMD_UNITWARDROBE * pMsg );

void s_WardrobeUpdateClient (
	GAME * pGame,
	UNIT * pUnit,
	GAMECLIENT * pClient );
void c_WardrobeMakeClientOnly (
	UNIT * pUnit );

PRESULT c_WardrobeSetMipBonusEnabled ( 
	int nWardrobe,
	BOOL bEnabled );

PRESULT c_WardrobeUpgradeCostumeCount( 
	int nWardrobe,
	BOOL bKeepCostumeUpgraded = FALSE );

PRESULT c_WardrobeDowngradeCostumeCount( 
	int nWardrobe );

PRESULT c_WardrobeUpgradeLOD( 
	int nWardrobe,
	BOOL bForce = FALSE );

PRESULT c_WardrobeDowngradeLOD( 
	int nWardrobe );

void WardrobeUpdateFromInventory (
	UNIT * pUnit );

BOOL ExcelWardrobePostProcess(
	struct EXCEL_TABLE * table);

BOOL ExcelWardrobeTextureSetPostProcess(
	EXCEL_TABLE * table);

BOOL ExcelWardrobeModelPostProcessRow(
	struct EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata);

void ExcelWardrobeModelFreeRow(
	struct EXCEL_TABLE * table,
	BYTE * rowdata);

void c_WardrobeRandomize ( 
	int nWardrobe );

//int c_WardrobeGetModelDefinition( 
//	int nWardrobeBaseId );

UNIT * WardrobeGetWeapon( 
	GAME * pGame,
	int nWardrobe,
	int nWeaponIndex );

int WardrobeGetWeaponIndex( 
	UNIT * pUnit,
	UNITID idWeapon );

void c_WardrobeSystemInit();
void c_WardrobeSystemDestroy();
void c_WardrobeUpdate();

void c_WardrobeUpdateAttachments (
	UNIT * pUnit,
	int nWardrobe,
	int nModelId,
	BOOL bUseWeaponModel,
	BOOL bJustUpdateWeapon,
	BOOL bRecreateWeaponAttachments = FALSE );

int c_WardrobeGetModelDefinition( 
	int nWardrobe );

void c_WardrobeForceLayerOntoUnit (
	int nWardrobe,
	int nModelThirdPerson,
	int nLayer );

void c_WardrobeForceLayerOntoUnit (
	UNIT * pUnit,
	int nLayer );

void c_WardrobeRemoveLayerFromUnit (
	UNIT * pUnit,
	int nLayer );

void c_WardrobeLoadAll();

BOOL c_WardrobeGetWeaponBones( 
	int nModel,
	int nWardrobe,
	int * pnBones );

void c_WardrobeSetColorSetIndex (
	int nWardrobe,
	int nColorSetIndex,
	int nUnittype,
	BOOL bSetAllCostumes );

void c_WardrobeIncrementBodyColor( 
	int nWardrobe, 
	BOOL bIsHair, 
	BOOL bNext );

BYTE c_WardrobeGetBodyColorIndex( 
	int nWardrobe, 
	UINT nIndex );

void c_WardrobeSetBodyColor( 
	int nWardrobe, 
	UINT nIndex,
	BYTE bColorIndex );

void c_WardrobeSetBodyColors( 
	int nWardrobe, 
	BYTE pbBodyColors[ NUM_WARDROBE_BODY_COLORS ] );

void c_WardrobeUpdateStates (
	UNIT * pUnit,
	int nModelId,
	int nWardrobe );

void c_WardrobeSetModelDefinitionOverride( 
	int nWardrobe,
	int nModelDefId );

void c_WardrobeSetBaseLayer(
	int nWardrobe, 
	int nBaseLayerId );

void WardrobeGetItemColorSetIndexAndPriority(
	GAME* pGame, UNIT* pItem,
	int& nColorSetIndex, int& nColorSetPriority,
	int nDefaultColorSetPriority = -1 );

void s_WardrobeSetColorsetItem( 
	UNIT * unit, 
	UNIT * pItem );

BOOL c_WardrobeGetBodyColor (
	int nBodyColorIndex,
	int nUnittype,
	BYTE bColorIndex,
	DWORD & dwColor );

int WardrobeAllocate(
	GAME* pGame,
	const struct UNIT_DATA* pUnitData,
	WARDROBE_INIT* pWardrobeInit,
	int nWardrobeFallback,
	BOOL& bWardrobeChanged ) ;

BOOL WardrobeExists(
	GAME* pGame,
	int nWardrobe );

void WardrobeSetMask (
	GAME* pGame,
	int nWardrobe,
	DWORD mask );

void c_WardrobeSetPrimaryColorReplacesSkin (
	GAME* pGame,
	int nWardrobe,
	BOOL bSet );

int WardrobeModelFileGetLine(
	const char * pszFilePath );

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
inline unsigned int WardrobeGetNumLayers(
	GAME * game)
{
	return ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_WARDROBE_LAYER);
}


#endif // _WARDROBE_H_
