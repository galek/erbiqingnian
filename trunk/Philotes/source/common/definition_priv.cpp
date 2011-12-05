//----------------------------------------------------------------------------
// definition.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "prime.h"
#include "definition_common.h"
#include "definition_priv.h"
#include "excel.h"
#include "markup.h"
#include "interpolationpath.h"
#include "event_data.h"
#include "inorderhash.h"
#include "filepaths.h"
#include "pakfile.h"
#include "config.h"
#include "performance.h"
#include "excel_private.h"


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define VARRAY_SUFFIX				"Count"
#define PATH_TIME_STRING			"PathTime"
#define PATH_VALUE_STRING			"PathValue"
#define PATH_COUNT_STRING			"PathCount"

#define DEFINITION_COOKED_EXTENSION "cooked"
#define DEFINITION_FILE_VERSION		8

#define COOKED_MAGIC_NUMBER			'k0OC'
#define COOKED_DATA_MARKER			'ATAD'

#if ISVERSION(DEVELOPMENT)
//#define DEFINITION_TRACE_DEBUG	1
//#define DEFINITION_LOG_DEBUG		1
#endif

static BOOL sgbForceSync = FALSE;		// force synchronous load

// CHB 2006.09.29
#ifdef _WIN64
typedef QWORD POINTER_AS_INT;
#else
typedef DWORD POINTER_AS_INT;
#endif


//----------------------------------------------------------------------------
// DEBUG DEFINES FOR TRACING/LOGGING
//----------------------------------------------------------------------------

#ifdef DEFINITION_TRACE_DEBUG
	#define DEF_TRACE(...) {trace(__VA_ARGS__); trace("\n");}
#else
	#define DEF_TRACE(...)
#endif

#ifdef DEFINITION_LOG_DEBUG
	#define DEF_LOG(...) {LogMessage(DEFINITION_LOG, __VA_ARGS__);}
#else
	#define DEF_LOG(...)
#endif

#define DEF_PRINT(...) {DEF_TRACE(__VA_ARGS__) DEF_LOG(__VA_ARGS__)}


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

