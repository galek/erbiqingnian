//Robert Donald
//----------------------------------------------------------------------------
// huffman.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//Given a set of words and their frequencies, build the optimal huffman
//code tree, and make a mapping of the words to their codewords.

//Note: works with non-normalized 


#include "heap.h"
#include "huffman.h"

//Huffman algorithm:
//take a sorted list of symbols.
//take the two lowest symbols
//combine them into one node
//remove those two nodes and add this new node to the sorted list.

#define MAX_HUFFMAN_SIZE 16384



void RecursiveFreeHuffmanNodes(HuffmanNode *node, MEMORYPOOL *pPool)
{
	if(node == NULL) return;
	RecursiveFreeHuffmanNodes(node->zero, pPool);
	RecursiveFreeHuffmanNodes(node->one, pPool);
	FREE(pPool, node);
}

#define INVALID_SYMBOL DWORD(0)

BOOL CompareByProbability(HuffmanNode *a, HuffmanNode *b)
{ //NULL pointers are higher, thus pushed to the end.
	if(!(a) ) return FALSE;
	if(!(b) ) return TRUE;

	return (a)->probability < (b)->probability;
}

BOOL BitCode::PushBack(UINT64 thebits, BYTE newbits)
{
	ASSERTX_RETFALSE(nBitCount + newbits <= 8*sizeof(bits), "BitCode may only have 64 bits.");
	ASSERTX_RETFALSE( (thebits >> newbits) == 0, "Input has more bits than specified.");
	bits = (bits << newbits);
	bits += thebits;
	nBitCount = newbits + nBitCount;
	return TRUE;
}
BOOL BitCode::PushFront(UINT64 thebits, BYTE newbits)
{
	ASSERTX_RETFALSE(nBitCount + newbits <= 8*sizeof(bits), "BitCode may only have 64 bits.");
	ASSERTX_RETFALSE( (thebits >> newbits) == 0, "Input has more bits than specified.");
	thebits = (thebits << nBitCount);
	bits += thebits;
	nBitCount = newbits + nBitCount;
	return TRUE;
}

const BitCode NoBits(0, 0);
const BitCode ZeroBit(0, 1);
const BitCode OneBit(1, 1);

void HuffmanCode::Free(MEMORYPOOL * pPool) 
{
	RecursiveFreeHuffmanNodes(m_pHead, pPool);
	m_pHead = NULL;
	m_nSymbolCount = 0;
	if(m_pMappingArray) FREE(pPool, m_pMappingArray);
	m_pMappingArray = NULL;
}
DWORD HuffmanCode::FindSymbol(BYTE * stream, int &startbit, int endbit)
{
	//Note: goes bit by bit.  If this is too slow, I'll remap the huffman binary
	//tree into a byte trie, which should be ten or so times faster.  Or perhaps a nibble trie.
	int i = startbit;
	HuffmanNode *pNode = m_pHead;

	while(i <= endbit)
	{
		ASSERT_RETNULL(pNode);
		if(pNode->symbol)
		{//Termination of code.
			startbit = i;
			return pNode->symbol;
		}
		BOOL bit = TESTBIT(stream, i);
		if(bit == 0) pNode = pNode->zero;
		else pNode = pNode->one;

		i++;
	}
	return NULL;
}

void RecursiveBitAssign(const BitCode &bits, BitCode* mapping, HuffmanNode *node)
{
	if (node->symbol) //it's a leaf.
	{
		ASSERT(node->one  == NULL);
		ASSERT(node->zero == NULL);

		mapping[node->symbol] = bits;
		return;
	}
	
	if(node->zero)
	{
		BitCode zerobits = bits;
		zerobits.PushFront(ZeroBit);

		RecursiveBitAssign(zerobits, mapping, node->zero);
	}

	if(node->one)
	{
		BitCode onebits = bits;
		onebits.PushFront(OneBit);

		RecursiveBitAssign(onebits, mapping, node->one);
	}
}

