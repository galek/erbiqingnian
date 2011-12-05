//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef __CURRENCY_H_
#define __CURRENCY_H_


#ifndef _STATS_H_
#include "stats.h"
#endif
//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
enum STATS_TYPE;


//ENUM listing the different types of values per currency type
const static enum ECURRENCY_VALUES
{
	KCURRENCY_VALUE_INGAME,			//returns STATS_GOLD
	KCURRENCY_VALUE_INGAME_RANK1,	//returns copper
	KCURRENCY_VALUE_INGAME_RANK2,	//returns silver
	KCURRENCY_VALUE_INGAME_RANK3,	//returns gold
	KCURRENCY_VALUE_REALWORLD,		//returns STATS_MONEY_REALWORLD
	KCURRENCY_VALUE_REALWORLD_RANK1,//return cents
	KCURRENCY_VALUE_REALWORLD_RANK2,//return dollars
	KCURRENCY_VALUE_PVPPOINTS,		//returns PVP points
	KCURRENCY_VALUE_COUNT
};

//CURRENCY TYPES
const static enum ECURRENCY_TYPES
{
	KCURRENCY_TYPE_INGAME,
	KCURRENCY_TYPE_REALWORLD,
	KCURRENCY_TYPE_PVPPOINTS,
	KCURRENCY_TYPE_COUNT
};

const static ECURRENCY_VALUES sCurrencyWholeValues[KCURRENCY_TYPE_COUNT] = { KCURRENCY_VALUE_INGAME,
																			  KCURRENCY_VALUE_REALWORLD,
																			  KCURRENCY_VALUE_PVPPOINTS };


const static int sCurrencyStats[KCURRENCY_TYPE_COUNT] = { STATS_GOLD,
														  STATS_MONEY_REALWORLD,
														  STATS_PVP_POINTS };

class cCurrency
{
private:	



	void AddCurrency( cCurrency &currency );	
	void RemoveCurrency( cCurrency &currency );
	inline void FillInCurrencyTypeValues( UNIT *pUnit );	

	int	m_CurrencyTypeValues[ KCURRENCY_TYPE_COUNT ];

public:
	cCurrency( void );
	cCurrency( UNIT *pUnit );	
	cCurrency( int valueInGame, int valueInWorld = 0, int valuePVPPoints = 0);
	~cCurrency();	
	int GetValue( ECURRENCY_VALUES value );		
	BOOL IsZero();
	BOOL IsNegative();
	BOOL CanCarry(  GAME *pGame, cCurrency currency );
	int HighestValue();
	int LowestValue();
	void operator/= ( int nValue );
	void operator*= ( int nValue );
	void operator/= ( float nValue );
	void operator*= ( float nValue );
	void operator+= ( cCurrency currency );
	void operator-= ( cCurrency currency );
	BOOL operator== ( cCurrency currency );
	BOOL operator!= ( cCurrency currency );
	BOOL operator>= ( cCurrency currency );
	BOOL operator<= ( cCurrency currency );
	BOOL operator> ( cCurrency currency );
	BOOL operator< ( cCurrency currency );
	cCurrency operator- (cCurrency currency );
	cCurrency operator+ ( cCurrency currency );



};

#endif