// caches of the definition IDs for commonly-accessed one-off definitions
int gnGlobalDefinitionID = INVALID_ID;
int gnConfigDefinitionID = INVALID_ID;
int gnServerlistDefinitionID = INVALID_ID;
int gnServerlistOverrideDefinitionID = INVALID_ID;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DefinitionForceNoAsync(
	void)
{
	sgbForceSync = TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sElementGetSize(
	const CDefinitionLoader::CElement * element)
{
	if (element->m_pDefinitionLoader)
	{
		return element->m_pDefinitionLoader->m_nStructSize;
	}
	else 
	{
		switch (element->m_eDataType)
		{
		case DATA_TYPE_BYTE:
			return sizeof( BYTE );
		case DATA_TYPE_INT:		
		case DATA_TYPE_FLOAT:	
		case DATA_TYPE_INT_NOSAVE:
		case DATA_TYPE_INT_DEFAULT:
		case DATA_TYPE_FLOAT_NOSAVE:
		case DATA_TYPE_INT_LINK:
		case DATA_TYPE_INT_INDEX_LINK:
		case DATA_TYPE_FLAG:
			return sizeof( int );
		case DATA_TYPE_POINTER_NOSAVE:
			return sizeof(POINTER_AS_INT);
		case DATA_TYPE_FLAG_BIT:
			return sizeof(DWORD) * DWORD_FLAG_SIZE(element->m_nParam2);
		case DATA_TYPE_STRING:	
			return (sizeof(char) * MAX_XML_STRING_LENGTH);
		// CHB 2006.09.29
		case DATA_TYPE_PATH_FLOAT:
			return sizeof(CInterpolationPath<float>);
		case DATA_TYPE_PATH_FLOAT_PAIR:
			return sizeof(CInterpolationPath<CFloatPair>);
		case DATA_TYPE_PATH_FLOAT_TRIPLE:
			return sizeof(CInterpolationPath<CFloatTriple>);
		case DATA_TYPE_INT_64:
			return sizeof( unsigned __int64 );
		default:
			ASSERT(0);
			return 0;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::FixDefaults(void)
{
	for (int ii = 0; m_pElements[ii].m_pszVariableName && m_pElements[ii].m_pszVariableName[0] != 0 ; ii++)
	{
		CDefinitionLoader::CElement * element = &m_pElements[ii];
		FixDefault(element);
	}
}

void CDefinitionLoader::FixDefault(CDefinitionLoader::CElement * element)
{
	switch (element->m_eDataType)
	{
		case DATA_TYPE_BYTE:
			if (element->m_pszDefaultValue && element->m_pszDefaultValue[0])
			{
				*(BYTE*)&element->m_DefaultValue = (BYTE)PStrToInt(element->m_pszDefaultValue);
//				element->m_pszDefaultValue = NULL;
			}
			break;
		case DATA_TYPE_INT:
		case DATA_TYPE_INT_NOSAVE:
		case DATA_TYPE_INT_DEFAULT:
		case DATA_TYPE_POINTER_NOSAVE:
		case DATA_TYPE_FLAG:
		case DATA_TYPE_FLAG_BIT:
			if (element->m_pszDefaultValue && element->m_pszDefaultValue[0])
			{
				*(int *)&element->m_DefaultValue = PStrToInt(element->m_pszDefaultValue);
//				element->m_pszDefaultValue = NULL;
			}
			break;
		case DATA_TYPE_INT_64:
			if (element->m_pszDefaultValue && element->m_pszDefaultValue[0])
			{
				unsigned __int64 i64;
				PStrToInt(element->m_pszDefaultValue, i64);
				*(unsigned __int64 *)&element->m_DefaultValue = i64;
//				element->m_pszDefaultValue = NULL;
			}
			break;
		case DATA_TYPE_FLOAT:
		case DATA_TYPE_FLOAT_NOSAVE:
		case DATA_TYPE_PATH_FLOAT:
		case DATA_TYPE_PATH_FLOAT_PAIR:
		case DATA_TYPE_PATH_FLOAT_TRIPLE:
			if (element->m_pszDefaultValue && element->m_pszDefaultValue[0])
			{
				*(float *)&element->m_DefaultValue = (float)PStrToFloat(element->m_pszDefaultValue);
//				element->m_pszDefaultValue = NULL;
			}
			break;
		case DATA_TYPE_INT_LINK:
		case DATA_TYPE_INT_INDEX_LINK:
			{
				*(int *)&element->m_DefaultValue = -1;
//				element->m_pszDefaultValue = NULL;
			}
			break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::ComputeVersion(
	void)
{
	if (m_dwCRC != 0)
	{
		return;
	}

	m_dwCRC = GLOBAL_FILE_VERSION + DEFINITION_FILE_VERSION;

	for (int ii = 0; m_pElements[ii].m_pszVariableName && m_pElements[ii].m_pszVariableName[0] != 0 ; ii++)
	{
		CDefinitionLoader::CElement * element = &m_pElements[ii];

		FixDefault(element);

		switch (m_pElements[ii].m_eDataType)
		{
		case DATA_TYPE_INT_NOSAVE:
		case DATA_TYPE_POINTER_NOSAVE:
		case DATA_TYPE_FLOAT_NOSAVE:
			break;
		default:
			if (element->m_pszVariableName)
			{
				m_dwCRC = CRC(m_dwCRC, (BYTE *)element->m_pszVariableName, PStrLen(element->m_pszVariableName));
			}
			m_dwCRC = CRC(m_dwCRC, (BYTE *)&element->m_eDataType, sizeof(int));
			m_dwCRC = CRC(m_dwCRC, (BYTE *)&element->m_nArrayLength, sizeof(int));
			if (element->m_pszDefaultValue)
			{
				m_dwCRC = CRC(m_dwCRC, (BYTE *)element->m_pszDefaultValue, PStrLen(element->m_pszDefaultValue));
			}
			m_dwCRC = CRC(m_dwCRC, (BYTE *)&element->m_nParam, sizeof(int));
			break;
		}
	}
	if (m_dwCRC == 0)
	{
		m_dwCRC = 1;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::Load (
	void * pTarget,
	CMarkup * pMarkup,
	BOOL bZeroOutData,
	BOOL bDefinitionFound )
{
	ComputeVersion();

	ASSERT_RETURN(m_pszDefinitionName);
	BOOL bFoundDef = bDefinitionFound && pMarkup->FindElem(m_pszDefinitionName);

	void * pTargetCurrent = pTarget;

	for (int ii = 0; m_pElements[ii].m_pszVariableName && m_pElements[ii].m_pszVariableName[0] != 0 ; ii++)
	{
		CDefinitionLoader::CElement * pElement = &m_pElements[ii];
		if (bFoundDef)
		{
			pMarkup->ResetChildPos();
		}

		// handle variable arrays
		int nArrayLength = pElement->m_nArrayLength;
		if (nArrayLength == -1)
		{
			CString sCountName;
			sCountName.Format("%s%s", pElement->m_pszVariableName, VARRAY_SUFFIX);
			if (bFoundDef && pMarkup->FindChildElem(sCountName))
			{
				nArrayLength = PStrToInt(pMarkup->GetChildData());
			} 
			else 
			{
				nArrayLength = 0;
			}
			// copy the array length into the target
			*(int *)((BYTE *)pTarget + pElement->m_nVariableArrayOffset) = nArrayLength;

			if (nArrayLength <= 0)
			{
				*(void **)((BYTE *)pTarget + pElement->m_nOffset) = NULL;
			} 
			else 
			{
				// make the array
				int nElementSize = sElementGetSize(pElement);
				int nArraySize = nArrayLength * nElementSize;
				pTargetCurrent = MALLOCFL(m_pMemPool, nArraySize, (this->m_pszDefinitionName ? this->m_pszDefinitionName : ""), __LINE__);
				*(void **)((BYTE *)pTarget + pElement->m_nOffset) = pTargetCurrent;
				if (bZeroOutData)
				{
					ZeroMemory(pTargetCurrent, nArraySize);
				}
			}
		}
		else
		{
			pTargetCurrent = (BYTE *)pTarget + pElement->m_nOffset;
		}

		// load the elements - for both arrays and non-arrays
		for (int nArrayEntry = 0; nArrayEntry < nArrayLength; nArrayEntry++)
		{
			BOOL bFound = bFoundDef ? pMarkup->FindChildElem(pElement->m_pszVariableName) : FALSE;
			const char * sValue = bFound ? pMarkup->GetChildData() : (pElement->m_pszDefaultValue ? pElement->m_pszDefaultValue : "");

			switch (pElement->m_eDataType)
			{
			case DATA_TYPE_BYTE:
				if (bFound)
				{
					*(BYTE*)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(BYTE)) = (BYTE)PStrToInt(sValue);
				}
				else
				{
					*(BYTE*)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(BYTE)) = *(BYTE*)&pElement->m_DefaultValue;
				}
				break;

			case DATA_TYPE_INT:
				if (bFound)
				{
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = (int)PStrToInt(sValue);
				}
				else
				{
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = *(int *)&pElement->m_DefaultValue;
				}
				break;

			case DATA_TYPE_INT_64:
				if (bFound)
				{
					unsigned __int64 i64;
					PStrToInt(sValue, i64);
					*(unsigned __int64 *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(unsigned __int64)) = i64;
				}
				else
				{
					*(unsigned __int64 *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(unsigned __int64)) = pElement->m_DefaultValue64;
				}
				break;

			case DATA_TYPE_INT_NOSAVE:
			case DATA_TYPE_INT_DEFAULT:
				*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = *(int *)&pElement->m_DefaultValue;
				break;

			case DATA_TYPE_POINTER_NOSAVE:
				*(POINTER_AS_INT *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(POINTER_AS_INT)) = *(int *)&pElement->m_DefaultValue;
				break;

			case DATA_TYPE_FLOAT:
				if (bFound)
				{
					*(float *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(float)) = (float)PStrToFloat(sValue);
				}
				else
				{
					*(float *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(float)) = *(float *)&pElement->m_DefaultValue;
				}
				break;

			case DATA_TYPE_FLOAT_NOSAVE:
				*(float *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(float)) = *(float *)&pElement->m_DefaultValue;
				break;

			case DATA_TYPE_STRING:
				{
					char * pszString = (char *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(char) * MAX_XML_STRING_LENGTH);
					PStrCopy(pszString, sValue, MAX_XML_STRING_LENGTH);
				}
				break;

			case DATA_TYPE_PATH_FLOAT:
				{
					ASSERT(pElement->m_nArrayLength == 1);
					pMarkup->IntoElem();
					LoadPath<float>(pTargetCurrent, pMarkup, (pElement->m_pszDefaultValue ? pElement->m_pszDefaultValue : ""));
					pMarkup->OutOfElem();
				}
				break;

			case DATA_TYPE_PATH_FLOAT_PAIR:
				{
					ASSERT(pElement->m_nArrayLength == 1);
					pMarkup->IntoElem();
					LoadPath<CFloatPair>(pTargetCurrent, pMarkup, (pElement->m_pszDefaultValue ? pElement->m_pszDefaultValue : ""));
					pMarkup->OutOfElem();
				}
				break;

			case DATA_TYPE_PATH_FLOAT_TRIPLE:
				{
					ASSERT(pElement->m_nArrayLength == 1);
					pMarkup->IntoElem();
					LoadPath<CFloatTriple>(pTargetCurrent, pMarkup, (pElement->m_pszDefaultValue ? pElement->m_pszDefaultValue : ""));
					pMarkup->OutOfElem();
				}
				break;

			case DATA_TYPE_DEFINITION:
				{
					ASSERT_BREAK(pElement->m_pDefinitionLoader != NULL);
					pElement->m_pDefinitionLoader->InitValues((BYTE *)pTargetCurrent + nArrayEntry * pElement->m_pDefinitionLoader->m_nStructSize, bZeroOutData);
					if ( bFound )
						pMarkup->IntoElem();

					pElement->m_pDefinitionLoader->Load((BYTE *)pTargetCurrent + nArrayEntry * pElement->m_pDefinitionLoader->m_nStructSize, pMarkup, bZeroOutData, bFound);

					if ( bFound )
						pMarkup->OutOfElem();
				}
				break;

			case DATA_TYPE_FLAG:
				{
					ASSERT(pElement->m_nParam != 0);
					int value = bFound ? PStrToInt(sValue) : *(int *)&pElement->m_DefaultValue;
					if (value)
					{
						*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) |= pElement->m_nParam;
					}
					else
					{
						*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) &= ~pElement->m_nParam;
					}
				}
				break;

			case DATA_TYPE_FLAG_BIT:
				{
					int nSize = sElementGetSize( pElement );
					int value = bFound ? PStrToInt(sValue) : *(int *)&pElement->m_DefaultValue;
					if (value)
					{
						SETBIT( (BYTE *)((BYTE *)pTargetCurrent + nArrayEntry * nSize), pElement->m_nParam );
					}
					else
					{
						CLEARBIT( (BYTE *)((BYTE *)pTargetCurrent + nArrayEntry * nSize), pElement->m_nParam );
					}
				}
				break;

			case DATA_TYPE_INT_LINK:
				if (bFound)
				{
					int value = INVALID_ID;
					if ( sValue[ 0 ] != 0 )
					{
						value = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), (DWORD)pElement->m_nParam, sValue);
					}
					if (value == INVALID_ID)
					{
						value = PStrToInt(sValue);
						if (value == 0 && sValue[0] != '0')
						{
							if (sValue[0])
							{
								ExcelLog("EXCEL WARNING:  DEFINITION LOAD LINK NOT FOUND:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(0)   KEY: %s", 
									m_pszDefinitionName, pElement->m_pszVariableName, ExcelGetTableName(pElement->m_nParam), sValue);
							}
							value = INVALID_ID;
						}
					}
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = value;
				}
				else
				{
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = *(int *)&pElement->m_DefaultValue;
				}
				break;
			case DATA_TYPE_INT_INDEX_LINK:
				if (bFound)
				{
					int value = INVALID_ID;
					if ( sValue[ 0 ] != 0 )
					{
						value = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), (unsigned int)pElement->m_nParam, (unsigned int)pElement->m_nParam2, sValue);
					}
					if (value == INVALID_ID)
					{
						value = PStrToInt(sValue);
						if (value == 0 && sValue[0] != '0')
						{
							if (sValue[0])
							{
								ExcelLog("EXCEL WARNING:  DEFINITION LOAD LINK NOT FOUND:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(%d)   KEY: %s", 
									m_pszDefinitionName, pElement->m_pszVariableName, ExcelGetTableName(pElement->m_nParam), pElement->m_nParam2, sValue);
							}
							value = INVALID_ID;
						}
					}
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = value;
				}
				else
				{
					*(int *)((BYTE *)pTargetCurrent + nArrayEntry * sizeof(int)) = *(int *)&pElement->m_DefaultValue;
				}
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::FreeElements(
	void * pData,
	int nCount,
	BOOL bFreePointer )
{
	BYTE * pCurrDef = (BYTE *) pData;
	for ( int j = 0; j < nCount; j++ )
	{
		for (int ii = 0; m_pElements[ii].m_pszVariableName && m_pElements[ii].m_pszVariableName[0] != 0 ; ii++)
		{
			CDefinitionLoader::CElement * pElement = &m_pElements[ii];

			// handle variable arrays
			int nArrayLength = pElement->m_nArrayLength;
			BYTE * pStartElement;
			if (nArrayLength == -1)
			{
				nArrayLength = *(int *)(pCurrDef + pElement->m_nVariableArrayOffset);
				pStartElement = (BYTE *) *(void **)((BYTE *)pCurrDef + pElement->m_nOffset);
			} else {
				pStartElement = pCurrDef + pElement->m_nOffset;
			}

			BYTE * pCurrElement = pStartElement;
			for ( int k = 0; k < nArrayLength; k++ )
			{
				switch (pElement->m_eDataType)
				{
				case DATA_TYPE_PATH_FLOAT:
					{
						ASSERT(pElement->m_nArrayLength == 1);
						CInterpolationPath<float> * pPath = (CInterpolationPath<float> *) pCurrElement;
						pPath->Destroy();
					}
					break;

				case DATA_TYPE_PATH_FLOAT_PAIR:
					{
						ASSERT(pElement->m_nArrayLength == 1);
						CInterpolationPath<CFloatPair> * pPath = (CInterpolationPath<CFloatPair> *) pCurrElement;
						pPath->Destroy();
					}
					break;

				case DATA_TYPE_PATH_FLOAT_TRIPLE:
					{
						ASSERT(pElement->m_nArrayLength == 1);
						CInterpolationPath<CFloatTriple> * pPath = (CInterpolationPath<CFloatTriple> *) pCurrElement;
						pPath->Destroy();
					}
					break;
				}
				if ( nArrayLength > 1 )
					pCurrElement += sElementGetSize( pElement );
			}

			if ( pElement->m_eDataType == DATA_TYPE_DEFINITION && pStartElement )
			{
				pElement->m_pDefinitionLoader->FreeElements( pStartElement, nArrayLength, FALSE );
			}

			if ( pElement->m_nArrayLength == -1 && pStartElement && nArrayLength > 0 )
			{
				FREE( m_pMemPool, pStartElement );
			}
		}
		pCurrDef += m_nStructSize;
	}

	if ( bFreePointer )
		FREE( m_pMemPool, pData );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::FreeElementData(
										DEFINITION_TRANSFER_CONTEXT & dtc)
{
	dtc.nNumSavedDefinitons = 0; // re-using context structure to keep track of which loaders must be deleted

	FreeElementData_(dtc);

	for (int ii=0; ii < dtc.nNumSavedDefinitons; ++ii)
	{
		FREE(g_ScratchAllocator, dtc.pSavedDefinitionLoader[ii]);
	}

	dtc.nNumSavedDefinitons = 0;
}

void CDefinitionLoader::FreeElementData_(DEFINITION_TRANSFER_CONTEXT & dtc)
{
	for (int ii = 0; ii < MAX_ELEMENTS_PER_DEFINITION; ++ii)
	{
		if (m_pElements[ii].m_pszDefaultValue)
		{
			FREE(g_ScratchAllocator, m_pElements[ii].m_pszDefaultValue);
			m_pElements[ii].m_pszDefaultValue = NULL;
		}

		if (m_pElements[ii].m_pDefinitionLoader)
		{
			CDefinitionLoader * pDefinitionLoader = m_pElements[ii].m_pDefinitionLoader;
			m_pElements[ii].m_pDefinitionLoader = NULL;

			if (pDefinitionLoader != this)
			{
				BOOL bFound = FALSE;

				for (int jj = 0; jj < dtc.nNumSavedDefinitons; ++jj)
				{
					if (dtc.pSavedDefinitionLoader[jj] == pDefinitionLoader)
					{
						bFound = TRUE;
						break;
					}
				}

				if (!bFound)
				{
					ASSERT(dtc.nNumSavedDefinitons < MAX_SAVED_DEFINITIONS);
					dtc.pSavedDefinitionLoader[dtc.nNumSavedDefinitons++] = pDefinitionLoader;
					pDefinitionLoader->FreeElementData_(dtc);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::InitElementData()
{
	ASSERT(m_pszDefinitionName != NULL);
	m_dwDefinitionNameHash = CRC(0, (BYTE *)m_pszDefinitionName, PStrLen(m_pszDefinitionName));
#if !ISVERSION(SERVER_VERSION)
	// CHB 2007.08.01 - Clarifying expression for developer diagnostics
	// and eliminating message string that is uninformative to end user.
	HALT(m_dwDefinitionNameHash != 0);	// CHB 2007.08.01 - String audit: USED IN RELEASE
#endif

	if (m_nNumElements)
	{
		return;
	}

	m_nNumElements = 0;
	while (m_pElements[m_nNumElements].m_pszVariableName || m_pElements[m_nNumElements].m_dwVariableNameHash)
	{
		++m_nNumElements;
	}

	for (int ii = 0; ii < (int)m_nNumElements; ++ii)
	{
		CDefinitionLoader::CElement * element = &m_pElements[ii];
		element->m_dwVariableNameHash = CRC(0, (BYTE *)element->m_pszVariableName, PStrLen(element->m_pszVariableName));
#if !ISVERSION(SERVER_VERSION)
		// CHB 2007.08.01 - Clarifying expression for developer diagnostics
		// and eliminating message string that is uninformative to end user.
		HALT(element->m_dwVariableNameHash != 0);	// CHB 2007.08.01 - String audit: USED IN RELEASE
#endif
		FixDefault(element);

		if (element->m_pDefinitionLoader)
		{
			element->m_pDefinitionLoader->InitElementData();
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CDefinitionLoader * CDefinitionLoader::AllocEmptyDefLoader()
{
	CDefinitionLoader * pDefLoader = (CDefinitionLoader*)MALLOCZ(g_ScratchAllocator, sizeof(CDefinitionLoader));

	for (int jj=0; jj<MAX_ELEMENTS_PER_DEFINITION; ++jj)
	{
		pDefLoader->m_pElements[jj].m_eDataType = DATA_TYPE_NONE;
		pDefLoader->m_pElements[jj].m_eElementType = ELEMENT_TYPE_NONE;
		pDefLoader->m_pElements[jj].m_nParam = -1;
		pDefLoader->m_pElements[jj].m_nParam2 = -1;
	}

	return pDefLoader;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void resizeTempBuf(
									DEFINITION_TRANSFER_CONTEXT & dtc,
									struct MEMORYPOOL * mempool,
									int nSize)
{
	if (dtc.pTempBuf)
	{
		FREE(mempool, dtc.pTempBuf);
	}

	nSize = (int)NEXTPOWEROFTWO((DWORD)nSize); // attempt to minimize the number of re-MALLOCs

	dtc.pTempBuf = MALLOCZ(mempool, nSize);

	if (dtc.pTempBuf)
	{
		dtc.nTempBufSize = nSize;
	}
	else
	{
		ASSERTV(0, "Temp buffer alloc of %d bytes failed!", nSize);
		dtc.nTempBufSize = 0;
	}
}

inline static void * getTempBuf(
									DEFINITION_TRANSFER_CONTEXT & dtc,
									struct MEMORYPOOL * mempool,
									int nSize)
{
	if (nSize > dtc.nTempBufSize)
	{
		resizeTempBuf(dtc, mempool, nSize);
	}

	return dtc.pTempBuf;
}

inline static void freeTempBuf(
									DEFINITION_TRANSFER_CONTEXT & dtc,
									struct MEMORYPOOL * mempool)
{
	if (dtc.pTempBuf)
	{
		FREE(mempool, dtc.pTempBuf);
		dtc.pTempBuf = NULL;
	}

	dtc.nTempBufSize = 0;
}

#define GET_TEMP_BUF(n) (getTempBuf(dtc, g_ScratchAllocator, n))
#define FREE_TEMP_BUF() (freeTempBuf(dtc, g_ScratchAllocator))

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL CDefinitionLoader::SaveCooked(
									const void * source,
									WRITE_BUF_DYNAMIC & fbuf)
{
	DEFINITION_TRANSFER_CONTEXT dtc;
	dtc.nReadOrWrite = DEFINITION_TRANSFER_WRITE;
	dtc.nNumSavedDefinitons = 0;
	dtc.writeBuf = &fbuf;

	return TransferCooked(dtc, source);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDefinitionLoader::LoadCooked(void * target,
									class BYTE_BUF & buf,
									BOOL bZeroOutData)
{
	DEFINITION_TRANSFER_CONTEXT dtc;
	dtc.nReadOrWrite = DEFINITION_TRANSFER_READ;
	dtc.nNumSavedDefinitons = 0;
	dtc.readBuf = &buf;

	return TransferCooked(dtc, target, bZeroOutData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<typename T> inline static BOOL transferInt(DEFINITION_TRANSFER_CONTEXT & dtc, T &x)
{
	const BOOL retVal = (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? dtc.writeBuf->PushInt(x) : dtc.readBuf->PopInt(&(x),0));
	DEF_PRINT("%s int %I64d (%d bytes)", dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? "writing" : "reading", (INT64)x, sizeof(x));
	return retVal;
}

inline static BOOL transferBuf(DEFINITION_TRANSFER_CONTEXT & dtc, void * x, const int y)
{
	const BOOL retVal = (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? dtc.writeBuf->PushBuf(x,y) : dtc.readBuf->PopBuf(x,y));

#if defined(DEFINITION_TRACE_DEBUG) || defined(DEFINITION_LOG_DEBUG)
	char * printBuf = (char*)GET_TEMP_BUF(y+1);
	memcpy(printBuf, x, y);
	printBuf[y] = '\0';
	DEF_PRINT("%s buffer of %d bytes: '%s'", dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? "writing" : "reading", y, printBuf);
#endif

	return retVal;
}

#define TRANSFER_INT(x) transferInt(dtc, x)
#define TRANSFER_BUF(x,y) transferBuf(dtc, x, y)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL CDefinitionLoader::TransferCooked(
									DEFINITION_TRANSFER_CONTEXT & dtc,					
									const void * source,									
									BOOL bZeroOutData)
{
	dtc.pTempBuf = NULL;
	dtc.nTempBufSize = 0;

	// initialize a definition loader
	//
	CDefinitionLoader * pDefLoader;

	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
	{
		pDefLoader = AllocEmptyDefLoader();
	}
	else
	{
		pDefLoader = this;
	}

	// transfer the header block
	//
	FILE_HEADER hdr;
	hdr.dwMagicNumber = COOKED_MAGIC_NUMBER;
	hdr.nVersion = DEFINITION_FILE_VERSION;
	ASSERT_RETFALSE(TRANSFER_INT(hdr.dwMagicNumber));
	ASSERT_RETFALSE(TRANSFER_INT(hdr.nVersion));
	ASSERT_RETFALSE(hdr.dwMagicNumber == COOKED_MAGIC_NUMBER);
	ASSERT_RETFALSE(hdr.nVersion == DEFINITION_FILE_VERSION);

	BOOL bRet = pDefLoader->TransferCookedDefinitions(dtc);

	if (bRet)
	{
		// transfer the data block
		//
		DWORD dwDataMarker = COOKED_DATA_MARKER;
		ASSERT_RETFALSE(TRANSFER_INT(dwDataMarker));
		ASSERT_RETFALSE(dwDataMarker == COOKED_DATA_MARKER);

		bRet = TransferCookedData(dtc, source, pDefLoader, bZeroOutData);
	}

	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
	{
		pDefLoader->FreeElementData(dtc);
		FREE(g_ScratchAllocator, pDefLoader);
	}

	FREE_TEMP_BUF();

	return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDefinitionLoader::TransferCookedDefinitions(
									DEFINITION_TRANSFER_CONTEXT & dtc)
{
	DEF_PRINT("\n*** %s header for definition '%s'", dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? "writing" : "reading", m_pszDefinitionName);

	#define DEFINITION_REPEATED_IND ((DWORD)(-1))

	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
	{
		ASSERT(m_pszDefinitionName != NULL);
		m_dwDefinitionNameHash = CRC(0, (BYTE *)m_pszDefinitionName, PStrLen(m_pszDefinitionName));
#if !ISVERSION(SERVER_VERSION)
		// CHB 2007.08.01 - Clarifying expression for developer diagnostics
		// and eliminating message string that is uninformative to end user.
		HALT(m_dwDefinitionNameHash != 0);	// CHB 2007.08.01 - String audit: USED IN RELEASE
#endif

		ASSERT_RETFALSE(TRANSFER_INT(m_dwDefinitionNameHash));

		for (int i=0; i<dtc.nNumSavedDefinitons; ++i)
		{
			if (dtc.nSavedDefinition[i] == m_dwDefinitionNameHash)
			{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
				// CHB 2007.08.01 - Since this is development-only,
				// an assertion will suffice here.
				ASSERTX(PStrCmp(dtc.pszDefinitionName[i], m_pszDefinitionName) == 0, "Definition name hash collision!");
#endif

				DWORD nNumElements = DEFINITION_REPEATED_IND;
				ASSERT_RETFALSE(TRANSFER_INT(nNumElements));
				return TRUE;
			}
		}

		m_nNumElements = 0;
		while (m_pElements[m_nNumElements].m_pszVariableName && m_pElements[m_nNumElements].m_pszVariableName[0] != 0)
		{
			++m_nNumElements;
		}

		ASSERT_RETFALSE(TRANSFER_INT(m_nNumElements));
	}
	else
	{
		ASSERT_RETFALSE(TRANSFER_INT(m_dwDefinitionNameHash));
		ASSERT_RETFALSE(TRANSFER_INT(m_nNumElements));

		if (m_nNumElements == DEFINITION_REPEATED_IND)
		{
			return TRUE;
		}
	}

	ASSERT_RETFALSE(dtc.nNumSavedDefinitons < MAX_SAVED_DEFINITIONS);
	dtc.nSavedDefinition[dtc.nNumSavedDefinitons] = m_dwDefinitionNameHash;
	dtc.pSavedDefinitionLoader[dtc.nNumSavedDefinitons] = this;
#if ISVERSION(DEVELOPMENT)
	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
	{
		PStrCopy(dtc.pszDefinitionName[dtc.nNumSavedDefinitons], m_pszDefinitionName, 256);
	}
#endif
	dtc.nNumSavedDefinitons++;

	for (int ii = 0; ii < (int)m_nNumElements; ++ii)
	{
		CDefinitionLoader::CElement * element = &m_pElements[ii];

		if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
		{
			element->m_dwVariableNameHash = CRC(0, (BYTE *)element->m_pszVariableName, PStrLen(element->m_pszVariableName));
#if !ISVERSION(SERVER_VERSION)
			// CHB 2007.08.01 - Clarifying expression for developer diagnostics
			// and eliminating message string that is uninformative to end user.
			HALT(element->m_dwVariableNameHash != 0);	// CHB 2007.08.01 - String audit: USED IN RELEASE
#endif

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
			for (int jj = 0; jj < (int)m_nNumElements; ++jj)
			{
				if (element->m_dwVariableNameHash == m_pElements[jj].m_dwVariableNameHash && ii != jj)
				{
					// CHB 2007.08.01 - Since this is development-only,
					// an assertion will suffice here.
					ASSERTX(PStrCmp(element->m_pszVariableName, m_pElements[jj].m_pszVariableName) == 0, "Element variable name hash collision!");
				}
			}
#endif
		}

		ASSERT_RETFALSE(TRANSFER_INT(element->m_dwVariableNameHash));
		ASSERT_RETFALSE(TRANSFER_INT(element->m_eElementType));
		ASSERT_RETFALSE(TRANSFER_INT(element->m_eDataType));

		// read/write the default value, in the appropriate format
		//
		FixDefault(element);

		switch (element->m_eElementType)
		{
			case ELEMENT_TYPE_MEMBER_LINK:
			case ELEMENT_TYPE_MEMBER_LINK_ARRAY:
			case ELEMENT_TYPE_MEMBER_INDEX_LINK:
			{
				if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
				{
					element->m_pszDefaultValue = NULL; // ""
					element->m_DefaultValue = (DWORD)(-1);
				}
				break;
			}
			case ELEMENT_TYPE_REFERENCE:
			case ELEMENT_TYPE_REFERENCE_ARRAY:
			case ELEMENT_TYPE_REFERENCE_VARRAY:
			{
				if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
				{
					element->m_pszDefaultValue = NULL; // ""
					element->m_DefaultValue = 0;
				}
				break;
			}
			case ELEMENT_TYPE_MEMBER_FLAG_BIT:
			{
				if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
				{
					element->m_pszDefaultValue = NULL; // "0"
					element->m_DefaultValue = 0;
				}
				break;
			}
			default:
			{
				switch (element->m_eDataType)
				{
					case DATA_TYPE_STRING:
					{
						BYTE iStrLen = 0;
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
						{
							iStrLen = (BYTE)PStrLen(element->m_pszDefaultValue);
						}

						ASSERT_RETFALSE(TRANSFER_INT(iStrLen));

						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
						{
							element->m_pszDefaultValue = (const char *)MALLOCFL(g_ScratchAllocator, iStrLen + 1, __FILE__, __LINE__);
						}

						if (iStrLen)
						{
							ASSERT_RETFALSE(TRANSFER_BUF((void*)element->m_pszDefaultValue, iStrLen + 1));
						}
						else if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
						{
							*((char*)element->m_pszDefaultValue) = '\0';
						}

						element->m_DefaultValue = 0;
						break;
					}
					case DATA_TYPE_POINTER_NOSAVE:
					{
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
						{
							element->m_pszDefaultValue = NULL; // "0"
							element->m_DefaultValue64 = 0;
						}
						break;
					}
					case DATA_TYPE_INT_64:
					{
						ASSERT_RETFALSE(TRANSFER_INT(element->m_DefaultValue64));
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
						{
							element->m_pszDefaultValue = NULL; // ""
						}
						break;
					}
					default:
					{
						ASSERT_RETFALSE(TRANSFER_INT(*((int*)&element->m_DefaultValue)));
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
						{
							element->m_pszDefaultValue = NULL; // ""
						}
						break;
					}
				}
			}
			break;
		}

		// read/write the parameter(s), if necessary
		//
		switch (element->m_eDataType)
		{
			case DATA_TYPE_FLAG:
			{
				ASSERT_RETFALSE(TRANSFER_INT(element->m_nParam));
				break;
			}
			case DATA_TYPE_FLAG_BIT:
			{
				ASSERT_RETFALSE(TRANSFER_INT(element->m_nParam));
				ASSERT_RETFALSE(TRANSFER_INT(element->m_nParam2));
				break;
			}
			case DATA_TYPE_INT_LINK:
			case DATA_TYPE_INT_INDEX_LINK:
			{
				DWORD wCode;

				if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
				{
					ASSERT_RETFALSE(TRANSFER_INT(wCode));
					element->m_nParam = ExcelTableGetByCode(wCode);
				}
				else
				{
					wCode = ExcelTableGetCode(element->m_nParam);
					ASSERT_RETFALSE(TRANSFER_INT(wCode));
				}

				if (element->m_eDataType == DATA_TYPE_INT_INDEX_LINK)
				{
					ASSERT_RETFALSE(TRANSFER_INT(element->m_nParam2));
				}
				break;
			}
		}

		// read/write the array length, if necessary
		//
		switch (element->m_eElementType)
		{
			case ELEMENT_TYPE_MEMBER_LINK_ARRAY:
			case ELEMENT_TYPE_MEMBER_ARRAY:
			case ELEMENT_TYPE_REFERENCE_ARRAY:
			{
				ASSERT_RETFALSE(TRANSFER_INT(element->m_nArrayLength));
				break;
			}
			case ELEMENT_TYPE_MEMBER_VARRAY:
			case ELEMENT_TYPE_REFERENCE_VARRAY:
			{
				element->m_nArrayLength = -1;
				break;
			}
			default:
			{
				element->m_nArrayLength = 1;
				break;
			}
		}

		if (element->m_eDataType == DATA_TYPE_DEFINITION)
		{
			if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
			{
				if (!element->m_pDefinitionLoader)
				{
					// read ahead and don't alloc if not necessary
					//
					DWORD dwDefinitionNameHash;
					int nNumElements;

					const unsigned int oldPos = dtc.readBuf->GetCursor();
					ASSERT_RETFALSE(TRANSFER_INT(dwDefinitionNameHash));
					ASSERT_RETFALSE(TRANSFER_INT(nNumElements));

					if (nNumElements == DEFINITION_REPEATED_IND)
					{
						// the definition was flagged as already loaded
						//
						for (int i=0; i<dtc.nNumSavedDefinitons; ++i)
						{
							if (dtc.nSavedDefinition[i] == dwDefinitionNameHash)
							{
								element->m_pDefinitionLoader = dtc.pSavedDefinitionLoader[i];
								break;
							}
						}

						ASSERT_RETFALSE(element->m_pDefinitionLoader);
						continue;
					}
					else
					{
						dtc.readBuf->SetCursor(oldPos);
						element->m_pDefinitionLoader = AllocEmptyDefLoader();
					}
				}
			}

			ASSERT_RETFALSE(element->m_pDefinitionLoader);

			element->m_pDefinitionLoader->TransferCookedDefinitions(dtc);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_LINK_STRING_LENGTH 254

BOOL CDefinitionLoader::TransferCookedData(
										DEFINITION_TRANSFER_CONTEXT & dtc,					
										const void * source,
										CDefinitionLoader * pOldDefs,
										BOOL bZeroOutData)
{
	#define MAX_USED_MEMBER_BYTES 32

	DEF_PRINT("\n*** %s data for definition '%s'\n", dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE ? "writing" : "reading", m_pszDefinitionName);

	ASSERT_RETFALSE(pOldDefs != NULL);

	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
	{
		// first, populate the structure with the current default values (in case some new elements didn't exist in the old definition)
		//
		for (int ii = 0; m_pElements[ii].m_pszVariableName || m_pElements[ii].m_dwVariableNameHash; ++ii)
		{
			CDefinitionLoader::CElement * element = &m_pElements[ii];

			if (element->m_nArrayLength > 0)
			{
				void * ptr = (void *)((BYTE *)source + element->m_nOffset);
				FillWithDefaults(ptr, element, element, 0, element->m_nArrayLength);
			}
		}
	}

	BYTE iUsedMemberBits[MAX_USED_MEMBER_BYTES];
	memset(iUsedMemberBits, 0, MAX_USED_MEMBER_BYTES * sizeof(BYTE));

	const int iNumUsedMemberBytes = pOldDefs->m_nNumElements ? BYTE_FLAG_SIZE(pOldDefs->m_nNumElements) : 0;
	ASSERT_RETFALSE(iNumUsedMemberBytes < MAX_USED_MEMBER_BYTES);

	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
	{
		// figure out which members are "used" (i.e., non-default)
		//
		for (int ii = 0; pOldDefs->m_pElements[ii].m_pszVariableName || pOldDefs->m_pElements[ii].m_dwVariableNameHash; ii++)
		{
			const CDefinitionLoader::CElement * element = &pOldDefs->m_pElements[ii];
			void * ptr = (void *)((BYTE *)source + element->m_nOffset);

			if (!ElementIsDefault(ptr, element))
			{
				int iWhichByte = (ii>>3);
				ASSERT_RETFALSE(iWhichByte < MAX_USED_MEMBER_BYTES);
				iUsedMemberBits[iWhichByte] |= MAKE_MASK(ii & 7);
			}
		}
	}

	ASSERT_RETFALSE(TRANSFER_BUF((void*)iUsedMemberBits, iNumUsedMemberBytes));

	for (int ii = 0; pOldDefs->m_pElements[ii].m_pszVariableName || pOldDefs->m_pElements[ii].m_dwVariableNameHash; ii++)
	{
		int iWhichByte = (ii>>3);
		ASSERT_RETFALSE(iWhichByte < MAX_USED_MEMBER_BYTES);

		// can't do this yet because ... even if it is default we have to match up to find the *old* default value
		//
//		if (!(iUsedMemberBits[iWhichByte] & MAKE_MASK(ii & 7)))
//		{
//			continue;
//		}

		CDefinitionLoader::CElement * oldElement = &pOldDefs->m_pElements[ii];
		CDefinitionLoader::CElement * element;

		static const CDefinitionLoader::CElement nullElement = {"", 0, ELEMENT_TYPE_NONE, DATA_TYPE_NONE, NULL, NULL, 0, 0, 0, 0, 0, "", 0};

		if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
		{
			element = oldElement;
		}
		else
		{
			// we are reading, look for a matching element
			//
			if (oldElement->m_pNewElement)
			{
				element = oldElement->m_pNewElement;
			}
			else
			{
				element = (CDefinitionLoader::CElement*)&nullElement;

				for (int jj=0; jj<(int)m_nNumElements; ++jj)
				{
					if (m_pElements[jj].m_dwVariableNameHash == oldElement->m_dwVariableNameHash)
					{
						if (m_pElements[jj].m_eElementType != oldElement->m_eElementType)
						{
							if ((m_pElements[jj].m_eElementType == ELEMENT_TYPE_MEMBER && oldElement->m_eElementType == ELEMENT_TYPE_MEMBER_ARRAY)           ||
								(m_pElements[jj].m_eElementType == ELEMENT_TYPE_MEMBER_ARRAY && oldElement->m_eElementType == ELEMENT_TYPE_MEMBER)           ||
								(m_pElements[jj].m_eElementType == ELEMENT_TYPE_REFERENCE && oldElement->m_eElementType == ELEMENT_TYPE_REFERENCE_ARRAY)     ||
								(m_pElements[jj].m_eElementType == ELEMENT_TYPE_REFERENCE_ARRAY && oldElement->m_eElementType == ELEMENT_TYPE_REFERENCE)     ||
								(m_pElements[jj].m_eElementType == ELEMENT_TYPE_MEMBER_LINK && oldElement->m_eElementType == ELEMENT_TYPE_MEMBER_LINK_ARRAY) ||
								(m_pElements[jj].m_eElementType == ELEMENT_TYPE_MEMBER_LINK_ARRAY && oldElement->m_eElementType == ELEMENT_TYPE_MEMBER_LINK))
							{
								// this should work as-is: the only difference is the nArrayLength
								//
								element = &m_pElements[jj];
								break;
							}

							// TODO: are there any other cases which should be supported?
							//
							ASSERTV_RETFALSE(0, "WARNING: Element '%s' cannot be re-used because element type (%d) differs from previous type (%d)!  See Mike K.\n", m_pElements[jj].m_pszVariableName, m_pElements[jj].m_eElementType, oldElement->m_eElementType);
						}
						else if (m_pElements[jj].m_nParam != oldElement->m_nParam)
						{
							// TODO: support if link table changes
							ASSERTV_RETFALSE(0, "WARNING: Element '%s' cannot be re-used because m_nParam (%d) differs from previous value (%d)!  See Mike K.\n", m_pElements[jj].m_pszVariableName, m_pElements[jj].m_nParam, oldElement->m_nParam);
						}
						else if (m_pElements[jj].m_nParam2 != oldElement->m_nParam2)
						{
							if (oldElement->m_eElementType == ELEMENT_TYPE_MEMBER_FLAG_BIT)
							{
								// we need to be tolerant of this for ELEMENT_TYPE_MEMBER_FLAG_BIT
								//
								if (oldElement->m_nParam < (sElementGetSize(&m_pElements[jj])<<3))
								{
									element = &m_pElements[jj];
								}
								else
								{
									ASSERTV(0, "WARNING: Discarding MEMBER_FLAG_BIT because it no longer fits in flag size!  See Mike K.\n");
								}
								break;
							}

							ASSERTV_RETFALSE(0, "WARNING: Element '%s' cannot be re-used because m_nParam2 (%d) differs from previous value (%d)!  See Mike K.\n", m_pElements[jj].m_pszVariableName, m_pElements[jj].m_nParam2, oldElement->m_nParam2);
						}
						else
						{
							element = &m_pElements[jj];
							break;
						}
					}
				}

				// remember result of this lookup for next time
				//
				oldElement->m_pNewElement = element;
			}
		}

		if (!(iUsedMemberBits[iWhichByte] & MAKE_MASK(ii & 7)))
		{
			if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && element != &nullElement)
			{
				// populate with old default value
				//
				void * ptr = (void *)((BYTE *)source + element->m_nOffset);
				FillWithDefaults(ptr, element, oldElement, 0, element->m_nArrayLength);
			}

			continue;
		}

		int new_array_length = element->m_nArrayLength;
		int old_array_length = oldElement->m_nArrayLength;

		void * ptr = NULL;
		BOOL bDeletePtr = FALSE;

		if (old_array_length == -1)
		{
			if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
			{
				old_array_length = *(int *)((BYTE *)source + element->m_nVariableArrayOffset);
				new_array_length = old_array_length;
				ptr = (void *)((BYTE *)source + element->m_nOffset);
				ptr = *(void **)ptr;
				ASSERT_RETFALSE(TRANSFER_INT(new_array_length));
			}
			else
			{
				ASSERT_RETFALSE(TRANSFER_INT(old_array_length));
				new_array_length = old_array_length;

				if(element != &nullElement)
				{
					*(int *)((BYTE *)source + element->m_nVariableArrayOffset) = new_array_length;
					if (new_array_length > 0)
					{
						int element_size = sElementGetSize(element);
						ptr = MALLOCZFL(m_pMemPool, new_array_length * element_size, (this->m_pszDefinitionName ? this->m_pszDefinitionName : ""), __LINE__);
						*(void **)((BYTE *)source + element->m_nOffset) = ptr;
					}
					else
					{
						*(void **)((BYTE *)source + element->m_nOffset) = NULL;
					}
				}
			}
		}

		if (old_array_length <= 0)
		{
//			continue;
		}

		int file_array_length = new_array_length < old_array_length ? new_array_length : old_array_length;		// read this many elements from the file
		int skip_array_length = new_array_length < old_array_length ? old_array_length - new_array_length : 0;	// skip over this many elements
		int fill_array_length = new_array_length > old_array_length ? new_array_length - old_array_length : 0;	// fill this many elements with default values

		if (!ptr)
		{
			if (element == &nullElement)
			{
				// this element does not exist in the new version of the definition - but we still have to read past it
				//
				int element_size = sElementGetSize(oldElement);
				ptr = MALLOC(g_ScratchAllocator, file_array_length * element_size);
				bDeletePtr = TRUE;

//				ASSERT_RETFALSE(file_array_length == 0); // not necessary to MALLOC since file_array_length should always be zero
//				ASSERT_RETFALSE(fill_array_length == 0); // (since nullElement has arraySize == 0)
			}
			else
			{
				ptr = (void *)((BYTE *)source + element->m_nOffset);
			}
		}

//		if (file_array_length)
		{
			switch (element->m_eDataType)
			{
				default:
				{
					if (element->m_eDataType == oldElement->m_eDataType)
					{
						int element_size = sElementGetSize(element);
						int array_size = file_array_length * element_size;
						ASSERT_RETFALSE(TRANSFER_BUF(ptr, array_size));
					}
					else
					{
						// Handle when data types are different
						//
						const int element_size = sElementGetSize(oldElement);
						const int array_size = file_array_length * element_size;
						BYTE * readBuf = (BYTE*)GET_TEMP_BUF(array_size);
						ASSERT_RETFALSE(TRANSFER_BUF(readBuf, array_size));

						if (element != &nullElement)
						{
							#define OLD_ELEMENT_SHIFT		8
							#define NEW_OLD_TYPES(x,y)		((x)|((y)<<OLD_ELEMENT_SHIFT))
							#define	CASE_CONVERT(x,y,z,w)	case NEW_OLD_TYPES(x,y): for (int jj = 0; jj < file_array_length; ++jj) {*((z*)ptr + jj) = (z)(*((w*)readBuf + jj));} break;

							switch (NEW_OLD_TYPES(element->m_eDataType, oldElement->m_eDataType))
							{
								CASE_CONVERT( DATA_TYPE_INT,	DATA_TYPE_FLOAT,	int,				float				);
								CASE_CONVERT( DATA_TYPE_INT,	DATA_TYPE_BYTE,		int,				BYTE				);
								CASE_CONVERT( DATA_TYPE_INT,	DATA_TYPE_INT_64,	int,				unsigned __int64	);

								CASE_CONVERT( DATA_TYPE_FLOAT,	DATA_TYPE_INT,		float,				int					);
								CASE_CONVERT( DATA_TYPE_FLOAT,	DATA_TYPE_BYTE,		float,				BYTE				);
								CASE_CONVERT( DATA_TYPE_FLOAT,	DATA_TYPE_INT_64,	float,				unsigned __int64	);

								CASE_CONVERT( DATA_TYPE_BYTE,	DATA_TYPE_INT,		BYTE,				int					);
								CASE_CONVERT( DATA_TYPE_BYTE,	DATA_TYPE_FLOAT,	BYTE,				float				);
								CASE_CONVERT( DATA_TYPE_BYTE,	DATA_TYPE_INT_64,	BYTE,				unsigned __int64	);

								CASE_CONVERT( DATA_TYPE_INT_64,	DATA_TYPE_INT,		unsigned __int64,	int					);
								CASE_CONVERT( DATA_TYPE_INT_64,	DATA_TYPE_FLOAT,	unsigned __int64,	float				);
								CASE_CONVERT( DATA_TYPE_INT_64,	DATA_TYPE_BYTE,		unsigned __int64,	BYTE				);

								// TODO: more permutations that should probably be supported
								//

								// normal <- nosave/default
								case NEW_OLD_TYPES( DATA_TYPE_INT,   DATA_TYPE_INT_NOSAVE   ):
								case NEW_OLD_TYPES( DATA_TYPE_INT,   DATA_TYPE_INT_DEFAULT  ):
								case NEW_OLD_TYPES( DATA_TYPE_FLOAT, DATA_TYPE_FLOAT_NOSAVE ):

								// these require conversion
								case NEW_OLD_TYPES( DATA_TYPE_INT,   DATA_TYPE_FLOAT_NOSAVE ):
								case NEW_OLD_TYPES( DATA_TYPE_FLOAT, DATA_TYPE_INT_NOSAVE   ):
								case NEW_OLD_TYPES( DATA_TYPE_FLOAT, DATA_TYPE_INT_DEFAULT  ):

								// nosave/default <- normal
								case NEW_OLD_TYPES( DATA_TYPE_INT_NOSAVE,   DATA_TYPE_INT   ):
								case NEW_OLD_TYPES( DATA_TYPE_INT_DEFAULT,  DATA_TYPE_INT   ):
								case NEW_OLD_TYPES( DATA_TYPE_FLOAT_NOSAVE, DATA_TYPE_FLOAT ):

								// these require conversion
								case NEW_OLD_TYPES( DATA_TYPE_FLOAT_NOSAVE, DATA_TYPE_INT   ):
								case NEW_OLD_TYPES( DATA_TYPE_INT_NOSAVE,   DATA_TYPE_FLOAT ):
								case NEW_OLD_TYPES( DATA_TYPE_INT_DEFAULT,  DATA_TYPE_FLOAT ):

								default:
								{
									ASSERTV_RETFALSE(0, "WARNING: Don't know how to convert definition data type %d to type %d!  See Mike K.\n", oldElement->m_eDataType, element->m_eDataType);
								}
							}
						}
					}
				}
				break;

				case DATA_TYPE_STRING:
				{
					int len = 0;
					for (int jj = 0; jj < file_array_length; jj++)
					{
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
						{
							len = PStrLen((const char*)ptr) + 1;
						}
						ASSERT_RETFALSE(TRANSFER_INT(len));
						ASSERT_RETFALSE(TRANSFER_BUF(ptr, len));
						(*(BYTE **)&ptr) += sElementGetSize(element);
					}

					if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && skip_array_length)
					{
						// read past extraneous strings
						//
						for (int jj = 0; jj < skip_array_length; jj++)
						{
							ASSERT_RETFALSE(TRANSFER_INT(len));
							char * szJunkString = (char*)GET_TEMP_BUF(len);
							ASSERT_RETFALSE(TRANSFER_BUF(szJunkString, len));
						}

						skip_array_length = 0;
					}
				}
				break;

				case DATA_TYPE_INT_NOSAVE:
				case DATA_TYPE_INT_DEFAULT:
				case DATA_TYPE_POINTER_NOSAVE:
				case DATA_TYPE_FLOAT_NOSAVE:
				{
					fill_array_length = new_array_length;
					file_array_length = 0;
					skip_array_length = 0;
				}
				break;

				case DATA_TYPE_FLAG:
				case DATA_TYPE_FLAG_BIT:
				{
					if (element != &nullElement)
					{
						BOOL found = FALSE;

						for (int jj = 0; m_pElements + jj != element; jj++)
						{
							ASSERT_RETFALSE(jj < MAX_ELEMENTS_PER_DEFINITION);

							int iWhichByte2 = (jj>>3);
							ASSERT_RETFALSE(iWhichByte2 < MAX_USED_MEMBER_BYTES);
							if (iUsedMemberBits[iWhichByte2] & MAKE_MASK(jj & 7))
							{
								const CDefinitionLoader::CElement * elem = m_pElements + jj;
								if (elem->m_nOffset == element->m_nOffset)
								{
									found = TRUE;
									break;
								}
							}
						}

						if (!found)
						{
							int element_size = sElementGetSize(element);
							int array_size = file_array_length * element_size;
							ASSERT_RETFALSE(TRANSFER_BUF(ptr, array_size));
						}
					}

					if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && skip_array_length)
					{
						// read past extraneous flag data
						//
						int element_size = sElementGetSize(element);
						int array_size = skip_array_length * element_size;
						BYTE * bitBucket = (BYTE*)GET_TEMP_BUF(array_size);
						ASSERT_RETFALSE(TRANSFER_BUF(bitBucket, array_size));
						skip_array_length = 0;
					}
				}
				break;

				case DATA_TYPE_PATH_FLOAT:
				{
					CInterpolationPath<float> * path = (CInterpolationPath<float> *)ptr;
					if (!TransferCookedPath(dtc, path, element, file_array_length, skip_array_length))
					{
						return FALSE;
					}
				}
				break;

				case DATA_TYPE_PATH_FLOAT_PAIR:
				{
					CInterpolationPath<CFloatPair> * path = (CInterpolationPath<CFloatPair> *)ptr;
					if (!TransferCookedPath(dtc, path, element, file_array_length, skip_array_length))
					{
						return FALSE;
					}
				}
				break;

				case DATA_TYPE_PATH_FLOAT_TRIPLE:
				{
					CInterpolationPath<CFloatTriple> * path = (CInterpolationPath<CFloatTriple> *)ptr;
					if (!TransferCookedPath(dtc, path, element, file_array_length, skip_array_length))
					{
						return FALSE;
					}
				}
				break;

				case DATA_TYPE_DEFINITION:
				{
					for (int jj = 0; jj < file_array_length; jj++)
					{
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
						{
							element->m_pDefinitionLoader->TransferCookedData(dtc, (BYTE *)ptr + jj * element->m_pDefinitionLoader->m_nStructSize, oldElement->m_pDefinitionLoader, bZeroOutData);
						}
						else
						{
							element->m_pDefinitionLoader->InitValues((BYTE *)ptr + jj * element->m_pDefinitionLoader->m_nStructSize, bZeroOutData);
							if (!element->m_pDefinitionLoader->TransferCookedData(dtc, (BYTE *)ptr + jj * element->m_pDefinitionLoader->m_nStructSize, oldElement->m_pDefinitionLoader, bZeroOutData))
							{
								return FALSE;
							}
						}
					}

					if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && skip_array_length)
					{
						// read past extraneous definition data
						//
						BYTE * bitBucket = (BYTE*)MALLOCZ(g_ScratchAllocator, element->m_pDefinitionLoader->m_nStructSize);
						for (int jj = 0; jj < skip_array_length; jj++)
						{
							if (!element->m_pDefinitionLoader->TransferCookedData(dtc, bitBucket, oldElement->m_pDefinitionLoader, bZeroOutData))
							{
								FREE(g_ScratchAllocator, bitBucket);							
								return FALSE;
							}
						}
						FREE(g_ScratchAllocator, bitBucket);

						skip_array_length = 0;
					}
				}
				break;

				case DATA_TYPE_INT_LINK:
				case DATA_TYPE_INT_INDEX_LINK:
				{
					static BYTE bInvalid = (BYTE)(EXCEL_LINK_INVALID & 0xff);

					ASSERT_BREAK(element->m_nParam >= 0);

					int element_size = sElementGetSize(element);

					for (int jj = 0; jj < file_array_length; jj++)
					{
						if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
						{
							unsigned int index = *(unsigned int *)((BYTE *)ptr + jj * element_size);
							if (ExcelIsLinkInvalid(index))
							{
								ASSERT_RETFALSE(TRANSFER_INT(bInvalid));
								continue;
							}
							const char * szIndex;
							if (element->m_eDataType == DATA_TYPE_INT_LINK)
							{
								szIndex = ExcelGetStringIndexByLine(EXCEL_CONTEXT(), (DWORD)element->m_nParam, index);
								if (!szIndex)
								{
									ExcelLog("EXCEL WARNING:  DEFINITION SAVE LINK INVALID:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(0)   INDEX: %d", 
										m_pszDefinitionName, element->m_pszVariableName, ExcelGetTableName(element->m_nParam), (int)index);
								}
							}
							else
							{
								szIndex = ExcelGetStringIndexByLine(EXCEL_CONTEXT(), (unsigned int)element->m_nParam, (unsigned int)element->m_nParam2, (unsigned int)index);
								if (!szIndex)
								{
									ExcelLog("EXCEL WARNING:  DEFINITION SAVE LINK INVALID:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(%d)   INDEX: %d", 
										m_pszDefinitionName, element->m_pszVariableName, ExcelGetTableName(element->m_nParam), element->m_nParam2, (int)index);
								}
							}
							
							int len = PStrLen(szIndex);
							ASSERT(len <= MAX_LINK_STRING_LENGTH);
							BYTE bLen = (BYTE)MIN(len, MAX_LINK_STRING_LENGTH);
							ASSERT_RETFALSE(TRANSFER_INT(bLen));
							ASSERT_RETFALSE(TRANSFER_BUF((void*)szIndex, bLen));
						}
						else
						{
							BYTE bLen = 0;
							ASSERT_RETFALSE(TRANSFER_INT(bLen));

							if (bLen == bInvalid)
							{
								*(int *)((BYTE *)ptr + jj * sizeof(int)) = INVALID_ID;
								continue;
							}

							char szIndex[MAX_LINK_STRING_LENGTH+1];
							ASSERT_RETFALSE( bLen <= MAX_LINK_STRING_LENGTH );
							ASSERT_RETFALSE(TRANSFER_BUF(szIndex, bLen * sizeof (char)));
							szIndex[bLen] = 0;
							int index;
							if (element->m_eDataType == DATA_TYPE_INT_LINK)
							{
								index = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), (DWORD)element->m_nParam, szIndex);
								if (szIndex[0] && index == EXCEL_LINK_INVALID)
								{
									ExcelLog("EXCEL WARNING:  DEFINITION LOAD KEY NOT FOUND:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(0)  KEY: %d", 
										m_pszDefinitionName, element->m_pszVariableName, ExcelGetTableName(element->m_nParam), szIndex);
								}
							}
							else
							{
								index = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), (unsigned int)element->m_nParam, (unsigned int)element->m_nParam2, szIndex);
								if (szIndex[0] && index == EXCEL_LINK_INVALID)
								{
									ExcelLog("EXCEL WARNING:  DEFINITION LOAD KEY NOT FOUND:  DEFINITION: %s   ELEMENT: %s   TABLE: %s(%d)  KEY: %d", 
										m_pszDefinitionName, element->m_pszVariableName, ExcelGetTableName(element->m_nParam), element->m_nParam2, szIndex);
								}
							}
							*(int *)((BYTE *)ptr + jj * sizeof(int)) = index;
						}
					}

					if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && skip_array_length)
					{
						// read past extraneous link data
						//
						for (int jj = 0; jj < skip_array_length; jj++)
						{
							BYTE bLen = 0;
							ASSERT_RETFALSE(TRANSFER_INT(bLen));
							if (bLen == bInvalid) continue;

							char * szJunkString = (char*)GET_TEMP_BUF(bLen * sizeof (char));
							ASSERT_RETFALSE(TRANSFER_BUF(szJunkString, bLen * sizeof (char)));
						}

						skip_array_length = 0;
					}
				}
				break;
			}
		}

		if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ)
		{
			if (skip_array_length)
			{
				// read past extraneous data
				//
				const int element_size = sElementGetSize(oldElement);
				const int iBufSize = skip_array_length * element_size;
				BYTE * bitBucket = (BYTE*)GET_TEMP_BUF(iBufSize);
				ASSERT_RETFALSE(TRANSFER_BUF(bitBucket, iBufSize));
			}

			if (fill_array_length)
			{
				FillWithDefaults(ptr, element, oldElement, file_array_length, new_array_length);
			}
		}

		if (bDeletePtr)
		{
			FREE(g_ScratchAllocator, ptr);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<typename T> BOOL CDefinitionLoader::TransferCookedPath(
										DEFINITION_TRANSFER_CONTEXT & dtc,
										T * path,
										CDefinitionLoader::CElement * element,
										int & file_array_length,
										int & skip_array_length)
{
	if (file_array_length)
	{
		if (dtc.nReadOrWrite == DEFINITION_TRANSFER_WRITE)
		{
			int count = path->GetPointCount();
			ASSERT_RETFALSE(TRANSFER_INT(count));
			for (int jj = 0; jj < count; jj++)
			{
				const T::CInterpolationPoint * point = path->GetPoint(jj);
				ASSERT_RETFALSE(dtc.writeBuf->PushBuf(point, sizeof(T::CInterpolationPoint)));
			}
		}
		else
		{
			path->Clear();
			float def = *(float *)&element->m_DefaultValue;
			path->SetDefault(def);

			int count = 0;
			ASSERT_RETFALSE(TRANSFER_INT(count));
			for (int jj = 0; jj < count; jj++)
			{
				T::CInterpolationPoint point;
				ASSERT_RETFALSE(dtc.readBuf->PopBuf(&point, sizeof(T::CInterpolationPoint)));
				path->AddPoint(point.m_tData, point.m_fTime);
			}
		}
	}

	// read past extraneous path data
	//
	if (dtc.nReadOrWrite == DEFINITION_TRANSFER_READ && skip_array_length)
	{
		int count = 0;
		ASSERT_RETFALSE(TRANSFER_INT(count));
		for (int jj = 0; jj < count; jj++)
		{
			T::CInterpolationPoint point;
			ASSERT_RETFALSE(dtc.readBuf->PopBuf(&point, sizeof(T::CInterpolationPoint)));
		}

		skip_array_length = 0;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL CDefinitionLoader::ElementIsDefault(
										void * ptr,
										const CDefinitionLoader::CElement * element)
{
	if (element->m_nArrayLength == -1)
	{
		return FALSE;
	}

	switch (element->m_eDataType)
	{
		case DATA_TYPE_BYTE:
		{
			const BYTE defaultValue = (BYTE)element->m_DefaultValue;

			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				if (*((BYTE*)ptr + ii) != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_INT:
		{
			const int defaultValue = (int)element->m_DefaultValue;
			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				if (*((int*)ptr + ii) != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_INT_64:
		{
			const unsigned __int64 defaultValue = element->m_DefaultValue64;
			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				if (*((unsigned __int64*)ptr + ii) != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_FLAG:
		{
			const int defaultValue = (int)element->m_DefaultValue * element->m_nParam;
			const int whichBit = element->m_nParam;

			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				const int bitVal = (*((int*)ptr + ii)) & whichBit;
				if (bitVal != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_FLAG_BIT:
		{
			const BOOL defaultValue = (BOOL)element->m_DefaultValue;
			const int maskSize = sElementGetSize(element);

			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				BYTE * bits = ((BYTE*)ptr + ii * maskSize);
				if (TESTBIT(bits, element->m_nParam) != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_FLOAT:
		{
			const float defaultValue = *(float*)&element->m_DefaultValue;

			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				if (*((float*)ptr + ii) != defaultValue)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		case DATA_TYPE_INT_NOSAVE:
		case DATA_TYPE_INT_DEFAULT:
		case DATA_TYPE_FLOAT_NOSAVE:
		case DATA_TYPE_POINTER_NOSAVE:
		{
			return TRUE;
		}
		case DATA_TYPE_PATH_FLOAT:
		{
			CInterpolationPath<float> * path = (CInterpolationPath<float> *)ptr;
			return path->IsEmpty();
		}
		case DATA_TYPE_PATH_FLOAT_PAIR:
		{
			CInterpolationPath<CFloatPair> * path = (CInterpolationPath<CFloatPair> *)ptr;
			return path->IsEmpty();
		}
		case DATA_TYPE_PATH_FLOAT_TRIPLE:
		{
			CInterpolationPath<CFloatTriple> * path = (CInterpolationPath<CFloatTriple> *)ptr;
			return path->IsEmpty();
		}
		case DATA_TYPE_STRING:
		{
			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				if (PStrCmp((char*)ptr + ii * sizeof(char) * MAX_XML_STRING_LENGTH, element->m_pszDefaultValue) != 0)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
		default:
		{
			return FALSE;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void CDefinitionLoader::FillWithDefaults(
										void * ptr,
										CDefinitionLoader::CElement * element,
										CDefinitionLoader::CElement * oldElement,
										const int iStart,
										const int iEnd)
{
	switch (element->m_eDataType)
	{
		case DATA_TYPE_BYTE:
		{
			const BYTE defaultValue = (BYTE)oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((BYTE*)ptr + ii) = defaultValue;
			}
			break;
		}
		case DATA_TYPE_INT:
		case DATA_TYPE_INT_DEFAULT:
		case DATA_TYPE_INT_NOSAVE:
		{
			const int defaultValue = (int)oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((int*)ptr + ii) = defaultValue;
			}
			break;
		}
		case DATA_TYPE_POINTER_NOSAVE:
		{
#if !ISVERSION(SERVER_VERSION)
			// CHB 2007.08.01 - Since the default value is ignored
			// (NULL is always used), an assertion should suffice here.
			ASSERTX(oldElement->m_DefaultValue == 0, "DATA_TYPE_POINTER_NOSAVE default value must be NULL.");
#endif
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((DWORD_PTR*)ptr + ii) = NULL;
			}
			break;
		}
		case DATA_TYPE_INT_LINK:
		case DATA_TYPE_INT_INDEX_LINK:
		{
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((int*)ptr + ii) = INVALID_ID;
			}
			break;
		}
		case DATA_TYPE_INT_64:
		{
			const unsigned __int64 defaultValue = oldElement->m_DefaultValue64;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((unsigned __int64*)ptr + ii) = defaultValue;
			}
			break;
		}
		case DATA_TYPE_FLAG:
		{
			const int defaultValue = (int)oldElement->m_DefaultValue * oldElement->m_nParam;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((int*)ptr + ii) |= defaultValue;
			}
			break;
		}
		case DATA_TYPE_FLAG_BIT:
		{
			const BOOL defaultValue = (BOOL)element->m_DefaultValue;
			const int maskSize = sElementGetSize(element);

			for (int ii=0; ii < element->m_nArrayLength; ++ii)
			{
				BYTE * bits = ((BYTE*)ptr + ii * maskSize);
				if (defaultValue)
				{
					SETBIT(bits, element->m_nParam);
				}
				else
				{
					CLEARBIT(bits, element->m_nParam);
				}
			}
			break;
		}
		case DATA_TYPE_FLOAT:
		case DATA_TYPE_FLOAT_NOSAVE:
		{
			const float defaultValue = *(float *)&oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				*((float*)ptr + ii) = defaultValue;
			}
			break;
		}
		case DATA_TYPE_PATH_FLOAT:
		{
			const float defaultValue = *(float *)&oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				CInterpolationPath<float> * path = (CInterpolationPath<float> *)ptr + ii;
				path->Clear();
				path->SetDefault(defaultValue);
			}
			break;
		}
		case DATA_TYPE_PATH_FLOAT_PAIR:
		{
			const float defaultValue = *(float *)&oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				CInterpolationPath<CFloatPair> * path = (CInterpolationPath<CFloatPair> *)ptr + ii;
				path->Clear();
				path->SetDefault(defaultValue);
			}
			break;
		}
		case DATA_TYPE_PATH_FLOAT_TRIPLE:
		{
			const float defaultValue = *(float *)&oldElement->m_DefaultValue;
			for (int ii = iStart; ii < iEnd; ii++)
			{
				CInterpolationPath<CFloatTriple> * path = (CInterpolationPath<CFloatTriple> *)ptr + ii;
				path->Clear();
				path->SetDefault(defaultValue);
			}
			break;
		}
		case DATA_TYPE_STRING:
		{
			for (int ii = iStart; ii < iEnd; ii++)
			{
				PStrCopy((char *)ptr + ii * MAX_XML_STRING_LENGTH, (oldElement->m_pszDefaultValue ? oldElement->m_pszDefaultValue : ""), MAX_XML_STRING_LENGTH);
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDefinitionLoader::ProcessFile( 
	FILE_REQUEST_DATA & tData,
	PAKFILE_LOAD_SPEC & tSpec,
	BOOL bLoadingCooked,
	BOOL bForceAddToPack )
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( tData.eGroup );
	ASSERT_RETFALSE( pContainer );
	void *pTarget = pContainer->GetById( tData.nDefinitionId, FALSE );
	ASSERTX( pTarget == NULL, "Definition already loaded" );
	
	// get new definition instance to load into
	pTarget = pContainer->GetById( tData.nDefinitionId, TRUE );
	ASSERT_RETFALSE( pTarget );

	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER *)pTarget;
	PStrCopy(pHeader->pszName, tData.pszDefinitionName, MAX_XML_STRING_LENGTH);

	// if we loaded from pak file, it must be cooked data
	if (tSpec.frompak == TRUE && (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) || AppIsPatching() == FALSE || AppGetPatchChannel() == NULL))
	{
		ASSERT_RETFALSE( bLoadingCooked == TRUE );
	}
	
	if (bLoadingCooked && tSpec.buffer)
	{
		BYTE_BUF buf( tSpec.buffer, tSpec.bytesread );
		if (!LoadCooked(pTarget, buf, tData.bZeroOutData))
		{// it didn't work to load from the pak, load directly
			// tData.pszFullFileName is the .xml, tSpec.pszFilename contains the .cooked file
			ASSERT( PStrStrI( tSpec.filename, DEFINITION_COOKED_EXTENSION ) != NULL );

			char szOriginalFileName[ MAX_PATH ];
			PStrRemoveExtension( szOriginalFileName, MAX_PATH, tSpec.filename );
			
			FREE(tSpec.pool, tSpec.buffer);
			tData.bForceLoadDirect = TRUE;
			tData.pszFullFileName = szOriginalFileName;
			//tData.bAsyncLoad = bOriginallyLoadedAsync;
			return RequestFile( tData );
		}
		ASSERT(buf.GetRemainder() == 0);
	} 
	else 
	{
		CMarkup tMarkup;
		if (tMarkup.SetDoc((char*)tSpec.buffer, tData.pszDefinitionName))
		{
		}
		Load(pTarget, &tMarkup, tData.bZeroOutData, TRUE);
	}

	// if dealing with binary/pak assets (ie the game, Hammer does not)	
	if (AppCommonUsePakFile() && (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) || AppIsPatching() == FALSE || AppGetPatchChannel() == NULL))
	{
		// if we really got a file and we might consider putting it into the pak
		if (tSpec.buffer != NULL && !(tSpec.flags & PAKFILE_LOAD_NEVER_PUT_IN_PAK))
		{
			// if we are to force it to add to the pak or it wasn't loaded in cooked form
			// or it wasn't loaded from the pak and it wasn't added to the pak already by the pakfile system
			// then we will cook the data down and save it to the pak
			if (bForceAddToPack == TRUE || bLoadingCooked == FALSE ||
				(tSpec.frompak == FALSE && !(tSpec.flags & PAKFILE_LOAD_ADD_TO_PAK)))
			{
				WRITE_BUF_DYNAMIC bbuf(NULL);

				// setup cooked filename (note that if we loaded cooked data that the filename
				// already refers to the cooked data
				char szFullFilenameCooked[MAX_PATH];
				if (bLoadingCooked == TRUE)
				{
					PStrCopy(szFullFilenameCooked, tSpec.fixedfilename, MAX_PATH);  // use filename as is
				}
				else
				{
					PStrPrintf(szFullFilenameCooked, MAX_PATH, "%s.%s", tData.pszFullFileName, DEFINITION_COOKED_EXTENSION);  // generate cooked filename

					// Clear out the id field when saving
					int tempId = pHeader->nId;
					pHeader->nId = 0;
					SaveCooked(pTarget, bbuf);
					pHeader->nId = tempId;

					BOOL bCanSave = TRUE;
					if ( FileIsReadOnly( szFullFilenameCooked ) )
						bCanSave = DataFileCheck( szFullFilenameCooked );
					
					if ( bCanSave )
						bbuf.Save(szFullFilenameCooked, NULL, &tSpec.gentime);
				}

				// history debugging
				FileDebugCreateAndCompareHistory(szFullFilenameCooked);
				
				DECLARE_LOAD_SPEC(specCooked, szFullFilenameCooked);
				specCooked.pool = g_ScratchAllocator;
				specCooked.buffer = (void *)bbuf.GetBuf();
				specCooked.size = bbuf.GetPos();
				specCooked.gentime = tSpec.gentime;
				specCooked.pakEnum = tSpec.pakEnum;
				PakFileAddFile(specCooked);
			}			
		}
	}

	// note that if we were loading cooked and it was good, then we've already returned by this point
	if (tSpec.buffer)
	{
		FREE(g_ScratchAllocator, tSpec.buffer);
		tSpec.buffer = NULL;
		SETBIT(pHeader->dwFlags, DHF_EXISTS_BIT, TRUE);
	}
	else
	{
		SETBIT(pHeader->dwFlags, DHF_EXISTS_BIT, FALSE);
	}

	SETBIT(pHeader->dwFlags, DHF_LOADED_BIT);

	BOOL bCallOtherPostLoads = TRUE;
	if (m_fnPostLoad)
	{
		bCallOtherPostLoads = m_fnPostLoad(pTarget, !tData.bAsyncLoad);
	}

	if (m_pCallbacks && bCallOtherPostLoads)
	{
		CallAllPostLoadCallbacks( tData.nDefinitionId, pTarget );
	}

	pContainer->Restore( pTarget );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::CallAllPostLoadCallbacks(
	int nDefinitionId,
	void * pDefinition ) 
{ 
	ASSERT_RETURN( pDefinition );
	if ( m_pCallbacks )
	{
		DefinitionAsynchLoadCallback * pCallback = InOrderHashGet(*m_pCallbacks, nDefinitionId);
		while(pCallback)
		{
			(*pCallback->m_fnPostLoad)(pDefinition, pCallback->m_pEventData);
			if ( pCallback->m_pEventData )
				FREE(g_ScratchAllocator, pCallback->m_pEventData);

			pCallback = InOrderHashGetNext(*m_pCallbacks, pCallback, nDefinitionId);
		}
		InOrderHashRemove(*m_pCallbacks, nDefinitionId);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct DEFINITION_LOAD_CALLBACK_DATA
{
	CDefinitionLoader * pLoader;
	DEFINITION_GROUP_TYPE eGroup;
	int nDefinitionId;
	char pszDefinitionName[ MAX_XML_STRING_LENGTH ];
	BOOL bZeroOutData;
	BOOL bNeverPutInPak;
	BOOL bWarnWhenMissing;
	BOOL bAlwaysWarnWhenMissing;
	BOOL bNodirectIgnoreMissing;
	BOOL bForceAddToPack;
	BOOL bLoadingCooked;
	BOOL bAsyncLoad;
};


//----------------------------------------------------------------------------
static PRESULT sFileLoadedCallback( 
	ASYNC_DATA & data) 
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	DEFINITION_LOAD_CALLBACK_DATA * callbackData = (DEFINITION_LOAD_CALLBACK_DATA *)spec->callbackData;
	ASSERT_RETFAIL(callbackData);

	FILE_REQUEST_DATA requestData;
	requestData.eGroup = callbackData->eGroup; 
	requestData.pszFullFileName = spec->fixedfilename;
	requestData.pszDefinitionName = callbackData->pszDefinitionName; 
	requestData.pszPathName = NULL;
	requestData.nDefinitionId = callbackData->nDefinitionId;
	requestData.bForceLoadDirect = FALSE;
	requestData.bNeverPutInPak = callbackData->bNeverPutInPak;
	requestData.bZeroOutData = callbackData->bZeroOutData;
	requestData.bWarnWhenMissing = callbackData->bWarnWhenMissing;
	requestData.bAlwaysWarnWhenMissing = callbackData->bAlwaysWarnWhenMissing;
	requestData.bNodirectIgnoreMissing = callbackData->bNodirectIgnoreMissing;
	requestData.bAsyncLoad = callbackData->bAsyncLoad;
	requestData.nFileLoadPriority = spec->priority;
	requestData.ePakfile = spec->pakEnum;

	// see if this definition has already completed a sync load while the async load was in progress
	CDefinitionContainer * container = CDefinitionContainer::GetContainerByType(requestData.eGroup);
	ASSERT_RETFAIL(container);
	DEFINITION_HEADER * header = (DEFINITION_HEADER *)container->GetById(requestData.nDefinitionId, TRUE);
	ASSERT_RETFAIL(header);
	if (!TESTBIT(header->dwFlags, DHF_LOADED_BIT))
	{
		callbackData->pLoader->ProcessFile(requestData, *spec, callbackData->bLoadingCooked, callbackData->bForceAddToPack);
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDefinitionNeedsUpdate(
	const char *pszFilenameCooked,
	const char *pszFilenameSource,
	BOOL bAlwaysCheckDirect, 
	BOOL bOptionalFile, 
	DWORD dwMagic,
	DWORD dwVersion)
{
	// see if the source xml is newer than the cooked file
	BOOL bInvalidSourceFile = FALSE;

	DWORD flags = 0; // FILE_UPDATE_UPDATE_IF_NOT_IN_PAK -- why would we need this?
	if (bAlwaysCheckDirect)
	{
		flags |= FILE_UPDATE_ALWAYS_CHECK_DIRECT;
	}

	if ( bOptionalFile )
	{
		flags |= FILE_UPDATE_OPTIONAL_FILE;
	}

	if ( AppCommonAllowAssetUpdate() && FileNeedsUpdate(pszFilenameCooked, pszFilenameSource, dwMagic, dwVersion, flags))
	{
		return TRUE;
	}

	// if file does not exist we will need an update
	if (bInvalidSourceFile == TRUE)
	{
		return TRUE;
	}

	return FALSE;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDefinitionLoader::RequestFile( 
	FILE_REQUEST_DATA & tData )
{
	ASSERT_RETFALSE( tData.pszDefinitionName && *tData.pszDefinitionName);

	// when not using pak files (like hammer) always force a load direct
	if (AppCommonUsePakFile() == FALSE)
	{
		tData.bForceLoadDirect = TRUE;
	}

	char pszNameBuffer[DEFAULT_FILE_WITH_PATH_SIZE];
	if ( ! tData.pszFullFileName )
	{
		if (tData.pszPathName[0] != 0)
		{
			PStrPrintf(pszNameBuffer, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", tData.pszPathName, tData.pszDefinitionName);
		}
		else
		{
			PStrCopy(pszNameBuffer, tData.pszDefinitionName, DEFAULT_FILE_WITH_PATH_SIZE);
		}
		PStrReplaceExtension(pszNameBuffer, DEFAULT_FILE_WITH_PATH_SIZE, pszNameBuffer, DEFINITION_FILE_EXTENSION);
		tData.pszFullFileName = pszNameBuffer;
	}

#if _DEBUG
	const char *szString = DEFINITION_COOKED_EXTENSION;
	ASSERTX( PStrStrI( tData.pszFullFileName, szString ) == NULL, "Request is already for cooked file" )
#endif

	// get filename for cooked data
	char szFullFileNameCooked[ MAX_PATH ];
	PStrPrintf( szFullFileNameCooked, MAX_PATH, "%s.%s", tData.pszFullFileName, DEFINITION_COOKED_EXTENSION );

	// is any part of the definition on disk more up to date than the one in the pak file
	ComputeVersion();

	FILE_HEADER hdr;
	hdr.dwMagicNumber = COOKED_MAGIC_NUMBER;	//was m_dwMagicNumber
	hdr.nVersion = DEFINITION_FILE_VERSION;		//was m_dwCRC

	BOOL bNeedsUpdate = sDefinitionNeedsUpdate(szFullFileNameCooked, tData.pszFullFileName, m_bAlwaysCheckDirect, tData.bForceIgnoreWarnIfMissing, hdr.dwMagicNumber, hdr.nVersion);
	
	// pak will also need update if we're forcing a direct load
	if (tData.bForceLoadDirect == TRUE )
	{
		bNeedsUpdate = TRUE;
	}
	
#if ISVERSION(DEVELOPMENT)
	PAKFILE_LOAD_SPEC spec(szFullFileNameCooked, m_pszDefinitionName, __LINE__);
#else
	DECLARE_LOAD_SPEC(spec, szFullFileNameCooked);
#endif

	spec.pool = g_ScratchAllocator;

	// setup load flags
	if (bNeedsUpdate)
	{
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	}
	if (tData.bWarnWhenMissing)
	{
		spec.flags |= PAKFILE_LOAD_WARN_IF_MISSING;
	}
	if (tData.bNodirectIgnoreMissing)
	{
		spec.flags |= PAKFILE_LOAD_NODIRECT_IGNORE_MISSING;
	}
	if (tData.bForceIgnoreWarnIfMissing)
	{
		spec.flags |= PAKFILE_LOAD_OPTIONAL_FILE;
	}

	// if we don't need to update we'll load the cooked one otherwise we'll load the raw data
	const char *pszFileToLoad = szFullFileNameCooked;
	BOOL bLoadingCooked = TRUE;	
	if ( bNeedsUpdate == TRUE && (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) || AppIsPatching() == FALSE || AppGetPatchChannel() == NULL))
	{
		// load the original xml
		pszFileToLoad = tData.pszFullFileName;
		
#if _DEBUG
		ASSERT(PStrStrI(pszFileToLoad, DEFINITION_COOKED_EXTENSION) == 0);
#endif

		spec.filename = pszFileToLoad;
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;

		// we are not loaded cooked assets
		bLoadingCooked = FALSE;

		if ( AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) )
			tData.bAlwaysWarnWhenMissing = FALSE;
	}
	if (bLoadingCooked)
	{
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	}
	if (tData.bAlwaysWarnWhenMissing)
	{
		spec.flags |= PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING;
	}

	DEFINITION_LOAD_CALLBACK_DATA * pCallbackData = (DEFINITION_LOAD_CALLBACK_DATA *) MALLOC( g_ScratchAllocator, sizeof( DEFINITION_LOAD_CALLBACK_DATA ));
	pCallbackData->eGroup					= tData.eGroup;
	pCallbackData->bForceAddToPack			= bNeedsUpdate;
	pCallbackData->bNeverPutInPak			= tData.bNeverPutInPak;
	pCallbackData->bWarnWhenMissing			= tData.bWarnWhenMissing;
	pCallbackData->bAlwaysWarnWhenMissing	= bLoadingCooked ? FALSE : tData.bAlwaysWarnWhenMissing;
	pCallbackData->bNodirectIgnoreMissing	= tData.bNodirectIgnoreMissing;
	pCallbackData->bZeroOutData				= tData.bZeroOutData;
	pCallbackData->nDefinitionId			= tData.nDefinitionId;
	pCallbackData->pLoader					= this;
	pCallbackData->bLoadingCooked			= bLoadingCooked;
	pCallbackData->bAsyncLoad				= tData.bAsyncLoad;
	PStrCopy(pCallbackData->pszDefinitionName, tData.pszDefinitionName, MAX_XML_STRING_LENGTH);

	// assign async load data
	spec.fpLoadingThreadCallback = sFileLoadedCallback;
	spec.callbackData = pCallbackData;
	spec.priority = tData.nFileLoadPriority;
	spec.pakEnum = tData.ePakfile;
	spec.flags |= PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_CALLBACK_IF_NOT_FOUND;
	if (tData.bNeverPutInPak)
	{
		spec.flags |= PAKFILE_LOAD_NEVER_PUT_IN_PAK;
	}

	// do async load or normal load
	if (tData.bAsyncLoad)
	{
		// request load
		PakFileLoad(spec);
	} 
	else 
	{
		// make file request now
		PakFileLoadNow(spec);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::Save(
	void * pSource,
	CMarkup * pMarkup)
{
	ComputeVersion();

	ASSERT_RETURN(m_pszDefinitionName);
	pMarkup->AddElem(m_pszDefinitionName);

	void * pSourceCurrent = pSource;
	for (int nIndex = 0; m_pElements[nIndex].m_pszVariableName && m_pElements[nIndex].m_pszVariableName[0] != 0; nIndex++)
	{
		const CDefinitionLoader::CElement * pElement = &m_pElements [nIndex];
		


		// handle variable arrays
		int nArrayLength = pElement->m_nArrayLength;
		if (nArrayLength == -1)
		{
			nArrayLength = *(int *)((BYTE *)pSource + pElement->m_nVariableArrayOffset);
			pSourceCurrent = *(void **)((BYTE *)pSource + pElement->m_nOffset);
			if( ElementIsDefault( pSourceCurrent, pElement ) )
			{
				continue;
			}
			// save the length of this array - we could count them on load, but it is slower
			CString sValue;
			sValue.Format("%d", nArrayLength);
			CString sCountName;
			sCountName.Format("%s%s", pElement->m_pszVariableName, VARRAY_SUFFIX);
			pMarkup->AddChildElem(sCountName, sValue);
		}
		else
		{
			pSourceCurrent = (BYTE *)pSource + pElement->m_nOffset;
			if( ElementIsDefault( pSourceCurrent, pElement ) )
			{
				continue;
			}
		}

		// save the elements - for both arrays and non-arrays
		for (int nArrayEntry = 0; pSourceCurrent && nArrayEntry < nArrayLength; nArrayEntry++)
		{
			// handle the single value cases
			BOOL bAddValue = TRUE;
			CString sValue;
			switch (pElement->m_eDataType)
			{
			case DATA_TYPE_BYTE:
				sValue.Format("%d", *(BYTE*)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(BYTE)));
				break;
			case DATA_TYPE_INT:
				sValue.Format("%d", *(int *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(int)));
				break;
			case DATA_TYPE_INT_64:
				{
					unsigned __int64 i64 = *(unsigned __int64 *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(unsigned __int64));
					sValue.Format("%I64u", i64);
				}
				break;
			case DATA_TYPE_INT_NOSAVE:
			case DATA_TYPE_INT_DEFAULT:
			case DATA_TYPE_POINTER_NOSAVE:
				bAddValue = FALSE;
				break;
			case DATA_TYPE_FLOAT:
				sValue.Format("%g", *(float *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(float)));
				break;
			case DATA_TYPE_FLOAT_NOSAVE:
				bAddValue = FALSE;
				break;
			case DATA_TYPE_STRING:
				sValue = (char *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(char) * MAX_XML_STRING_LENGTH);
				break;
			case DATA_TYPE_FLAG:
				sValue = ((*(int *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(int))) & pElement->m_nParam) != 0 ? "1" : "0";
				break;
			case DATA_TYPE_FLAG_BIT:
				{
					int nSize = sElementGetSize( pElement );
					sValue = TESTBIT((BYTE *)((BYTE *)pSourceCurrent + nArrayEntry * nSize), pElement->m_nParam) ? "1" : "0";
				}
				break;
			case DATA_TYPE_INT_LINK:
				{
					int nLine = *(int *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(int));
					if (nLine != INVALID_ID)
					{
						const char * str = ExcelGetStringIndexByLine(EXCEL_CONTEXT(), (DWORD)pElement->m_nParam, nLine);
						if (str == NULL)
						{
							ExcelLog("EXCEL WARNING:  DEFINITION LOAD INVALID LINK:  DEFINITION: %s   ELEMENT: %s   TABLE: %s (0)   LINK: %d", 
								m_pszDefinitionName, pElement->m_pszVariableName, ExcelGetTableName(pElement->m_nParam), nLine);
						}
						sValue = str;
					}
					else
					{
						sValue = "";
					}
				}
				break;
			case DATA_TYPE_INT_INDEX_LINK:
				{
					int nLine = *(int *)((BYTE *)pSourceCurrent + nArrayEntry * sizeof(int));
					if (nLine != INVALID_ID)
					{
						const char * str = ExcelGetStringIndexByLine(EXCEL_CONTEXT(), (unsigned int)pElement->m_nParam, (unsigned int)pElement->m_nParam2, (unsigned int)nLine);
						if (str == NULL)
						{
							ExcelLog("EXCEL WARNING:  DEFINITION LOAD INVALID LINK:  DEFINITION: %s   ELEMENT: %s   TABLE: %s (%d)   LINK: %d", 
								m_pszDefinitionName, pElement->m_pszVariableName, ExcelGetTableName(pElement->m_nParam), pElement->m_nParam2, nLine);
						}
						sValue = str;
					}
					else
					{
						sValue = "";
					}
				}
				break;
			}
			if (bAddValue)
			{
				pMarkup->AddChildElem(pElement->m_pszVariableName, sValue);
			}

			// handle the complex cases
			switch (pElement->m_eDataType)
			{
			case DATA_TYPE_PATH_FLOAT_PAIR:
				pMarkup->IntoElem();
				SavePath<CFloatPair>(pSourceCurrent, pMarkup);
				pMarkup->OutOfElem();
				break;

			case DATA_TYPE_PATH_FLOAT_TRIPLE:
				pMarkup->IntoElem();
				SavePath<CFloatTriple>(pSourceCurrent, pMarkup);
				pMarkup->OutOfElem();
				break;

			case DATA_TYPE_DEFINITION:
				pMarkup->IntoElem();
				ASSERT(pElement->m_pDefinitionLoader != NULL);
				pElement->m_pDefinitionLoader->Save((BYTE *)pSourceCurrent + nArrayEntry * pElement->m_pDefinitionLoader->m_nStructSize, pMarkup);
				pMarkup->OutOfElem();
				break;
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define __WTEXT(x) L##x
#define WTEXT(x) __WTEXT(x)
BOOL CDefinitionLoader::SaveFile(
		void * pSource,
		const WCHAR * pszFilePath,
		BOOL bIgnoreFilePath)
{
	CMarkup tMarkup;
	Save(pSource, &tMarkup);

	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER *)pSource;

	WCHAR szHeaderName[_countof(pHeader->pszName)];
	PStrCvt(szHeaderName, pHeader->pszName, _countof(szHeaderName));

	WCHAR pszFileName[MAX_XML_STRING_LENGTH];
	if (pszFilePath[0] != 0 && !bIgnoreFilePath)
	{
		PStrPrintf(pszFileName, _countof(pszFileName), L"%s%s", pszFilePath, szHeaderName);
	}
	else
	{
		PStrCopy(pszFileName, szHeaderName, _countof(pszFileName));
	}
	PStrReplaceExtension(pszFileName, _countof(pszFileName), pszFileName, WTEXT(DEFINITION_FILE_EXTENSION));

	return tMarkup.Save(pszFileName);
}

BOOL CDefinitionLoader::SaveFile(
		void * pSource,
		const char * pszFilePath,
		BOOL bIgnoreFilePath)
{
#if 1
	WCHAR szName[MAX_XML_STRING_LENGTH];
	PStrCvt(szName, pszFilePath, _countof(szName));
	return SaveFile(pSource, szName, bIgnoreFilePath);
#else
	CMarkup tMarkup;
	Save(pSource, &tMarkup);

	char pszFileName[MAX_XML_STRING_LENGTH];
	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER *)pSource;
	if (pszFilePath[0] != 0 && !bIgnoreFilePath)
	{
		PStrPrintf(pszFileName, MAX_XML_STRING_LENGTH, "%s%s", pszFilePath, pHeader->pszName);
	}
	else
	{
		PStrCopy(pszFileName, pHeader->pszName, MAX_XML_STRING_LENGTH);
	}
	PStrReplaceExtension(pszFileName, MAX_XML_STRING_LENGTH, pszFileName, DEFINITION_FILE_EXTENSION);

	return tMarkup.Save(pszFileName);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TDataType>
void sLoadPathPoint(
	TDataType * pValue,
	CMarkup * pMarkup);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLoadPathPoint(
	float * pValue,
	CMarkup * pMarkup)
{
	BOOL bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	*pValue = (float)PStrToFloat(pMarkup->GetChildData());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLoadPathPoint(
	CFloatPair * pValue,
	CMarkup * pMarkup)
{
	BOOL bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	pValue->fX = (float)PStrToFloat(pMarkup->GetChildData());

	bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	pValue->fY = (float)PStrToFloat(pMarkup->GetChildData());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLoadPathPoint(
	CFloatTriple * pValue,
	CMarkup * pMarkup)
{
	BOOL bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	pValue->fX = (float)PStrToFloat(pMarkup->GetChildData());

	bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	pValue->fY = (float)PStrToFloat(pMarkup->GetChildData());

	bFound = pMarkup->FindChildElem(PATH_VALUE_STRING);
	ASSERT(bFound);
	pValue->fZ = (float)PStrToFloat(pMarkup->GetChildData());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TDataType>
void CDefinitionLoader::LoadPath(
	void * pTarget,
	CMarkup * pMarkup,
	const char * pszDefault)
{
	// get the path and clear it
	CInterpolationPath<TDataType> * pPath = (CInterpolationPath<TDataType> *)pTarget;
	pPath->Clear();

	// get the count of elements in the path
	BOOL bFound = pMarkup->FindChildElem(PATH_COUNT_STRING);
	int nPointCount = bFound ? PStrToInt(pMarkup->GetChildData()) : 0;

	// set the default
	float fValue = (float)PStrToFloat(pszDefault);
	pPath->SetDefault(fValue);

	// add the points to the interpolation path
	for (int nPoint = 0; nPoint < nPointCount; nPoint++)
	{
		bFound = pMarkup->FindChildElem(PATH_TIME_STRING);
		ASSERT(bFound);
		float fTime = (float)PStrToFloat(pMarkup->GetChildData());

		TDataType tValue;
		sLoadPathPoint<TDataType>(&tValue, pMarkup);

		pPath->AddPoint(tValue, fTime);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class TDataType>
void sSavePathPoint(
	const TDataType * pValue,
	CMarkup * pMarkup);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSavePathPoint(
	const float * pValue,
	CMarkup * pMarkup)
{
	CString sValue;
	sValue.Format("%g", *pValue);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSavePathPoint(
	const CFloatPair * pValue,
	CMarkup * pMarkup)
{
	CString sValue;
	sValue.Format("%g", pValue->fX);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);

	sValue.Format("%g", pValue->fY);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSavePathPoint (
	const CFloatTriple * pValue,
	CMarkup * pMarkup)
{
	CString sValue;
	sValue.Format("%g", pValue->fX);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);

	sValue.Format("%g", pValue->fY);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);

	sValue.Format("%g", pValue->fZ);
	pMarkup->AddChildElem(PATH_VALUE_STRING, sValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class TDataType>
void CDefinitionLoader::SavePath(
	void * pSource,
	CMarkup * pMarkup)
{
	// get the path and clear it
	CInterpolationPath<TDataType> * pPath = (CInterpolationPath<TDataType> *)pSource;

	// get the count of elements in the path
	CString sValue;
	sValue.Format("%d", pPath->GetPointCount());
	pMarkup->AddChildElem(PATH_COUNT_STRING, sValue);

	// add the points to the interpolation path
	for (int nPointIndex = 0; nPointIndex < pPath->GetPointCount(); nPointIndex ++)
	{
		const CInterpolationPath<TDataType>::CInterpolationPoint * pPoint = pPath->GetPoint(nPointIndex);

		sValue.Format("%g", pPoint->m_fTime);
		pMarkup->AddChildElem(PATH_TIME_STRING, sValue);

		sSavePathPoint<TDataType>(&pPoint->m_tData, pMarkup);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::InitValues(
	void * pTarget,
	BOOL bZeroOutData)
{
	if (bZeroOutData)
	{
		memclear(pTarget, m_nStructSize);
	} 
	else 
	{
		for (int nIndex = 0; m_pElements[nIndex].m_pszVariableName && m_pElements[nIndex].m_pszVariableName[0] != 0; nIndex++)
		{
			const CDefinitionLoader::CElement * pElement = &m_pElements[nIndex];
			if (pElement->m_nArrayLength == -1)
			{
				*(POINTER_AS_INT *)((BYTE *)pTarget + pElement->m_nVariableArrayOffset) = 0;
				*(POINTER_AS_INT *)((BYTE *)pTarget + pElement->m_nOffset) = 0;
			}
			switch (pElement->m_eDataType)
			{
			case DATA_TYPE_PATH_FLOAT:
				{
					CInterpolationPath<float> * pTargetPath = (CInterpolationPath<float> *)((BYTE *)pTarget + pElement->m_nOffset);
					CInterpolationPathInit(*pTargetPath);
				}
				break;
			case DATA_TYPE_PATH_FLOAT_PAIR:
				{
					CInterpolationPath<CFloatPair> * pTargetPath = (CInterpolationPath<CFloatPair> *)((BYTE *)pTarget + pElement->m_nOffset);
					CInterpolationPathInit(*pTargetPath);
				}
				break;
			case DATA_TYPE_PATH_FLOAT_TRIPLE:
				{
					CInterpolationPath<CFloatTriple> * pTargetPath = (CInterpolationPath<CFloatTriple> *)((BYTE *)pTarget + pElement->m_nOffset);
					CInterpolationPathInit(*pTargetPath);
				}
				break;
			}
		}
	}

	InitElementData();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2006.09.29
#if 0
void CDefinitionLoader::InitValues2(
	void * pTarget,
	BOOL bZeroOutData)
{
	if (bZeroOutData)
	{
		memclear(pTarget, m_nStructSize);
	} 
//	else 
	{
		for (int nIndex = 0; m_pElements[nIndex].m_pszVariableName && m_pElements[nIndex].m_pszVariableName[0] != 0; nIndex++)
		{
			const CDefinitionLoader::CElement * pElement = &m_pElements[nIndex];

			int nArrayLength = pElement->m_nArrayLength;

			if (nArrayLength == -1)
			{
				*(POINTER_AS_INT *)((BYTE *)pTarget + pElement->m_nVariableArrayOffset) = 0;
				*(POINTER_AS_INT *)((BYTE *)pTarget + pElement->m_nOffset) = 0;
				nArrayLength = 0;
			}

			int nElementSize = sElementGetSize(pElement);

			for (int nElement = 0; nElement < nArrayLength; ++nElement)
			{
				void * pData = (BYTE *)pTarget + pElement->m_nOffset + nElement * nElementSize;

				switch (pElement->m_eDataType)
				{
				case DATA_TYPE_FLOAT:
					C_ASSERT(sizeof(float) == sizeof(int));
					// Fall through...
				case DATA_TYPE_INT:
				case DATA_TYPE_INT_DEFAULT:
				case DATA_TYPE_INT_NOSAVE:
				case DATA_TYPE_INT_LINK:
				case DATA_TYPE_INT_INDEX_LINK:
					*static_cast<int *>(pData) = *static_cast<const int *>(static_cast<const void *>(&pElement->m_DefaultValue));
					break;

				case DATA_TYPE_STRING:
					*static_cast<char *>(pData) = '\0';
					break;

				case DATA_TYPE_PATH_FLOAT:
					new(pData) CInterpolationPath<float>;
					break;
				case DATA_TYPE_PATH_FLOAT_PAIR:
					new(pData) CInterpolationPath<CFloatPair>;
					break;
				case DATA_TYPE_PATH_FLOAT_TRIPLE:
					new(pData) CInterpolationPath<CFloatTriple>;
					break;
				}
			}
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::InitChildMemPools(
	struct MEMORYPOOL * pMemPool)
{
	for(unsigned int i=0; i<MAX_ELEMENTS_PER_DEFINITION; i++)
	{
		if(m_pElements[i].m_pDefinitionLoader && !m_pElements[i].m_pDefinitionLoader->m_pMemPool && m_pElements[i].m_pDefinitionLoader != this)
		{
			m_pElements[i].m_pDefinitionLoader->m_pMemPool = pMemPool;
			m_pElements[i].m_pDefinitionLoader->InitChildMemPools(pMemPool);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::Init(
	struct MEMORYPOOL * pMemPool)
{
	m_pCallbacks = (CInOrderHash<DefinitionAsynchLoadCallback>*)MALLOCZ(NULL, sizeof(CInOrderHash<DefinitionAsynchLoadCallback>));
	InOrderHashInit(*m_pCallbacks, NULL, 64);
	m_pMemPool = pMemPool;
	if(pMemPool)
	{
		InitChildMemPools(pMemPool);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::ShutDown()
{
	for ( DefinitionAsynchLoadCallback * pCallback = InOrderHashGetFirst(*m_pCallbacks);
		pCallback;
		pCallback = InOrderHashGetNextInHash(*m_pCallbacks,pCallback) )
	{
		if ( pCallback->m_pEventData )
			FREE( g_ScratchAllocator, pCallback->m_pEventData );
	}
	InOrderHashFree(*m_pCallbacks);
	FREE(g_ScratchAllocator, m_pCallbacks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::AddPostLoadCallback(
	int nId,
	PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad,
	EVENT_DATA * pEventData)
{
	DefinitionAsynchLoadCallback * pCallback = InOrderHashAdd(*m_pCallbacks, nId);
	pCallback->m_fnPostLoad = fnPostLoad;
	if ( pEventData )
	{
		pCallback->m_pEventData = (EVENT_DATA*)MALLOCZ(g_ScratchAllocator, sizeof(EVENT_DATA));
		memcpy(pCallback->m_pEventData, pEventData, sizeof(EVENT_DATA));
	} else {
		pCallback->m_pEventData = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CDefinitionLoader::Copy(
	void * pTarget,
	void * pSource,
	BOOL bZeroOutData)
{
	ComputeVersion();

	if (m_bHeader)
	{
		DEFINITION_HEADER * pTargetHeader = (DEFINITION_HEADER *)pTarget;
		DEFINITION_HEADER * pSourceHeader = (DEFINITION_HEADER *)pSource;
		PStrCopy(pTargetHeader->pszName, pSourceHeader->pszName, MAX_XML_STRING_LENGTH);
		pTargetHeader->nId = pSourceHeader->nId;
		pTargetHeader->dwFlags = pSourceHeader->dwFlags;
	}

	for (int nIndex = 0; m_pElements[nIndex].m_pszVariableName && m_pElements[nIndex].m_pszVariableName[0] != 0; nIndex++)
	{
		const CElement * pElement = &m_pElements[nIndex];
		void * pTargetCurrent = (BYTE *)pTarget + pElement->m_nOffset;
		void * pSourceCurrent = (BYTE *)pSource + pElement->m_nOffset;

		int nArrayLength = pElement->m_nArrayLength;
		if (nArrayLength == -1)
		{
			// get and copy the length
			int nOldLength = *(int *)((BYTE *)pTarget + pElement->m_nVariableArrayOffset);
			nArrayLength = *(int *)((BYTE *)pSource + pElement->m_nVariableArrayOffset);
			*(int *)((BYTE *)pTarget + pElement->m_nVariableArrayOffset) = nArrayLength;

			// allocate the memory
			if (nArrayLength > 0)
			{
				int nArraySize = 0;
				int nElementSize = sElementGetSize(pElement);
				nArraySize = nArrayLength * nElementSize;
				*(void **)pTargetCurrent = REALLOC(m_pMemPool, *(void **)pTargetCurrent, nArraySize);

				if (pElement->m_pDefinitionLoader)
				{
					ASSERT(pElement->m_eDataType == DATA_TYPE_DEFINITION);	// CHB 2006.09.29

					for (int ii = nOldLength; ii < nArrayLength; ii++)
					{
						// CHB 2006.09.29 - Note redundant checking of pElement->m_pDefinitionLoader for null.
						if (pElement->m_pDefinitionLoader)
						{
							pElement->m_pDefinitionLoader->InitValues(*(BYTE **)pTargetCurrent + ii * pElement->m_pDefinitionLoader->m_nStructSize, bZeroOutData);
						}
					}
				} 
				else if (nOldLength < nArrayLength && bZeroOutData) 
				{
					memclear(*(BYTE **)pTargetCurrent + nOldLength, (nArrayLength - nOldLength));
				}
			} 
			else 
			{
				if (nOldLength > 0 && *(void **)pTargetCurrent)
				{
					FREE(m_pMemPool, *(void **)pTargetCurrent);
				}
				*(void **)pTargetCurrent = NULL;
			}
			pTargetCurrent = *(void **)pTargetCurrent;
			pSourceCurrent = *(void **)pSourceCurrent;
		}

		switch (m_pElements[nIndex].m_eDataType)
		{
		case DATA_TYPE_BYTE:		
			if (pSourceCurrent)
			{
				memcpy(pTargetCurrent, pSourceCurrent, sizeof(BYTE) * nArrayLength);
			}
			else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					*(BYTE*)((BYTE *)pTargetCurrent + sizeof(BYTE) * ii) = *(BYTE*)&pElement->m_DefaultValue;
				}
			}

		case DATA_TYPE_INT:		
		case DATA_TYPE_INT_NOSAVE:
		case DATA_TYPE_INT_LINK:
		case DATA_TYPE_INT_INDEX_LINK:
		case DATA_TYPE_FLAG:
			if (pSourceCurrent)
			{
				memcpy(pTargetCurrent, pSourceCurrent, sizeof(int) * nArrayLength);
			}
			else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					*(int *)((BYTE *)pTargetCurrent + sizeof(int) * ii) = *(int *)&pElement->m_DefaultValue;
				}
			}
			break;

		case DATA_TYPE_FLAG_BIT:
			{
				int nSize = sElementGetSize( pElement );
				if (pSourceCurrent)
				{
					memcpy(pTargetCurrent, pSourceCurrent, nSize * nArrayLength);
				}
				else
				{
					ZeroMemory( pTargetCurrent, nSize * nArrayLength );
				}
			}
			break;

		case DATA_TYPE_INT_64:
			if (pSourceCurrent)
			{
				memcpy(pTargetCurrent, pSourceCurrent, sizeof(unsigned __int64) * nArrayLength);
			}
			else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					*(unsigned __int64 *)((BYTE *)pTargetCurrent + sizeof(unsigned __int64) * ii) = *(unsigned __int64 *)&pElement->m_DefaultValue;
				}
			}
			break;

		case DATA_TYPE_POINTER_NOSAVE:
			//if (pSourceCurrent)
			//{
			//	memcpy(pTargetCurrent, pSourceCurrent, sizeof(POINTER_AS_INT) * nArrayLength);
			//}
			//else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					*(POINTER_AS_INT *)((BYTE *)pTargetCurrent + sizeof(POINTER_AS_INT) * ii) = *(int *)&pElement->m_DefaultValue;
				}
			}
			break;

		case DATA_TYPE_FLOAT:	
		case DATA_TYPE_FLOAT_NOSAVE:
			if (pSourceCurrent)
			{
				memcpy(pTargetCurrent, pSourceCurrent, sizeof(float) * nArrayLength);
			}
			else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					*(float *)((BYTE *)pTargetCurrent + sizeof(float) * ii) = *(float *)&pElement->m_DefaultValue;
				}
			}
			break;

		case DATA_TYPE_INT_DEFAULT:
			for (int ii = 0; ii < nArrayLength; ii++)
			{
				*(int *)((BYTE *)pTargetCurrent + sizeof(int) * ii) = *(int *)&pElement->m_DefaultValue;
			}
			break;

		case DATA_TYPE_STRING:	
			if (pSourceCurrent)
			{
				memcpy(pTargetCurrent, pSourceCurrent, sizeof(char) * MAX_XML_STRING_LENGTH * nArrayLength);	
			}
			else
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					PStrCopy((char *)pTargetCurrent + ii * MAX_XML_STRING_LENGTH, (pElement->m_pszDefaultValue ? pElement->m_pszDefaultValue : ""), MAX_XML_STRING_LENGTH);
				}
			}
			break;

		case DATA_TYPE_PATH_FLOAT:
			for (int ii = 0; ii < nArrayLength; ii++)
			{
				int nOffset = ii * sizeof(CInterpolationPath<float>);
				CInterpolationPath<float> * pTargetPath = (CInterpolationPath<float> *)((BYTE *)pTargetCurrent + nOffset);
				CInterpolationPath<float> * pSourcePath = (CInterpolationPath<float> *)((BYTE *)pSourceCurrent + nOffset);
				pTargetPath->Copy(pSourcePath);
			}
			break;

		case DATA_TYPE_PATH_FLOAT_PAIR:
			for (int ii = 0; ii < nArrayLength; ii++)
			{
				int nOffset = ii * sizeof(CInterpolationPath<CFloatPair>);
				CInterpolationPath<CFloatPair> * pTargetPath = (CInterpolationPath<CFloatPair> *)((BYTE *)pTargetCurrent + nOffset);
				CInterpolationPath<CFloatPair> * pSourcePath = (CInterpolationPath<CFloatPair> *)((BYTE *)pSourceCurrent + nOffset);
				pTargetPath->Copy(pSourcePath);
			}
			break;

		case DATA_TYPE_PATH_FLOAT_TRIPLE:
			for (int ii = 0; ii < nArrayLength; ii++)
			{
				int nOffset = ii * sizeof(CInterpolationPath<CFloatTriple>);
				CInterpolationPath<CFloatTriple> * pTargetPath = (CInterpolationPath<CFloatTriple> *)((BYTE *)pTargetCurrent + nOffset);
				CInterpolationPath<CFloatTriple> * pSourcePath = (CInterpolationPath<CFloatTriple> *)((BYTE *)pSourceCurrent + nOffset);
				pTargetPath->Copy( pSourcePath );
			}
			break;

		case DATA_TYPE_DEFINITION:
			if (pElement->m_nArrayLength != -1)
			{
				for (int ii = 0; ii < nArrayLength; ii++)
				{
					int nOffset = ii * pElement->m_pDefinitionLoader->m_nStructSize;
					pElement->m_pDefinitionLoader->InitValues((BYTE *)pTargetCurrent + nOffset, bZeroOutData);
				}
			}

			for (int ii = 0; ii < nArrayLength; ii++)
			{
				int nOffset = ii * pElement->m_pDefinitionLoader->m_nStructSize;
				pElement->m_pDefinitionLoader->Copy((BYTE *)pTargetCurrent + nOffset, (BYTE *)pSourceCurrent + nOffset, bZeroOutData);
			}
			break;
		}
	}


	if (m_fnPostLoad)
	{
		m_fnPostLoad(pTarget, FALSE);
	}

}


// ************************************************************************************************
//
// --- Specific Definitions --- 
//
// ************************************************************************************************
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DEFINITION_START ( GLOBAL_DEFINITION, sgtDefinitionLoaderGlobals, 'GLOD', TRUE, TRUE, NULL, 4 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_FLOAT, fPlayerSpeed, 7.0f )
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NORAGDOLLS,			 			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_INVERTMOUSE,		 				0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NO_SHRINKING_BONES,	 			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_CHEAT_LEVELS,						0	)	
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NOMONSTERS,			 			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_GENERATE_ASSETS_IN_GAME,			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_DATA_WARNINGS,		 				1	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_BACKGROUND_WARNINGS,				1	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_FORCE_SYNCH,		 				0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_LOAD_DEBUG_BACKGROUND_TEXTURES,	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_SKILL_LEVEL_CHEAT,					0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_SILENT_ASSERT,		 				0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NOLOOT,				 			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_ABSOLUTELYNOMONSTERS,				0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_FULL_LOGGING,						0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_UPDATE_TEXTURES_IN_GAME,			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_AUTOMAP_ROTATE,					0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_FORCEBLOCKINGSOUNDLOAD,			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_HAVOKFX_ENABLED,					0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_MULTITHREADED_HAVOKFX_ENABLED,		0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_HAVOKFX_RAGDOLL_ENABLED,			0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NO_POPUPS,          		    	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_NO_CLEANUP_BETWEEN_LEVELS,     	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_CAST_ON_HOTKEY,			    	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_STRING_WARNINGS,					0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_MAX_POWER,					    	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_SKILL_COOLDOWN,			    	0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_USE_HQ_SOUNDS,			    		0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_PROMPT_FOR_CHECKOUT,				0	)
	DEFINITION_MEMBER_FLAG ( GLOBAL_DEFINITION, dwFlags, GLOBAL_FLAG_DONT_UPDATE_TEXTURES_IN_HAMMER,	0	)
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, dwGameFlags, 0 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, dwSeed, 0 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_STRING, szPlayerName, Marcus )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_STRING, szEnvironment,  )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, nDefWidth, 800 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, nDefHeight, 600 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, nDebugOutputLevel, 4 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_INT, nDebugSoundDelayTicks, 0 )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_STRING, szLanguage, English )
	DEFINITION_MEMBER ( GLOBAL_DEFINITION, DATA_TYPE_STRING, szAuthenticationServer, 192.168.50.162 )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_ADMINISTRATOR,							NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_DEVELOPER,								NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_FSSPING0_EMPLOYEE,						NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE,	 	NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_SUBSCRIBER,	 							NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_TITLE_BOT,	 								NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_MODIFIER_TRIAL_SUBSCRIPTION,					NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_MODIFIER_STANDARD_SUBSCRIPTION,				NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_MODIFIER_LIFETIME_SUBSCRIPTION,				NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_STATUS_UNDERAGE,								NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_STATUS_SUSPENDED,								NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_STATUS_BANNED_FROM_GUILDS,					NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_ACCOMPLISHMENT_ALPHA_TESTER,					NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_ACCOMPLISHMENT_BETA_TESTER,					NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_ACCOMPLISHMENT_HARDCORE_MODE_BEATEN,			NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	DEFINITION_MEMBER_FLAG_BIT ( GLOBAL_DEFINITION, Badges.m_badges, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN,			NUM_ACCOUNT_BADGE_TYPES	* BADGES_PER_TYPE_GROUP )
	
DEFINITION_END

class CGlobalContainer : public CDefinitionContainer
{
public:
	CGlobalContainer() 
	{
		m_pLoader = &sgtDefinitionLoaderGlobals;
		m_pszFilePath = FILE_PATH_GLOBAL;
		m_bAlwaysLoadDirect = TRUE;
		InitLoader();
	}
	void Free  ( void * pData, int nCount ) 
	{ 
		REF(nCount);
		FREE_DELETE_ARRAY(m_pMemPool, pData, GLOBAL_DEFINITION);
	}
	void * Alloc ( int nCount ) 
	{ 
		return MALLOC_NEWARRAY( m_pMemPool, GLOBAL_DEFINITION, nCount ); 
	}
};

//-------------------------------------------------------------------------------------------------
DEFINITION_START(CONFIG_DEFINITION, sgtDefinitionLoaderConfig, 'CFGD', TRUE, TRUE, NULL, 4 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_INT,   dwFlags, 1 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_INT,	  nSoundOutputType, 2 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_INT,	  nSoundSpeakerConfig, 1 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_INT,	  nSoundMemoryReserveType, 0 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_INT,	  nSoundMemoryReserveMegabytes, 64 )
	DEFINITION_MEMBER(CONFIG_DEFINITION, DATA_TYPE_FLOAT, fMouseSensitivity, 1.5f )
DEFINITION_END

class CConfigContainer : public CDefinitionContainer
{
public:
	CConfigContainer()
	{
		m_pLoader = &sgtDefinitionLoaderConfig;
		m_pszFilePath = FILE_PATH_GLOBAL;
		m_bAlwaysLoadDirect = TRUE;
		InitLoader();
	}
	void Free(
		void * pData,
		int nCount)
	{ 
		UNREFERENCED_PARAMETER(nCount);
		FREE_DELETE_ARRAY(m_pMemPool, pData, GLOBAL_DEFINITION); 
	}
	void * Alloc(
		int nCount)
	{ 
		return MALLOC_NEWARRAY(m_pMemPool, GLOBAL_DEFINITION, nCount); 
	}
};

//-------------------------------------------------------------------------------------------------
CDefinitionContainer * gppDefinitionContainers[ NUM_DEFINITION_GROUPS ];

// ************************************************************************************************
//
// --- Definition Functions --- 
//
// ************************************************************************************************
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void * DefinitionGetById  ( DEFINITION_GROUP_TYPE eType, int nId )
{
	ASSERT_RETNULL(nId != INVALID_ID);
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	if (!pContainer)
	{
		ASSERT(CDefinitionContainer::Closed());
		return NULL;
	}
	return pContainer->GetById( nId );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const char * DefinitionGetNameById  ( DEFINITION_GROUP_TYPE eType, int nId )
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	ASSERT_RETNULL( pContainer );
	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER *)pContainer->GetById( nId, TRUE );
	return pHeader->pszName;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void * DefinitionGetByName( 
	DEFINITION_GROUP_TYPE eType, 
	const char * pszName, 
	int nPriorityOverride,
	BOOL bForceSyncLoad,
	BOOL bForceIgnoreWarnIfMissing)
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	ASSERT_RETNULL( pContainer );
	DWORD dwFlags = 0;
	dwFlags |= bForceSyncLoad ? CDefinitionContainer::FLAG_FORCE_SYNC_LOAD : 0;
	dwFlags |= bForceIgnoreWarnIfMissing ? CDefinitionContainer::FLAG_FORCE_DONT_WARN_IF_MISSING : 0;
	return pContainer->GetByName( pszName, dwFlags, nPriorityOverride );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void * DefinitionGetByFilename( 
	DEFINITION_GROUP_TYPE eType, 
	const char * pszName, 
	int nPriorityOverride,
	BOOL bForceSyncLoad,
	BOOL bForceIgnoreWarnIfMissing)
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	ASSERT_RETNULL( pContainer );
	DWORD dwFlags = CDefinitionContainer::FLAG_NAME_HAS_PATH;
	dwFlags |= bForceSyncLoad ? CDefinitionContainer::FLAG_FORCE_SYNC_LOAD : 0;
	dwFlags |= bForceIgnoreWarnIfMissing ? CDefinitionContainer::FLAG_FORCE_DONT_WARN_IF_MISSING : 0;

	return pContainer->GetByName( pszName, dwFlags, nPriorityOverride );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int DefinitionGetIdByFilename( 
	DEFINITION_GROUP_TYPE eType, 
	const char * pszName, 
	int nPriorityOverride,
	BOOL bForceSyncLoad,
	BOOL bForceIgnoreWarnIfMissing,
	PAK_ENUM ePakfile /*= PAK_DEFAULT*/)
{
	if (PStrStrI(pszName, "nonWSloading_kor") != 0)
	{
		pszName = pszName;
	}

	DWORD dwFlags = CDefinitionContainer::FLAG_NAME_HAS_PATH | CDefinitionContainer::FLAG_EVEN_IF_NOT_LOADED;
	dwFlags |= bForceSyncLoad ? CDefinitionContainer::FLAG_FORCE_SYNC_LOAD : 0;
	dwFlags |= bForceIgnoreWarnIfMissing ? CDefinitionContainer::FLAG_FORCE_DONT_WARN_IF_MISSING : 0;

	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER *) CDefinitionContainer::GetContainerByType( eType )->
		GetByName( pszName, dwFlags, nPriorityOverride, ePakfile );
	if ( pHeader )
		return pHeader->nId;
	return INVALID_ID;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int DefinitionGetIdByName( 
	DEFINITION_GROUP_TYPE eType, 
	const char * pszName, 
	int nPriorityOverride,
	BOOL bForceSyncLoad,
	BOOL bForceIgnoreWarnIfMissing)
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType(eType);
	if (pContainer == NULL)
	{
		if (bForceIgnoreWarnIfMissing)
		{
			ASSERT_RETINVALID(pContainer);
		}
		return INVALID_ID;
	}
	return pContainer->GetIdByName( pszName, nPriorityOverride, bForceSyncLoad, bForceIgnoreWarnIfMissing );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int DefinitionGetNextId( 
	DEFINITION_GROUP_TYPE eType,
	int nId,
	BOOL bEvenIfNotLoaded /*= FALSE*/ )
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	// CHB 2006.09.05 - This would cause an infinite loop (caught by
	// a bounded while).
//	ASSERT_RETNULL( pContainer );
	ASSERT_RETINVALID( pContainer );
	return pContainer->GetNext( nId, bEvenIfNotLoaded );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int DefinitionGetFirstId( 
	DEFINITION_GROUP_TYPE eType,
	BOOL bEvenIfNotLoaded /*= FALSE*/ )
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	ASSERT_RETINVALID( pContainer );
	return pContainer->GetFirst( bEvenIfNotLoaded );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void DefinitionAddProcessCallback(
	DEFINITION_GROUP_TYPE eType,
	int nId, 
	PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad, 
	struct EVENT_DATA * pEventData)
{
	CDefinitionContainer * pContainer = CDefinitionContainer::GetContainerByType( eType );
	ASSERT_RETURN( pContainer );
	void * pDef = pContainer->GetById( nId );
	if (pDef)
	{
		(*fnPostLoad)(pDef, pEventData);
		return;
	}
	pContainer->AddPostLoadCallback(nId, fnPostLoad, pEventData);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define MAX_DEFINITIONS_PER_GROUP 1000

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

CDefinitionContainer *& DefinitionContainerGetRef( DEFINITION_GROUP_TYPE eType )
{
	static CDefinitionContainer * pNull = NULL;
	// should always be NULL
	ASSERT( pNull == NULL );
	if ( eType < 0 || eType >= NUM_DEFINITION_GROUPS )
		return pNull;
	return gppDefinitionContainers[ eType ];
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CDefinitionContainer::AddPostLoadCallback( int nId, PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad, struct EVENT_DATA * eventData)
{
	if (m_pLoader)
	{
		m_pLoader->AddPostLoadCallback(nId, fnPostLoad, eventData);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL CDefinitionContainer::m_bClosed = FALSE;

void CDefinitionContainer::Close ( void )
{
	m_bClosed = TRUE;
	for ( int nGroup = 0; nGroup < NUM_DEFINITION_GROUPS; nGroup ++ )
	{
		if ( gppDefinitionContainers[ nGroup ] )
		{
			gppDefinitionContainers[ nGroup ]->Destroy();
			delete gppDefinitionContainers[ nGroup ];
			gppDefinitionContainers[ nGroup ] = NULL;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void * CDefinitionContainer::GetByName( 
	const char * pszName, 
	DWORD dwFlags,
	int nPriorityOverride,
	PAK_ENUM ePakfile /*= PAK_DEFAULT*/)
{
	// in the case of no (meaningful) name, early-out
	if ( ! pszName || ! pszName[0] )
		return NULL;

#ifdef _DEBUG
	char szTimerName[MAX_PATH];
	PStrPrintf(szTimerName, MAX_PATH, "CDefinitionContainer::GetByName [%s]", pszName);
	TIMER_STARTEX(szTimerName, DEFAULT_PERF_THRESHOLD_LARGE);
#endif

	ASSERT_RETNULL(this);
	ASSERT_RETNULL(m_pLoader);

	BOOL bEvenIfNotLoaded			= !!(dwFlags & FLAG_EVEN_IF_NOT_LOADED);
	BOOL bForceSyncLoad				= !!(dwFlags & FLAG_FORCE_SYNC_LOAD);
	//BOOL bNullWhenMissing			= !!(dwFlags & FLAG_NULL_WHEN_MISSING);
	BOOL bNameHasPath				= !!(dwFlags & FLAG_NAME_HAS_PATH);
	BOOL bForceDontWarnIfMissing	= !!(dwFlags & FLAG_FORCE_DONT_WARN_IF_MISSING);

	char szNameWithoutExtension[MAX_XML_STRING_LENGTH];
	if (m_bRemoveExtension)
	{
		PStrRemoveExtension(szNameWithoutExtension, MAX_XML_STRING_LENGTH, pszName);
	}
	else
	{
		const char * pszExt = PStrGetExtension( pszName );
		if ( NULL == pszExt || NULL == pszExt[0] )
		{
			// Append ".xml" to extensionless request
			PStrRemoveExtension( szNameWithoutExtension, MAX_XML_STRING_LENGTH, pszName );
			PStrCat( szNameWithoutExtension, ".xml", MAX_XML_STRING_LENGTH );
		}
		else
		{
			PStrCopy(szNameWithoutExtension, pszName, MAX_XML_STRING_LENGTH);
		}
	}

	// these names are sometimes constructed with data, force that all slashes are backslashes
	// and force lowercase (it's easy to mistype case and [back]slash in data, but neither
	// of them affect access to a file in windows since the file system is not case sensitive
	// Forcing the backslash format and all lowercase will assure that all requests refer
	// to the same definition instance even when their paths are not exactly the same
	PStrFixPathBackslash(szNameWithoutExtension);
	PStrLower(szNameWithoutExtension, MAX_XML_STRING_LENGTH);

	// force the full path to avoid double-loading definitions
	char szFullNameWithoutExtension[MAX_XML_STRING_LENGTH];

	OS_PATH_CHAR oszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	OS_PATH_CHAR oszFullFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	if ( bNameHasPath )
	{
		PStrCvt( oszFilePath, szNameWithoutExtension, DEFAULT_FILE_WITH_PATH_SIZE );
		FileGetFullFileName( oszFullFilePath, oszFilePath, DEFAULT_FILE_WITH_PATH_SIZE );


//#ifdef DEBUG // In debug, pakfiles don't work...so we need fullpaths - bmanegold
//		FileGetFullFileName(szFullNameWithoutExtension, szNameWithoutExtension, MAX_XML_STRING_LENGTH);
//#else // but in retail...full paths theoretically could have unicode, so we can't use them
//		PStrCopy(szFullNameWithoutExtension, szNameWithoutExtension, MAX_XML_STRING_LENGTH);
//#endif
	} 
	else
	{
		// Prepend the definition root
		const char * pszDefRoot = GetFilePath();
		char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		char szTemp[ DEFAULT_FILE_WITH_PATH_SIZE ];

		PStrRemovePath( szTemp, DEFAULT_FILE_WITH_PATH_SIZE, pszDefRoot, szNameWithoutExtension );

		PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s\\%s", pszDefRoot, szTemp );
		PStrCvt( oszFilePath, szFilePath, DEFAULT_FILE_WITH_PATH_SIZE );
		FileGetFullFileName( oszFullFilePath, oszFilePath, DEFAULT_FILE_WITH_PATH_SIZE );
		//PStrCopy(szFullNameWithoutExtension, szNameWithoutExtension, MAX_XML_STRING_LENGTH);
	}

	PStrFixPathBackslash( oszFullFilePath );
	PStrLower( oszFullFilePath, DEFAULT_FILE_WITH_PATH_SIZE );
	const OS_PATH_CHAR * poszRoot = AppCommonGetRootDirectory( NULL );
	PStrRemovePath( oszFilePath, DEFAULT_FILE_WITH_PATH_SIZE, poszRoot, oszFullFilePath );
	PStrCvt( szFullNameWithoutExtension, oszFilePath, MAX_XML_STRING_LENGTH );


#if ISVERSION(DEVELOPMENT)
	ASSERTX(PStrStrI(szFullNameWithoutExtension, DEFINITION_COOKED_EXTENSION) == NULL, "Definition::GetByName() is being asked for a cooked asset, it should be asking for .xml or a definition via a name that is extension free");
	//This asserts on Mythos all the time because we share assets for the models which means it goes back a few directory levels
	//to link to the assets.
	if( AppIsHellgate() )
	{
		ASSERTV(PStrStrI(szFullNameWithoutExtension, "..") == NULL, "Definition::GetByName() is being asked for an asset using a path with \"..\" in it!  This is not supported and will result, at best, in a duplicate copy of the definition in memory!\nPath:\n%s\n", szFullNameWithoutExtension);
	}
#endif

	DEFINITION_HEADER * pHeader = NULL;
	void * pDefinition = NULL;

	// first see if we already have this definition
	{
		DEFINITION_HASH * pHashDef = HashGetFirst( m_Hash );
		while ( pHashDef )
		{
			DEFINITION_HEADER * pTempHeader = pHashDef->GetHeader();

			if (PStrICmp(pTempHeader->pszName, szFullNameWithoutExtension, MAX_XML_STRING_LENGTH) == 0)
			{
				if (!TESTBIT(pTempHeader->dwFlags, DHF_LOADED_BIT) && bForceSyncLoad )
				{
					// not yet loaded, and we want sync load, try waiting for a while to see if we get loaded
					for (unsigned int ii = 0; ii < 200; ++ii)
					{
						LogMessage( "Definition requested force-sync, but already loading async!  Waiting for async load to complete... -- FILE: %s\n", szFullNameWithoutExtension);
						AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
						if (TESTBIT(pTempHeader->dwFlags, DHF_LOADED_BIT))
						{
							return pTempHeader;
						}
						Sleep(5);
					}

					pHeader = pTempHeader;
					pDefinition = pTempHeader;
					break;
				}
				if (TESTBIT(pTempHeader->dwFlags, DHF_LOADED_BIT) || bEvenIfNotLoaded)
				{
					return pTempHeader;
				}
				return NULL;
			}
			pHashDef = HashGetNext( m_Hash, pHashDef );
		}
	}

	if (!pHeader)
	{
		// make a new definition
		DEFINITION_HASH * pDefHash = HashAdd( m_Hash, m_nNextId++ );
		pDefHash->pData = Alloc( 1 );

		ASSERT_RETNULL(m_pLoader);
		pDefinition = (BYTE *)pDefHash->pData;

		m_pLoader->InitValues(pDefinition, m_bZeroOutData);

		// Initialize the definition with defaults
		pHeader = (DEFINITION_HEADER *)pDefinition;
		PStrCopy(pHeader->pszName, szFullNameWithoutExtension, MAX_XML_STRING_LENGTH);
		pHeader->nId = pDefHash->nId;
		pHeader->dwFlags = 0;
	}

	if (!pHeader->pszName[0])
	{
		return pDefinition;
	}

	int nPriority = nPriorityOverride != -1 ? nPriorityOverride : m_nLoadPriority;

	BOOL bAsyncLoad = c_GetToolMode() ? FALSE : m_bAsyncLoad;
	bAsyncLoad = bForceSyncLoad ? FALSE : bAsyncLoad;
	if (sgbForceSync)
	{
		bAsyncLoad = FALSE;
	}
	// Load the definition from file
	if (pHeader->pszName[0] != 0)
	{
		// CML 2008.05.06 - In order to fix the double load bug when switching between GetByName and GetByFilename,
		//   the definition header now always contains the app-root-relative filepath.
		Load(pHeader, TRUE, pHeader->nId, bAsyncLoad, nPriority, bForceDontWarnIfMissing, ePakfile);
	} 
	else 
	{
		SETBIT(pHeader->dwFlags, DHF_LOADED_BIT);
	}

	if (TESTBIT(pHeader->dwFlags, DHF_LOADED_BIT) || bEvenIfNotLoaded)
	{
		return pDefinition;
	}
	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL CDefinitionContainer::SaveCooked( void* pDefinition )
{
	WRITE_BUF_DYNAMIC bbuf(NULL);

	char szFullFilenameCooked[ MAX_PATH ];
	
	// generate cooked filename
	DEFINITION_HEADER* pHeader = (DEFINITION_HEADER*)pDefinition;
	if ( m_bIgnoreFilePathOnSave )
		PStrPrintf( szFullFilenameCooked, MAX_PATH, "%s", pHeader->pszName );
	else
	{
		char szFullFilename[ MAX_PATH ];
		PStrPrintf( szFullFilename, MAX_PATH, "%s%s", m_pszFilePath, pHeader->pszName );
		FileGetFullFileName( szFullFilenameCooked, szFullFilename, MAX_PATH );
	}
	PStrReplaceExtension( szFullFilenameCooked, MAX_PATH, szFullFilenameCooked, DEFINITION_FILE_EXTENSION );
	PStrPrintf( szFullFilenameCooked, MAX_PATH, "%s.%s", szFullFilenameCooked, DEFINITION_COOKED_EXTENSION);	

	// Clear out the id field when saving
	int tempId = pHeader->nId;
	pHeader->nId = 0;
	m_pLoader->SaveCooked( pDefinition, bbuf );
	pHeader->nId = tempId;

	BOOL bCanSave = TRUE;
	if ( FileIsReadOnly( szFullFilenameCooked ) )
		bCanSave = DataFileCheck( szFullFilenameCooked );

	if ( bCanSave )
		return bbuf.Save( szFullFilenameCooked );

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CDefinitionContainer::Reload ( void* pDefinition )
{
	DEFINITION_HEADER* pHeader = (DEFINITION_HEADER *)pDefinition;
	CLEARBIT(pHeader->dwFlags, DHF_LOADED_BIT);
	Load( pHeader, TRUE, pHeader->nId, TRUE, -1 );	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CDefinitionContainer::Load( 
	DEFINITION_HEADER* pHeader, 
	BOOL bFilenameHasPath, 
	int nId, 
	BOOL bAsyncLoad, 
	int nPriority, 
	BOOL bForceIgnoreWarnIfMissing,
	PAK_ENUM ePakfile /*= PAK_DEFAULT*/ )
{
	FILE_REQUEST_DATA tData;
	tData.eGroup = m_eDefinitionGroup;
	tData.pszFullFileName = NULL;
	tData.pszDefinitionName = pHeader->pszName; 
	tData.pszPathName = bFilenameHasPath ? "" : m_pszFilePath;
	tData.nDefinitionId = nId;
	tData.bForceLoadDirect = m_bAlwaysLoadDirect;
	tData.bNeverPutInPak = m_bNeverPutInPak;
	tData.bZeroOutData = m_bZeroOutData;
	tData.bWarnWhenMissing = m_bWarnWhenMissing;
	tData.bAlwaysWarnWhenMissing = bForceIgnoreWarnIfMissing ? FALSE : m_bAlwaysWarnWhenMissing;
	tData.bNodirectIgnoreMissing = m_bNodirectIgnoreMissing;
	tData.bAsyncLoad = bAsyncLoad;
	tData.nFileLoadPriority = nPriority;
	tData.ePakfile = ePakfile;
	tData.bForceIgnoreWarnIfMissing = bForceIgnoreWarnIfMissing;

	m_pLoader->RequestFile(tData);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int CDefinitionContainer::GetIdByName( 
	const char * pszName,
	int nPriorityOverride,
	BOOL bForceSyncLoad,
	BOOL bForceIgnoreWarnIfMissing)
{
	DWORD dwFlags = CDefinitionContainer::FLAG_EVEN_IF_NOT_LOADED;
	dwFlags |= bForceSyncLoad ? CDefinitionContainer::FLAG_FORCE_SYNC_LOAD : 0;
	dwFlags |= bForceIgnoreWarnIfMissing ? CDefinitionContainer::FLAG_FORCE_DONT_WARN_IF_MISSING : 0;

	DEFINITION_HEADER * pHeader = (DEFINITION_HEADER * ) GetByName( pszName, dwFlags, nPriorityOverride );

	return pHeader ? pHeader->nId : INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void * CDefinitionContainer::GetById( 
	int nId, 
	BOOL bEvenIfNotLoaded )
{
	ASSERT_RETNULL( nId >= 0 );

	DEFINITION_HASH * pDefHash = HashGet( m_Hash, nId );
	if ( ! pDefHash )
		return NULL;

	DEFINITION_HEADER * pHeader = pDefHash->GetHeader();
	if ( TESTBIT(pHeader->dwFlags, DHF_LOADED_BIT) || bEvenIfNotLoaded )
		return pHeader;

	return NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int CDefinitionContainer::GetFirst ( 
	BOOL bEvenIfNotLoaded )
{
	DEFINITION_HASH* pDefHash = HashGetFirst( m_Hash );
	if ( !pDefHash )
		return INVALID_ID;

	DEFINITION_HEADER* pHeader = pDefHash->GetHeader();
	if ( TESTBIT(pHeader->dwFlags, DHF_LOADED_BIT) || bEvenIfNotLoaded )
		return pHeader->nId;

	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int CDefinitionContainer::GetNext( 
	int nId, 
	BOOL bEvenIfNotLoaded )
{
	DEFINITION_HASH * pDefHash = HashGet( m_Hash, nId );
	if ( ! pDefHash )
		return INVALID_ID;

	pDefHash = HashGetNext( m_Hash, pDefHash );
	if ( ! pDefHash )
		return INVALID_ID;

	while ( pDefHash )
	{
		DEFINITION_HEADER * pHeader = pDefHash->GetHeader();
		if ( TESTBIT(pHeader->dwFlags, DHF_LOADED_BIT) || bEvenIfNotLoaded )
			return pHeader->nId;

		pDefHash = HashGetNext( m_Hash, pDefHash );
	}

	return INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL CDefinitionContainer::Save ( void * pDefinition, BOOL bAndSaveCooked /*= FALSE*/ )
{
	// Always try to save the cooked as well in Hammer
	if ( c_GetToolMode() )
		bAndSaveCooked = TRUE;

	BOOL bResult;
	if (m_pszFilePathOS != 0)
	{
		bResult = m_pLoader->SaveFile( pDefinition, m_pszFilePathOS, m_bIgnoreFilePathOnSave );
	}
	else
	{
		bResult = m_pLoader->SaveFile( pDefinition, m_pszFilePath, m_bIgnoreFilePathOnSave );
	}
	if ( ! bResult )
		return FALSE;

	if ( bAndSaveCooked )
		bResult = SaveCooked( pDefinition );

	return bResult;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CDefinitionContainer::Copy ( void ** ppTarget, void * pSource )
{
	// allocate one if they need it
	if ( *ppTarget == NULL )
	{
		*ppTarget = Alloc( 1 );
		m_pLoader->InitValues( *ppTarget, m_bZeroOutData );
	}
	m_pLoader->Copy( *ppTarget, pSource, m_bZeroOutData );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CDefinitionContainer::RestoreAll ( )
{
	DEFINITION_HASH * pDefHash = HashGetFirst( m_Hash );
	while ( pDefHash )
	{
		Restore( pDefHash->pData );
		pDefHash = HashGetNext( m_Hash, pDefHash );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CDefinitionContainer * CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_TYPE eType )
{
	ASSERT_RETNULL( eType >= 0 && eType < NUM_DEFINITION_GROUPS );
	return gppDefinitionContainers[ eType ];
}

// ************************************************************************************************
//
// --- Definition Test Code --- 
//
// ************************************************************************************************

#if ISVERSION(DEVELOPMENT)

struct TEST_STRUCT_INSIDE 
{
	int nTestInt53;
	int nTestInt27;
	float fTestFloat2p7;
};

//-------------------------------------------------------------------------------------------------
// the structure that we intend to test with.  The numbers show what the test values should be.
struct TEST_STRUCT 
{
	DEFINITION_HEADER tHeader;

	int nIntNoSave;
	int nIntDefault;
	unsigned __int64 nBigInt;

	int nTestInt63;
	float nTestInt12;
	int nTestInt24;
	int nTestInt42;
	int pnTestIntArray1234[4];
	int nTestIntIgnore;

	DWORD dwTestFlags;
	UINT64 nTestFlagBits;

	char pszTestString[MAX_XML_STRING_LENGTH];

	TEST_STRUCT_INSIDE tStructInside;

	float fTestFloat4;
	float fTestFloat6p32;
	float fTestFloatIgnore;

	TEST_STRUCT_INSIDE ptStructInsideArray[2];

	int nStructInsideCount;
	TEST_STRUCT_INSIDE * ptStructInsideVariableArray;

	CInterpolationPath<CFloatPair> tTestInterpolationPath;
	CInterpolationPath<CFloatPair> tEmptyInterpolationPath;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sCompareTestInsideStructures( TEST_STRUCT_INSIDE & tFirst, TEST_STRUCT_INSIDE & tSecond )
{
	ASSERT ( tFirst.fTestFloat2p7 == tSecond.fTestFloat2p7 );
	ASSERT ( tFirst.nTestInt53	   == tSecond.nTestInt53	);
	ASSERT ( tFirst.nTestInt27	   == tSecond.nTestInt27	);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define TEST_STRING "Test String of Fun"
#define TEST_NAME "Save"
static void sCompareTestStructures( TEST_STRUCT * pStructOriginal, TEST_STRUCT * pStructCopy )
{
//	ASSERT ( PStrICmp( pStructCopy->tHeader.pszName, TEST_NAME ) == 0 );
	ASSERT ( pStructCopy->nTestInt63 == 63 );
	ASSERT ( pStructCopy->nTestInt12 == 12 );
	ASSERT ( pStructCopy->nTestInt42 == 42 );
	ASSERT ( pStructCopy->fTestFloat4		== 4.0f );
	ASSERT ( pStructCopy->fTestFloat6p32	== 6.32f );

	ASSERT ( pStructCopy->pnTestIntArray1234[ 0 ] == 1 );
	ASSERT ( pStructCopy->pnTestIntArray1234[ 1 ] == 2 );
	ASSERT ( pStructCopy->pnTestIntArray1234[ 2 ] == 3 );
	ASSERT ( pStructCopy->pnTestIntArray1234[ 3 ] == 4 );

	ASSERT ( PStrCmp ( pStructCopy->pszTestString, TEST_STRING ) == 0 );

	sCompareTestInsideStructures ( pStructCopy->tStructInside, pStructOriginal->tStructInside );
	sCompareTestInsideStructures ( pStructCopy->ptStructInsideArray[0], pStructOriginal->ptStructInsideArray[0] );
	sCompareTestInsideStructures ( pStructCopy->ptStructInsideArray[1], pStructOriginal->ptStructInsideArray[1] );
	ASSERT ( pStructCopy->nStructInsideCount == 2);
	sCompareTestInsideStructures ( pStructCopy->ptStructInsideVariableArray[0], pStructOriginal->ptStructInsideVariableArray[0] );
	sCompareTestInsideStructures ( pStructCopy->ptStructInsideVariableArray[1], pStructOriginal->ptStructInsideVariableArray[1] );

	ASSERT ( pStructCopy->tTestInterpolationPath.GetPointCount() == 4 );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 0 )->m_fTime == 0.0f );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 1 )->m_fTime == 0.1f );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 2 )->m_fTime == 0.2f );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 3 )->m_fTime == 0.3f );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 0 )->m_tData == CFloatPair( 123.0f, 10.0f ) );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 1 )->m_tData == CFloatPair( 234.0f, 20.0f ) );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 2 )->m_tData == CFloatPair( 345.0f, 30.0f ) );
	ASSERT ( pStructCopy->tTestInterpolationPath.GetPoint( 3 )->m_tData == CFloatPair( 456.0f, 40.0f ) );

	ASSERT ( pStructCopy->tEmptyInterpolationPath.GetPointCount() == 0 );

	ASSERT ( pStructCopy->nIntDefault == 555 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

enum
{
	TEST_FLAG_BIT_ZERO,
	TEST_FLAG_BIT_ONE,
	TEST_FLAG_BIT_TWO,
	TEST_FLAG_BIT_THREE,
	TEST_FLAG_BIT_FOUR,
	TEST_FLAG_BIT_FIVE,
	NUM_TEST_FLAG_BITS,
};

// The structures that we intend to test with.  The numbers show what the test values should be.
// We don't include values called "Ignore" on purpose
DEFINITION_START (TEST_STRUCT_INSIDE, tDefinitionTestStructInside, 'TESD', FALSE, FALSE, NULL, 1000)
	DEFINITION_MEMBER ( TEST_STRUCT_INSIDE, DATA_TYPE_INT, nTestInt53, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT_INSIDE, DATA_TYPE_INT, nTestInt27, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT_INSIDE, DATA_TYPE_FLOAT, fTestFloat2p7, 0 )
DEFINITION_END

DEFINITION_START (TEST_STRUCT, tDefinitionTestStruct, 'TESD', TRUE, FALSE, NULL, 1000)
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT_NOSAVE, nIntNoSave, 123 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT_DEFAULT, nIntDefault, 555 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_FLOAT, nTestInt12, 0 )

	DEFINITION_MEMBER_FLAG ( TEST_STRUCT, dwTestFlags, GLOBAL_FLAG_MAX_POWER, 0 )							// stores 0x00000000, 0x02000000 (def,shifted)
//	DEFINITION_MEMBER_FLAG ( TEST_STRUCT, dwTestFlags, GLOBAL_FLAG_SKILL_COOLDOWN, 0 )						// stores 0x00000000, 0x04000000 (def,shifted)
	DEFINITION_MEMBER_FLAG ( TEST_STRUCT, dwTestFlags, GLOBAL_FLAG_USE_HQ_SOUNDS, 1 )						// stores 0x00000001, 0x08000000 (def,shifted)

//	DEFINITION_MEMBER_FLAG_BIT ( TEST_STRUCT, nTestFlagBits, TEST_FLAG_BIT_ZERO,  NUM_TEST_FLAG_BITS )		// stores 0x00000000, 0x00000006 (shift,count)
//	DEFINITION_MEMBER_FLAG_BIT ( TEST_STRUCT, nTestFlagBits, TEST_FLAG_BIT_THREE, NUM_TEST_FLAG_BITS )		// stores 0x00000003, 0x00000006 (shift,count)
//	DEFINITION_MEMBER_FLAG_BIT ( TEST_STRUCT, nTestFlagBits, TEST_FLAG_BIT_FIVE,  NUM_TEST_FLAG_BITS )		// stores 0x00000005, 0x00000006 (shift,count)

	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT, nTestInt24, 5 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT, nTestInt42, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT, nTestInt63, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_INT_64, nBigInt, 0x1231231231231231 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_FLOAT, fTestFloat4, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_FLOAT, fTestFloat6p32, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_STRING, pszTestString, "blah" )
	DEFINITION_MEMBER_ARRAY ( TEST_STRUCT, DATA_TYPE_INT, pnTestIntArray1234, 0, 4 )
	DEFINITION_REFERENCE ( TEST_STRUCT, tDefinitionTestStructInside, tStructInside )
	DEFINITION_REFERENCE_ARRAY ( TEST_STRUCT, tDefinitionTestStructInside, ptStructInsideArray, 2 )
	DEFINITION_REFERENCE_VARRAY ( TEST_STRUCT, tDefinitionTestStructInside, ptStructInsideVariableArray, nStructInsideCount )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_PATH_FLOAT_PAIR, tTestInterpolationPath, 0 )
	DEFINITION_MEMBER ( TEST_STRUCT, DATA_TYPE_PATH_FLOAT_PAIR, tEmptyInterpolationPath, 0.2f )
DEFINITION_END

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

class CTestContainer : public CDefinitionContainer
{
public:
	CTestContainer() 
	{
		m_pLoader = &tDefinitionTestStruct;
		m_pszFilePath = FILE_PATH_GLOBAL;
//		m_bAlwaysLoadDirect = TRUE;
		m_bZeroOutData = FALSE;
		InitLoader();
	}
	void Free( void * pData, int nCount ) 
	{ 
		REF(nCount);
		FREE_DELETE_ARRAY(NULL, pData, TEST_STRUCT);
	}
	void * Alloc( int nCount ) 
	{ 
		return MALLOC_NEWARRAY( NULL, TEST_STRUCT, nCount ); 
	}
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void testDefinition()
{
	// Fill out a test copy of this data
	TEST_STRUCT tStructToSave;
	tStructToSave.nTestInt12 = 12;
	tStructToSave.nTestInt24 = 24;
	tStructToSave.nTestInt42 = 42;
	tStructToSave.nTestInt63 = 63;
	tStructToSave.fTestFloat4		= 4.0f;
	tStructToSave.fTestFloat6p32	= 6.32f;
	tStructToSave.pnTestIntArray1234[ 0 ] = 1;
	tStructToSave.pnTestIntArray1234[ 1 ] = 2;
	tStructToSave.pnTestIntArray1234[ 2 ] = 3;
	tStructToSave.pnTestIntArray1234[ 3 ] = 4;
	PStrCopy( tStructToSave.pszTestString, TEST_STRING, MAX_XML_STRING_LENGTH );

	tStructToSave.tStructInside.fTestFloat2p7 = 2.7f;
	tStructToSave.tStructInside.nTestInt53 = 53;
	tStructToSave.tStructInside.nTestInt27 = 27;

	tStructToSave.nBigInt = 0x5555555555555555;

	tStructToSave.dwTestFlags = GLOBAL_FLAG_MAX_POWER | GLOBAL_FLAG_SKILL_COOLDOWN;
	tStructToSave.nTestFlagBits = (((UINT64)1)<<TEST_FLAG_BIT_THREE) | (((UINT64)1)<<TEST_FLAG_BIT_FIVE);

	tStructToSave.ptStructInsideArray[0].fTestFloat2p7 = 2.7f;
	tStructToSave.ptStructInsideArray[0].nTestInt53 = 53;
	tStructToSave.ptStructInsideArray[0].nTestInt27 = 27;
	tStructToSave.ptStructInsideArray[1].fTestFloat2p7 = 2.7f + 100.0f;
	tStructToSave.ptStructInsideArray[1].nTestInt53 = 53 + 100;
	tStructToSave.ptStructInsideArray[1].nTestInt27 = 27 + 100;

	tStructToSave.nStructInsideCount = 2;
	tStructToSave.ptStructInsideVariableArray = (TEST_STRUCT_INSIDE *) MALLOC(NULL, 2 * sizeof (TEST_STRUCT_INSIDE));
	tStructToSave.ptStructInsideVariableArray[0].fTestFloat2p7 = 2.7f + 200.0f;
	tStructToSave.ptStructInsideVariableArray[0].nTestInt53 = 53 + 200;
	tStructToSave.ptStructInsideVariableArray[0].nTestInt27 = 27 + 200;
	tStructToSave.ptStructInsideVariableArray[1].fTestFloat2p7 = 2.7f + 300.0f;
	tStructToSave.ptStructInsideVariableArray[1].nTestInt53 = 53 + 300;
	tStructToSave.ptStructInsideVariableArray[1].nTestInt27 = 27 + 300;

	tStructToSave.tTestInterpolationPath.AddPoint( CFloatPair( 123.0f, 10.0f ), 0.0f );
	tStructToSave.tTestInterpolationPath.AddPoint( CFloatPair( 234.0f, 20.0f ), 0.1f );
	tStructToSave.tTestInterpolationPath.AddPoint( CFloatPair( 345.0f, 30.0f ), 0.2f );
	tStructToSave.tTestInterpolationPath.AddPoint( CFloatPair( 456.0f, 40.0f ), 0.3f );

	tStructToSave.tEmptyInterpolationPath.SetDefault(1.5f);

	tStructToSave.nIntDefault = 666;
	tStructToSave.nIntNoSave = 456;

	PStrCopy( tStructToSave.tHeader.pszName, TEST_NAME, MAX_XML_STRING_LENGTH );
	tStructToSave.tHeader.nId = 12;

	// Save the test data (to XML)

	tDefinitionTestStruct.SaveFile( &tStructToSave, "data\\test\\", FALSE );

	// Load the same file and see if we get the data back out of it.
//	tStructToLoad.nTestIntIgnore	= 52;
//	tStructToLoad.fTestFloatIgnore	= 25.0f;

	// load the same test data into a different struct and compare
	TEST_STRUCT * tStructToLoad = (TEST_STRUCT*)DefinitionGetByFilename( DEFINITION_GROUP_TEST, "data\\test\\save.xml", -1, FALSE, FALSE );

	sCompareTestStructures( & tStructToSave, tStructToLoad );

//	ASSERT ( tStructToLoad->nTestIntIgnore   == 52 );
//	ASSERT ( tStructToLoad->fTestFloatIgnore == 25.0f );

	ASSERT ( tStructToLoad->nIntNoSave == 123 );

	TEST_STRUCT tStructCopyTo;
	ZeroMemory ( &tStructCopyTo, sizeof( TEST_STRUCT ) );
	CInterpolationPathInit(tStructCopyTo.tTestInterpolationPath);
	tDefinitionTestStruct.Copy( & tStructCopyTo, & tStructToSave, FALSE );

	sCompareTestStructures( & tStructToSave, & tStructCopyTo );
	ASSERT ( tStructCopyTo.tHeader.nId == 12 );

	FREE (NULL, tStructToSave.ptStructInsideVariableArray);
	FREE (NULL, tStructCopyTo.ptStructInsideVariableArray);
//	FREE (NULL, tStructToLoad->ptStructInsideVariableArray);
}

#endif //#if ISVERSION(DEVELOPMENT)

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void CDefinitionContainer::InitCommon ( void )
{
	DefinitionContainerGetRef( DEFINITION_GROUP_GLOBAL ) = new CGlobalContainer;
	DefinitionContainerGetRef( DEFINITION_GROUP_CONFIG ) = new CConfigContainer;
//	DefinitionContainerGetRef( DEFINITION_GROUP_SERVERLIST ) = new CServerlistContainer;
//	DefinitionContainerGetRef( DEFINITION_GROUP_SERVERLISTOVERRIDE ) = new CServerlistOverrideContainer;

#if ISVERSION(DEVELOPMENT)
	DefinitionContainerGetRef( DEFINITION_GROUP_TEST ) = new CTestContainer;
#endif //#if ISVERSION(DEVELOPMENT)
}