BitCode * HuffmanCode::MakeMappingArray(MEMORYPOOL * pPool)
{
	BitCode * toRet = (BitCode *)MALLOCZ(pPool, (m_nMaxSymbol+1)*sizeof(BitCode)); //+1 because by convention the zero symbol means invalid

	ASSERT_RETNULL(toRet);
	//Descend the tree, and for all nodes with nonzero symbol, add bitcode to array.
	RecursiveBitAssign(NoBits, toRet, m_pHead);

	return toRet;
}

BOOL ValidateHuffman(HuffmanCode * pHuffmanCode)
{
	DWORD symbol;

	for(DWORD i = 1; i <= pHuffmanCode->m_nSymbolCount; i++)
	{
		int startbit = 0;

		BitCode codeWord = pHuffmanCode->FindCodeWord(i);

		if(codeWord.nBitCount == 0) continue;

		symbol = pHuffmanCode->FindSymbol( (BYTE *)&codeWord.bits, startbit, 
			codeWord.nBitCount);
		ASSERT_RETFALSE(symbol == i);
	}
	return TRUE;
}

BitCode FindHuffmanInvalidPad(BitCode *MappingArray, int nSize)
{
	//ignores zeroeth element of mapping array
	for(int i = 1; i <= nSize; i++)
	{
		if(MappingArray[i].nBitCount >= 8) return MappingArray[i];
	}
	return INVALID_BITCODE;
}

HuffmanCode * MakeHuffman( HuffmanNode ** nodes, DWORD nNodeCount, MEMORYPOOL * pPool)
{
	ASSERTX_RETNULL(nNodeCount < MAX_HUFFMAN_SIZE - 1, "Symbol table too large for huffman implementation.\n");

	HuffmanCode * toRet = (HuffmanCode*) MALLOC(pPool, sizeof(HuffmanCode) );
	ASSERT_RETNULL(toRet);

	Heap<HuffmanNode, MAX_HUFFMAN_SIZE, CompareByProbability> pqueue;

	toRet->m_nMaxSymbol = 0;
	for(DWORD i = 0; i < nNodeCount; i++)
	{
		ASSERT_CONTINUE(nodes[i]);
		pqueue.AddNode(nodes[i]);
		if(nodes[i]->symbol > toRet->m_nMaxSymbol) 
			toRet->m_nMaxSymbol = nodes[i]->symbol;
	}

	while(pqueue.GetItemCount() >= 2)
	{
		HuffmanNode * newnode = (HuffmanNode *) MALLOC(pPool, sizeof(HuffmanNode) );
		ASSERT_GOTO(newnode, make_huffman_error);
		
		newnode->symbol 	= 0;
		newnode->zero		= pqueue.PopMinNode();
		newnode->one		= pqueue.PopMinNode();
		newnode->probability	= newnode->zero->probability + newnode->one->probability;

		//newnode->zero->head	= newnode;
		//newnode->one ->head	= newnode;
		
		pqueue.AddNode(newnode);
	}


	toRet->m_pHead = pqueue.PopMinNode();
	ASSERT_GOTO(toRet->m_pHead, make_huffman_error);

	toRet->m_nSymbolCount = nNodeCount;
	//most likely, the code will also include a mapping of symbols to codewords.
	toRet->m_pMappingArray = toRet->MakeMappingArray(pPool);
	ASSERT_GOTO(toRet->m_pMappingArray, make_huffman_error);

	toRet->m_bcInvalidPad = FindHuffmanInvalidPad(toRet->m_pMappingArray, toRet->m_nSymbolCount);

#if ISVERSION(DEBUG_VERSION)
	ValidateHuffman(toRet);
#endif
	
	return toRet;

make_huffman_error:
	toRet->Free(pPool);
	pqueue.FreeNodes(pPool);
	FREE(pPool, toRet);
	return NULL;
}


 


