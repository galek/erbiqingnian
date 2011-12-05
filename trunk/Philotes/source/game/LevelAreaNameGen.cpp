#include "stdafx.h"
#include "LevelAreaNameGen.h"
#include "LevelAreas.h"
#include "level.h"
#include "stringtable.h"

using namespace MYTHOS_LEVELAREAS;

inline BOOL FillInMadlib( const LEVEL_DEFINITION *pLevelDefPortal,
						  int nLevelAreaID,
						  const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea,
						  WCHAR *puszBuffer,
						  int nBufferSize )
{
	if( pLevelDefPortal &&
		pLevelArea &&
		MYTHOS_LEVELAREAS::LevelAreaGetNumOfGeneratedMapsAllowed( nLevelAreaID ) <= 1)
	{
		return FALSE;
	}
	RAND rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &rand, nLevelAreaID );
	int m_MadlibCounts[MYTHOS_MADLIB_COUNT];
	for( int t = 0; t < MYTHOS_MADLIB_COUNT; t++ )
	{
		m_MadlibCounts[t] = ExcelGetCount( NULL, pLevelDefPortal->nLevelMadlibNames[ t ] );
		ASSERT( m_MadlibCounts[t] > 0 );
	}
	//madlibs we can use
	int m_MadlibTableNeed[ 10 ];
	const WCHAR *m_Strings[ 10 ];
	int nMadLibTableNeedCount( 0 );
	for( int t = 0; t < 10; t++ )
	{
		m_MadlibTableNeed[t] = INVALID_ID;
		m_Strings[ t ] = NULL;
	}
	//get the sentence to use
	int nSentenceToUse =  RandGetNum( rand, 0, m_MadlibCounts[MYTHOS_MADLIB_SENTENCES] - 1 );
	const LEVELAREA_MADLIB_ENTRY *madlibEntry = (const LEVELAREA_MADLIB_ENTRY *)ExcelGetData( NULL, (EXCELTABLE)pLevelDefPortal->nLevelMadlibNames[MYTHOS_MADLIB_SENTENCES], nSentenceToUse );	
	const WCHAR * puszMadLibSentence = StringTableGetStringByIndex( madlibEntry->m_StringIndex );
	int length = PStrLen( puszMadLibSentence );
	//fill out the sentence
	int nBufferIndex(0);
	WCHAR puszTempString[ 1024 ];
	for( int t = 0; t < length; t++ )
	{
		if( puszMadLibSentence[t] == L'%' )
		{
			puszTempString[nBufferIndex++] = L'%';
			puszTempString[nBufferIndex++] = L's'; //we add in the string to insert			
			t++;
			switch( puszMadLibSentence[t] )
			{
			case L'A':
			case L'a':
				m_MadlibTableNeed[ nMadLibTableNeedCount++ ] = MYTHOS_MADLIB_AFFIXS;
				break;
			case L'P':
			case L'p':
				m_MadlibTableNeed[ nMadLibTableNeedCount++ ] = MYTHOS_MADLIB_PROPERNAMES;
				break;
			case L'S':
			case L's':
				m_MadlibTableNeed[ nMadLibTableNeedCount++ ] = MYTHOS_MADLIB_SUFFIXS;
				break;	
			case L'J':
			case L'j':
				m_MadlibTableNeed[ nMadLibTableNeedCount++ ] = MYTHOS_MADLIB_ADJECTIVES;
				break;
			case L'N':
			case L'n':
				m_MadlibTableNeed[ nMadLibTableNeedCount++ ] = MYTHOS_MADLIB_NOUNS;
				break;
			}
		}
		else
		{
			puszTempString[nBufferIndex++] = puszMadLibSentence[t];
		}
	}
	puszTempString[nBufferIndex] = 0;//end the name

	
	for( int t = 0; t < nMadLibTableNeedCount; t++ )
	{
		int nTableIndex = pLevelDefPortal->nLevelMadlibNames[m_MadlibTableNeed[t]];
		int nChoice = RandGetNum( rand, 0, m_MadlibCounts[ m_MadlibTableNeed[t] ] - 1 );				
		const LEVELAREA_MADLIB_ENTRY *madlibEntry = (const LEVELAREA_MADLIB_ENTRY *)ExcelGetData( NULL, (EXCELTABLE)nTableIndex, nChoice );
		m_Strings[ t ] = StringTableGetStringByIndex( madlibEntry->m_StringIndex );
		ASSERT_CONTINUE( m_Strings[ t ] );
							
	}
	
	int nCount( 0 );
	int nBufIndex(0);
	for( int t = 0; t < nBufferIndex; t++ )
	{
		if( puszTempString[t] == L'%' &&
			puszTempString[t + 1] == L's' )
		{
			t++;
			int nStringLen = PStrLen( m_Strings[ nCount ] );
			for( int i = 0; i<nStringLen; i++ )
			{
				puszBuffer[ nBufIndex++ ] = m_Strings[ nCount ][i];
			}
			nCount++;
		}
		else
		{
			puszBuffer[ nBufIndex++ ] = puszTempString[t];
		}
	}
	puszBuffer[ nBufIndex ] = 0;
	return TRUE;



	
}
//----------------------------------------------------------------------------
void MYTHOS_LEVELAREAS::GetRandomLibName( const LEVEL *pLevel,
										  int nLevelAreaID,
										  int nLevelAreaDepth,
										   WCHAR *puszBuffer,
										   int nBufferSize,
										   BOOL bForceGeneralNameOfArea )
{

	puszBuffer[0] = 0;
	
	if( nLevelAreaID == INVALID_ID )
	{
		ASSERT_RETURN( pLevel );
		nLevelAreaID = LevelGetLevelAreaID( pLevel );
	}
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
	ASSERTX_RETURN( pLevelArea, "Level area is null" );

	int nLevelNameOverride = MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaName( nLevelAreaID, nLevelAreaDepth, bForceGeneralNameOfArea );
	int nLevelID = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, nLevelAreaID, nLevelAreaDepth ); //last level
	const LEVEL_DEFINITION *pLevelDefPortal = LevelDefinitionGet( nLevelID ); 
	if( nLevelNameOverride != INVALID_ID ||  //if we have an override don't even do the mad lib.
		FillInMadlib( pLevelDefPortal, nLevelAreaID, pLevelArea, puszBuffer, nBufferSize ) == FALSE )
	{
		if( pLevelArea )
		{
			ASSERT_RETURN( nLevelNameOverride != INVALID_ID );			
			const WCHAR * puszAreaName = StringTableGetStringByIndex( nLevelNameOverride );
			PStrCvt( puszBuffer, puszAreaName, nBufferSize );			
		}

	}

}