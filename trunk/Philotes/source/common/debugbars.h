//----------------------------------------------------------------------------
// debugbars.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#ifndef __DEBUGBARS_H__
#define __DEBUGBARS_H__

#define UI_BAR_SET_NAME_MAX_LEN		32

#define SET_BAR(nBar,szName,nValue,nMax,nGood,nBad) CommonSetUIBar( nBar, szName, nValue, nMax, nGood, nBad )

typedef void (*PFN_UPDATE_UI_BAR_SET)();
typedef void (*PFN_SET_UI_BAR)( int nBarIndex, const char * szName, int nValue, int nMax, int nGood, int nBad );

inline void CommonSetUIBar(
	int nBarIndex,
	const char * szName,
	int nValue,
	int nMax,
	int nGood,
	int nBad )
{
	extern PFN_SET_UI_BAR gpfn_SetUIBar;
	ASSERT_RETURN( gpfn_SetUIBar );
	gpfn_SetUIBar( nBarIndex, szName, nValue, nMax, nGood, nBad );
}

inline void c_InitDebugBars( PFN_SET_UI_BAR pfnCallback )
{
	extern PFN_SET_UI_BAR gpfn_SetUIBar;
	gpfn_SetUIBar = pfnCallback;
}
#endif // __DEBUGBARS_H__
