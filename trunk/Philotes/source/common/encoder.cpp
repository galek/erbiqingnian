
#include "encoder.h"
#include "bitmanip.h"
#include "huffman.h"
#include "trie.h"

MEMORYPOOL *		Encoder::m_pMemoryPool = NULL;
Trie * 				Encoder::m_pTrie[COMPRESSION_TYPE_COUNT] =
{ NULL, NULL, NULL, NULL, NULL};
HuffmanCode * 		Encoder::m_pHuffmanCode[COMPRESSION_TYPE_COUNT] =
{ NULL, NULL, NULL, NULL, NULL};
UntranslatedSymbol *Encoder::m_pUntranslatedSymbolArray[COMPRESSION_TYPE_COUNT] =
{ NULL, NULL, NULL, NULL, NULL};

#define MAX_ENCODED_SOURCE_SIZE (1 << 16)
#define MAX_SYMBOL_VALUE (1<<14) //this number is also seen in huffmanDictionaryOptimizer.cpp

const TCHAR * compressionSpecificationFile[] =
{
	NULL,
	"data\\compression\\gameStoC.cmp",
	"data\\compression\\gameCtoS.cmp",
	"data\\compression\\chatStoC.cmp",
	"data\\compression\\chatCtoS.cmp"
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL Encoder::InitCompressionGlobalForCompressionType(
	MEMORYPOOL * pPool,
	COMPRESSION_TYPE eCompressionType)
{
	ASSERT_RETFALSE(eCompressionType < COMPRESSION_TYPE_COUNT);
	if(compressionSpecificationFile[eCompressionType] == NULL)
	{
		m_pTrie[eCompressionType] = NULL;
		m_pHuffmanCode[eCompressionType] = NULL;
		m_pUntranslatedSymbolArray[eCompressionType] = NULL;
		return TRUE;
	}

	HANDLE file = FileOpen(compressionSpecificationFile[eCompressionType],
		GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);

	ASSERT_RETFALSE(file != INVALID_HANDLE_VALUE);

	ASSERTX(m_pTrie[eCompressionType] == NULL, "Compression already initialized!");

	m_pTrie[eCompressionType] = (Trie *)MALLOC(pPool, sizeof(m_pTrie));
	ASSERT_GOTO(m_pTrie[eCompressionType], InitCompression_trie_allocate_error);

	ASSERT_GOTO(m_pTrie[eCompressionType]->Init(pPool) , InitCompression_trie_init_error);

	//Allocated the untranslated symbol array
	ASSERT(m_pUntranslatedSymbolArray[eCompressionType] == NULL);

	m_pUntranslatedSymbolArray[eCompressionType] = 
		(UntranslatedSymbol *)MALLOCZ(pPool, (MAX_SYMBOL_VALUE+1)*sizeof(UntranslatedSymbol) );

	ASSERT_GOTO(m_pUntranslatedSymbolArray[eCompressionType], InitCompression_symbolarray_allocate_error);

	unsigned long frequencyArray[MAX_SYMBOL_VALUE+1];
	HuffmanNode * HuffmanNodes[MAX_SYMBOL_VALUE];

	memclear(HuffmanNodes, sizeof(HuffmanNodes));


	DWORD nBytesRead;
	DWORD nSymbolCount;
	ASSERT(ReadFile(file,
		&nSymbolCount,
		sizeof(nSymbolCount),
		&nBytesRead,
		NULL));

	DWORD ii = 1;
	while(ii <= nSymbolCount)
	{
		ASSERT(ReadFile(file, 
			&m_pUntranslatedSymbolArray[eCompressionType][ii].size,
			sizeof(m_pUntranslatedSymbolArray[eCompressionType][ii].size),
			&nBytesRead, NULL));
		if(nBytesRead == 0) break;

		ASSERT_GOTO(m_pUntranslatedSymbolArray[eCompressionType][ii].size <= 
			sizeof(m_pUntranslatedSymbolArray[eCompressionType][ii].bbuff), InitCompression_file_read_error);
		ASSERT(ReadFile(file,
			m_pUntranslatedSymbolArray[eCompressionType][ii].bbuff,
			m_pUntranslatedSymbolArray[eCompressionType][ii].size,
			&nBytesRead, NULL));
		ASSERT_BREAK(nBytesRead > 0);

		nBytesRead = ReadFile(file,
			&frequencyArray[ii],
			sizeof(frequencyArray[ii]),
			&nBytesRead, NULL);
		ASSERT_BREAK(nBytesRead > 0);

		ASSERT_GOTO(ii == m_pTrie[eCompressionType]->AddWord(
			m_pUntranslatedSymbolArray[eCompressionType][ii].bbuff,
			m_pUntranslatedSymbolArray[eCompressionType][ii].size ),
			InitCompression_file_read_error);

		HuffmanNodes[ii-1] = (HuffmanNode *) MALLOCZ(pPool, sizeof(HuffmanNode) );
		ASSERT_GOTO(HuffmanNodes[ii-1], InitCompression_file_read_error);
		HuffmanNodes[ii-1]->probability = frequencyArray[ii];
		HuffmanNodes[ii-1]->symbol = ii;

		ii++;
		if(ii > MAX_SYMBOL_VALUE) break;
	}

	
	m_pUntranslatedSymbolArray[eCompressionType] =
		(UntranslatedSymbol *)REALLOC(pPool, m_pUntranslatedSymbolArray[eCompressionType], ii*sizeof(UntranslatedSymbol));
	ASSERT_GOTO(m_pUntranslatedSymbolArray[eCompressionType], InitCompression_file_read_error);

	m_pHuffmanCode[eCompressionType] = MakeHuffman(HuffmanNodes, ii-1, pPool);
	ASSERT_GOTO(m_pHuffmanCode, InitCompression_make_huffman_error);

	FileClose(file);

	return TRUE;
InitCompression_file_read_error:

	for(DWORD jj = 0; jj < ii-2; jj++) if(HuffmanNodes[jj]) FREE(pPool, HuffmanNodes[jj]);
InitCompression_make_huffman_error:
	FREE(pPool, m_pUntranslatedSymbolArray[eCompressionType]);
	m_pUntranslatedSymbolArray[eCompressionType] = NULL;

InitCompression_symbolarray_allocate_error:
	m_pTrie[eCompressionType]->Free();
InitCompression_trie_init_error:
	FREE(pPool, m_pTrie[eCompressionType]);
	m_pTrie[eCompressionType] = NULL;
InitCompression_trie_allocate_error:
	FileClose(file);	

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL Encoder::InitCompressionGlobal(MEMORYPOOL * pPool)
{
	BOOL bSuccess = TRUE;
	for(int i = 0; i < COMPRESSION_TYPE_COUNT; i++)
	{
		bSuccess &= InitCompressionGlobalForCompressionType(pPool, (COMPRESSION_TYPE)i);
	}
	m_pMemoryPool = pPool;

	return bSuccess;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Encoder::FreeCompressionGlobalForCompressionType(MEMORYPOOL * pPool, COMPRESSION_TYPE eCompressionType)
{
	if(m_pHuffmanCode[eCompressionType] == NULL ||
		m_pUntranslatedSymbolArray[eCompressionType] == NULL ||
		m_pTrie[eCompressionType] == NULL)
	{
		ASSERT (m_pHuffmanCode[eCompressionType] == NULL);
		ASSERT (m_pUntranslatedSymbolArray[eCompressionType] == NULL);
		ASSERT (m_pTrie[eCompressionType] == NULL);
		return;
	}
	m_pHuffmanCode[eCompressionType]->Free(pPool);
	FREE(pPool, m_pHuffmanCode[eCompressionType]);
	m_pHuffmanCode[eCompressionType] = NULL;
	m_pTrie[eCompressionType]->Free();

	FREE(pPool, m_pUntranslatedSymbolArray[eCompressionType]);
	m_pUntranslatedSymbolArray[eCompressionType] = NULL;

	FREE(pPool, m_pTrie[eCompressionType]);
	m_pTrie[eCompressionType] = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Encoder::FreeCompressionGlobal(MEMORYPOOL * pPool)
{
	for(int i = 0; i < COMPRESSION_TYPE_COUNT; i++)
	{
		FreeCompressionGlobalForCompressionType(pPool, COMPRESSION_TYPE(i));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Encoder::InitEncryption(BYTE *Key, size_t nBytes,
							 ENCRYPTION_TYPE eEncryptionType)
{
	switch(eEncryptionType)
	{
	case ENCRYPTION_TYPE_NONE:
		break;
	case ENCRYPTION_TYPE_ARC4:
		m_arcFour.Init(Key, nBytes);
		break;
	case ENCRYPTION_TYPE_PRNG8:
		{
			DWORD seed1 = 0;
			DWORD seed2 = 0;
			if(nBytes >= sizeof(DWORD)) 
			{
				seed1 = *((DWORD*)Key);
				seed2 = *((DWORD*)(Key+nBytes-sizeof(DWORD)));
			}

			//We can't allow zero seeds--this will cause RAND to timer-init itself.
			if(seed1 == 0) seed1 = 12345;

			RandInit(m_rand, seed1, seed2);

			break;
		}
	default:
		ASSERTV_MSG("Unhandled encryption type %d in encoder", m_eEncryptionType);
	}
	m_eEncryptionType = eEncryptionType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Encoder::Init(BYTE * Key, size_t nBytes, 
				   COMPRESSION_TYPE eCompressionType,
				   ENCRYPTION_TYPE eEncryptionType)
{
	ASSERT_RETURN(eCompressionType < COMPRESSION_TYPE_COUNT);
	m_eCompressionType = eCompressionType;
	InitEncryption(Key, nBytes, eEncryptionType);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void Encoder::Init(COMPRESSION_TYPE eCompressionType)
{
	m_eCompressionType = eCompressionType;
	m_eEncryptionType = ENCRYPTION_TYPE_NONE;
}

//----------------------------------------------------------------------------
// Protected function, since this may alter the internal state of the
// encryptor.  Should be used in the context of encode and decode.
//
// Currently, all ciphers are symmetric with respect to encryption and
// decryption.  Since it's a simple XOR, the same algorithm is used in both.
// If this changes, we need to change some EncryptBuffers to DecryptBuffers.
//
// Currently, all ciphers return the same number of bytes as go in.
// No padding or alignment is needed.  We will likely always use stream
// ciphers which satisfy this property.
//----------------------------------------------------------------------------
BOOL Encoder::EncryptBuffer(
	BYTE * sourceanddest, 
	unsigned int nSourceAndDestSize)
{
	ASSERT_RETFALSE(sourceanddest);
	switch(m_eEncryptionType)
	{
	case ENCRYPTION_TYPE_NONE:
		return TRUE;
	case ENCRYPTION_TYPE_ARC4:
		for(unsigned int i = 0; i < nSourceAndDestSize; i++)
		{
			sourceanddest[i] ^= m_arcFour.GetByte();
		}
		return TRUE;
	case ENCRYPTION_TYPE_PRNG8:
		for(unsigned int i = 0; i < nSourceAndDestSize; i++)
		{
			sourceanddest[i] ^= (BYTE)RandGetNum(m_rand);
		}
		return TRUE;
	default:
		ASSERTV_MSG("Unhandled encryption type %d in encoder", m_eEncryptionType);
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//Note that it is possible for the encoded data to be significantly larger
// or smaller than the original.
//----------------------------------------------------------------------------
BOOL Encoder::Encode(
	BYTE * dest, 
	unsigned int nDestSize, 
	BYTE * source,
	unsigned int nSourceSize,
	unsigned int &nEncodedBytes)
{ //returns FALSE if we overrun the destination buffer size
	ASSERT_RETFALSE(dest);
	ASSERT_RETFALSE(source);

	BIT_BUF bitbuf(dest, nDestSize);

	if(m_pTrie[m_eCompressionType] == NULL || m_pHuffmanCode[m_eCompressionType] == NULL)
	{
		ASSERT(m_pTrie[m_eCompressionType] == NULL && m_pHuffmanCode[m_eCompressionType] == NULL);
		ASSERTX_RETFALSE(nDestSize >= nSourceSize, "Source buffer larger than destination buffer.\n");
		ASSERT_RETFALSE(MemoryCopy(dest, nDestSize, source, nSourceSize));
		nEncodedBytes = nSourceSize;
		goto Encode_Encryption;
	}

	DWORD symbol = m_pTrie[m_eCompressionType]->FindWord(source, nSourceSize);
	while(symbol)
	{//encode it by running it through the trie and doing an huffmanmappingarray lookup
		ASSERT_BREAK( bitbuf.PushBuf(
			m_pHuffmanCode[m_eCompressionType]->m_pMappingArray[symbol].bbits,
			m_pHuffmanCode[m_eCompressionType]->m_pMappingArray[symbol].nBitCount
			BBD_FILE_LINE) );

		symbol = m_pTrie[m_eCompressionType]->FindWord(source, nSourceSize);
	}
	nEncodedBytes = bitbuf.GetLen();
	int nExtraBits = nEncodedBytes*8 - bitbuf.GetCursor();
	ASSERT(nExtraBits >= 0);

	if(nExtraBits > 0)//Not ending on a byte
	{	
		//add a pad to finish off the last byte
		ASSERT(nExtraBits < 8);
		
		bitbuf.PushBuf(m_pHuffmanCode[m_eCompressionType]->m_bcInvalidPad.bbits, nExtraBits  BBD_FILE_LINE);
	}
	ASSERT(nEncodedBytes == bitbuf.GetLen());

Encode_Encryption:
	EncryptBuffer(dest, nEncodedBytes);

	ASSERT(nEncodedBytes <= MAX_ENCODED_SOURCE_SIZE);
	ASSERTX(nEncodedBytes <= nDestSize, "Buffer overrun in message compression!");

	return TRUE;
}

//Note that it is possible for the decoded data to be significantly larger or smaller than the encoded.
BOOL Encoder::Decode(
	BYTE * dest, 
	unsigned int nDestSize, 
	BYTE * source,
	unsigned int nSourceSize,
	unsigned int &nDecodedBytes)
{ //returns FALSE if we overrun the destination buffer size
	ASSERT_RETFALSE(dest);
	ASSERT_RETFALSE(source);

	if(m_pTrie[m_eCompressionType] == NULL || m_pHuffmanCode[m_eCompressionType] == NULL)
	{
		ASSERT(m_pTrie[m_eCompressionType] == NULL && m_pHuffmanCode[m_eCompressionType] == NULL);
		ASSERTX_RETFALSE(nDestSize >= nSourceSize, "Source buffer larger than destination buffer.\n");
		nDecodedBytes = nSourceSize;
		ASSERT_RETFALSE(MemoryCopy(dest, nDestSize, source, nSourceSize));
		EncryptBuffer(dest, nSourceSize);
		return TRUE;
	}

	BYTE bIntermediate[MAX_ENCODED_SOURCE_SIZE];
	BYTE *bUnencrypted = bIntermediate;

	if(m_eEncryptionType != ENCRYPTION_TYPE_NONE)
	{
		ASSERT_RETFALSE(MemoryCopy(bIntermediate, MAX_ENCODED_SOURCE_SIZE, source, nSourceSize));
		EncryptBuffer(bIntermediate, nSourceSize);
	}
	else bUnencrypted = source;

	int startbit = 0;

	BYTE_BUF bbuff(dest, nDestSize);

	DWORD symbol;
	while(1) 
	{
		symbol = m_pHuffmanCode[m_eCompressionType]->FindSymbol(bUnencrypted, startbit, nSourceSize*BITS_PER_BYTE);
		if(symbol == 0) break;

		ASSERTX_CONTINUE(symbol <= m_pTrie[m_eCompressionType]->GetMaxSymbol(),"Symbol not in trie!")
		//push the translated symbol onto the byte buffer output.
		ASSERT_BREAK(bbuff.PushBuf(
			m_pUntranslatedSymbolArray[m_eCompressionType][symbol].bbuff,
			m_pUntranslatedSymbolArray[m_eCompressionType][symbol].size) );
	}
	nDecodedBytes = bbuff.GetLen();
	ASSERTX(nDecodedBytes <= nDestSize, "Buffer overrun in message decompression!");

	return TRUE;
}

BOOL Encoder::Encode( //Front end for encryption without compression
	BYTE * sourceanddest,
	unsigned int nSourceAndDestSize)
{
	ASSERT_RETFALSE(sourceanddest);
	ASSERT_RETFALSE(m_eCompressionType == COMPRESSION_TYPE_NONE);

	return EncryptBuffer(sourceanddest, nSourceAndDestSize);
}

void EncoderEncryptionTest()
{
	Encoder::InitCompressionGlobal(NULL);

	for(int nn = 0; nn < COMPRESSION_TYPE_COUNT; nn++)
	{
		Encoder Encode( (BYTE *)"Some random key", 15, COMPRESSION_TYPE(nn), ENCRYPTION_TYPE_ARC4);
		//Encoder Decode( (BYTE *)"Some random key", 15, COMPRESSION_TYPE(nn), TRUE);
		Encoder Decode( (COMPRESSION_TYPE)(nn));
		Decode.InitEncryption( (BYTE *)"Some random key", 15, ENCRYPTION_TYPE_ARC4);

		BYTE bArbitraryData[64] = "jeriojwrur8ugfd09fu0gfud09eru04ujoi4hjiorhjiogfjfxjnlchjnlfu894";
									//"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

		BYTE bEncodedBuff[512]; //with compression, this needs to be larger.
		BYTE bDecodedBuff[64];

		unsigned int nEncodedBytes, nDecodedBytes;
		ASSERT(Encode.Encode(bEncodedBuff, 512, bArbitraryData, 64, nEncodedBytes) );

		if (COMPRESSION_TYPE(nn) == COMPRESSION_TYPE_NONE) ASSERT(nEncodedBytes == 64);

		Decode.Decode(bDecodedBuff, 64, bEncodedBuff, nEncodedBytes, nDecodedBytes);

		ASSERT(nDecodedBytes == 64);

		for(int i = 0; i < 64; i++) ASSERT(bDecodedBuff[i] == bArbitraryData[i]);
	}
	//Test simple single buffer encryption.
	{
		Encoder Encode( (BYTE *)"Some random key", 15, COMPRESSION_TYPE_NONE, ENCRYPTION_TYPE_PRNG8);
		//Encoder Decode( (BYTE *)"Some random key", 15, COMPRESSION_TYPE(nn), TRUE);
		Encoder Decode(COMPRESSION_TYPE_NONE);
		Decode.InitEncryption( (BYTE *)"Some random key", 15, ENCRYPTION_TYPE_PRNG8);

		BYTE bArbitraryData[64] = "jeriojwrur8ugfd09fu0gfud09eru04ujoi4hjiorhjiogfjfxjnlchjnlfu894";
		BYTE bCorrectData[64] =   "jeriojwrur8ugfd09fu0gfud09eru04ujoi4hjiorhjiogfjfxjnlchjnlfu894";

		Encode.Encode(bArbitraryData, 64);
		Decode.Decode(bArbitraryData, 64);
		for(int i = 0; i < 64; i++) ASSERT(bArbitraryData[i] == bCorrectData[i]);
	}

	Encoder::FreeCompressionGlobal();
}