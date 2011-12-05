//----------------------------------------------------------------------------
// trie.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _TRIE_H_
#define _TRIE_H_

#define INVALID_CODEWORD DWORD(0)
#define BRANCHNUM 16

//Design decision: keep total compression memory in the hundred k range.

//To save memory, we use a branching factor of 16, splitting bytes in half.

//----------------------------------------------------------------------------
// Trie
// Simple trie of nibbles (4 bits).
// Intended use: encoding streams of byte buffs.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
#define MAX_SYMBOL_SIZE 16

struct UntranslatedSymbol
{//Symbols are implicit within the trie.  This is the explicit form.
	BYTE size;
	BYTE bbuff[MAX_SYMBOL_SIZE];

	BOOL SetToSum(const UntranslatedSymbol &a, const UntranslatedSymbol &b)
	{
		if(a.size + b.size > MAX_SYMBOL_SIZE) return FALSE;
		size = a.size + b.size;
		ASSERT_RETFALSE(MemoryCopy(bbuff, size, a.bbuff, a.size));
		ASSERT_RETFALSE(MemoryCopy(bbuff+a.size, b.size, b.bbuff, b.size));
		return TRUE;
	}
};

struct TrieNode
{
	DWORD codeword;
	TrieNode * branches[BRANCHNUM];
};

class Trie
{
private:
	void RecursiveFree(TrieNode * trie, MEMORYPOOL * pPool)
	{
		if (trie == NULL) return;

		for(int i = 0; i < BRANCHNUM; i++)
			RecursiveFree(trie->branches[i], pPool);

		FREE(pPool, trie);
	}

protected:
	unsigned int m_nEntries;
	unsigned int m_nNodes;
	TrieNode * m_pHead;
	MEMORYPOOL * m_pPool;
	unsigned int m_nMaxSymbol;

public:

	BOOL Init(MEMORYPOOL *mempool)
	{
		m_nMaxSymbol = 0;
		m_nEntries = 0;
		m_nNodes = 1;
		m_pPool = mempool;
		m_pHead = (TrieNode *)MALLOCZ(m_pPool, sizeof(TrieNode) );
		if(m_pHead)
			return TRUE;
		else
			return FALSE;	
	}
	void Free()
	{
		RecursiveFree(m_pHead, m_pPool);
		m_nEntries = 0;
		m_nNodes = 0;		
	}
	Trie(MEMORYPOOL *pPool) {Init(pPool);}
	~Trie() {Free();}


	//all the above functions wrapped
	DWORD FindWord(BYTE *& string, int &nLength)
	{
	 //Finds the longest word starting at the beginning, moves the string forward.
		TrieNode * trie = m_pHead;
		ASSERTX_RETZERO(trie, "Trie not initialized or already freed.");

		DWORD codeword = 0;
		int nCodePos = 0;
		int nPos = 0;

		while(trie && (nPos < nLength*2) )
		{
			if (trie->codeword != 0) codeword = trie->codeword, nCodePos = nPos;

			BYTE branchnum = string[nPos/2];
			if (nPos %2) branchnum &= 0x0f; //specific code to 16-branching case
			else branchnum = branchnum >> 4;

		
			trie = trie->branches[branchnum];
			nPos++;
		}
		if (trie && trie->codeword != 0) codeword = trie->codeword, nCodePos = nPos;
	
		ASSERT(nCodePos % 2 == 0);
		nLength -= nCodePos/2; //16 branch specific			
		string += nCodePos/2;

		return codeword;
	}
	inline DWORD FindWord(BYTE *& string, unsigned int &nLength)
	{
		ASSERT_RETZERO (int(nLength) >= 0);
		return FindWord(string, (int &)nLength);
	}

	DWORD AddWord(BYTE * string, int nLength, DWORD nSymbol = 0) 
		//note that it is possible for several strings to map to the same symbol if you pass in a symbol number.
	{//Returns: codeword, or zero for failure.  If it fails, you probably want to disconnect since
	// communication will get out of sync.
	//descend the trie till the place of our desired codeword, adding nodes where needed.
		int nPos = 0;

		if(nSymbol == 0) nSymbol = m_nMaxSymbol+1;

		TrieNode * trie = m_pHead;
		ASSERTX_RETZERO(trie, "Trie not initialized or already freed.");

		while(nPos < nLength*2 )
		{

			BYTE branchnum = string[nPos/2];
			if (nPos %2) branchnum &= 0x0f; //specific code to 16-branching case
			else branchnum = branchnum >> 4;

		
			if( (trie->branches[branchnum]) == NULL )
			{
				trie->branches[branchnum] = (TrieNode* ) MALLOCZ(m_pPool, sizeof(TrieNode) );
				m_nNodes++;
			}

			trie = trie->branches[branchnum];
			if(!trie) 
			{
				m_nNodes--;
				return FALSE;
			} 

			nPos++;
		}
		ASSERT_RETZERO(trie->codeword == 0); //we shouldn't be adding words that already exist.
		//if(trie->codeword != 0) return NULL;

		m_nEntries++;

		trie->codeword = nSymbol;
		if(m_nMaxSymbol < nSymbol) m_nMaxSymbol = nSymbol;
		
		return trie->codeword;
	}

	unsigned int GetEntryCount()	{return m_nEntries;}
	unsigned int GetNodeCount()		{return m_nNodes;}
	unsigned int GetMaxSymbol()		{return m_nMaxSymbol;}

private:
	BOOL RecursiveDelete(BYTE *string, int nLength, int nPos, TrieNode* pNode)
	{
		//Follow the next node.  If it is deleted, see if it is an only child, and delete this
		//then return true.  Otherwise, return false.

		ASSERTX_RETFALSE(pNode, "Symbol is not in trie!");
		//Base condition: if we are at the end of the string-- if have no children, delete
		//and return true, else zero the codeword and return false.
		if(nPos == nLength*2)
		{
			BOOL empty = TRUE;
			for(int i = 0; i < BRANCHNUM; i++) if(pNode->branches[i]) empty = FALSE;

			ASSERTX_RETFALSE(pNode->codeword != 0, "Symbol is not in trie!");
			m_nEntries--;

			if(empty)
			{
				FREE(m_pPool, pNode);
				m_nNodes--;
				return TRUE;
			}
			else
			{
				pNode->codeword = 0;
				return FALSE;
			}
		}

		BYTE branchnum = string[nPos/2];
		if (nPos %2) branchnum &= 0x0f; //specific code to 16-branching case
		else branchnum = branchnum >> 4;

		TrieNode * pNewNode = pNode->branches[branchnum];
		if(!RecursiveDelete(string, nLength, nPos+1, pNewNode)) return FALSE;
		else
		{
			pNode->branches[branchnum] = NULL;
			int nChildren = 0;
			for(int i = 0; i < BRANCHNUM; i++) if(pNode->branches[i]) nChildren++;

			if (nChildren == 0 && pNode->codeword == 0)
			{
				FREE(m_pPool, pNode);
				m_nNodes--;
				return TRUE;
			}
			else return FALSE;
		}
	}
public:
	void Trie::DeleteWord(BYTE *string, int nLength)
	{
		RecursiveDelete(string, nLength, 0, this->m_pHead);
		if(m_nEntries == 0) m_pHead = NULL;
	}
};


#endif