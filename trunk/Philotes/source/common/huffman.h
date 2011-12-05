//----------------------------------------------------------------------------
// huffman.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

//----------------------------------------------------------------------------
// COMPONENT STRUCTS
//----------------------------------------------------------------------------

struct HuffmanNode
{
	DWORD symbol;
	//	HuffmanNode *head;
	HuffmanNode *zero; //follow this branch for bit = 0.  Less likely codewords.
	HuffmanNode *one;  //follow this branch for bit = 1.   More likely.
	unsigned long probability;
	HuffmanNode(DWORD asymbol, unsigned long aprobability)
		: symbol(asymbol), probability(aprobability),
		zero(NULL), one(NULL)
	{}
};

struct BitCode
{//Bit code using most minor bits.  Caps at 64 currently.
	union
	{
		UINT64 bits;
		BYTE bbits[8];
	};
	BYTE nBitCount;

	BitCode(UINT64 thebits, BYTE numbits):bits(thebits), nBitCount(numbits) {}
	BitCode():bits(0), nBitCount(0) {}

	BOOL PushBack(UINT64 thebits, BYTE newbits);
	BOOL PushFront(UINT64 thebits, BYTE newbits);

	inline BOOL PushBack(const BitCode &code)
	{
		return PushBack(code.bits, code.nBitCount);
	}
	inline BOOL PushFront(const BitCode &code)
	{
		return PushFront(code.bits, code.nBitCount);
	}
};

const BitCode INVALID_BITCODE( (UINT64)-1, (BYTE)-1);

//----------------------------------------------------------------------------
// MAIN STRUCT
//----------------------------------------------------------------------------

struct HuffmanCode
{
	HuffmanNode		*m_pHead;
	DWORD			m_nSymbolCount;
	DWORD			m_nMaxSymbol;
	BitCode			*m_pMappingArray;
	BitCode			m_bcInvalidPad; //A bitcode of 7 bits that will not decode
									//to anything, and thus can be used to
									//pad packets that do not come to an even
									//number of bytes.

	void Free(MEMORYPOOL * pPool);

	BitCode * MakeMappingArray(MEMORYPOOL * pPool);
	DWORD FindSymbol(BYTE * stream, int &startbit, int endbit);
	inline BitCode FindCodeWord(DWORD symbol) {return m_pMappingArray[symbol];}
	//given a stream of bits, find the symbol mapping to the first variable-length codeword
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL ValidateHuffman(
	HuffmanCode * pHuffmanCode);

HuffmanCode * MakeHuffman( 
	HuffmanNode ** nodes, 
	DWORD nNodeCount, 
	MEMORYPOOL * pPool);

void HuffmanTest(
	MEMORYPOOL * pPool);

#endif