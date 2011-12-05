// Frontend that handles both encryption and compression of a bytestream.
// Note that it's possible for compressed data to be either larger or
// smaller than the original data.  Make your buffers big enough for this.
// It is not possible with the current code for the compressed data to
// be more than 8 times as big as the uncompressed data.  (Generally
// it will be much smaller.)

// Note that every encode or decode changes the state of the stream
// cipher.  Packets thus must always be decoded in the correct order.
// This is intentional, to defeat packet replay and make reverse engineering
// by packet sniffing much more difficult.

#ifndef _ENCODER_H_
#define _ENCODER_H_

#ifndef _ARC4_H_
#include "arc4.h"
#endif

#ifndef _RANDOM_H_
#include "random.h"
#endif

#include "encoder_def.h"

struct Encoder
{
protected:
	static MEMORYPOOL *					m_pMemoryPool;
	static class Trie *					m_pTrie[COMPRESSION_TYPE_COUNT];
	static struct HuffmanCode *			m_pHuffmanCode[COMPRESSION_TYPE_COUNT];
	static struct UntranslatedSymbol *  m_pUntranslatedSymbolArray[COMPRESSION_TYPE_COUNT];

	ENCRYPTION_TYPE						m_eEncryptionType;
	COMPRESSION_TYPE					m_eCompressionType;
	ArcFourEncryptor					m_arcFour;
	RAND								m_rand;

	BOOL EncryptBuffer(
		BYTE * sourceanddest,
		unsigned int nSourceAndDestSize);

public:
	static BOOL InitCompressionGlobalForCompressionType(
		MEMORYPOOL * pPool,
		COMPRESSION_TYPE eCompressionType);
	static BOOL InitCompressionGlobal(MEMORYPOOL * pPool);

	static void FreeCompressionGlobalForCompressionType(
		MEMORYPOOL *pPool,
		COMPRESSION_TYPE eCompressionType);

	static void			FreeCompressionGlobal(MEMORYPOOL * pPool);
	inline static void	FreeCompressionGlobal()
	{
		FreeCompressionGlobal(m_pMemoryPool);
	}

	void Init(
		BYTE * Key, 
		size_t nBytes, 
		COMPRESSION_TYPE eCompressionType,
		ENCRYPTION_TYPE eEncryptionType = ENCRYPTION_TYPE_NONE);

	void Init(COMPRESSION_TYPE eCompressionType);//Init for no encryption.

	void Encoder::InitEncryption(BYTE *Key, size_t nBytes,
		ENCRYPTION_TYPE eEncryptionType); //turn on encryption on the fly.

	BOOL Encode(
		BYTE * dest,
		unsigned int nDestSize,
		BYTE * source,
		unsigned int nSourceSize,
		unsigned int &nEncodedBits);
	BOOL Decode(
		BYTE * dest,
		unsigned int nDestSize,
		BYTE * source, 
		unsigned int nSourceSize,
		unsigned int &nDecodedBits);

	BOOL Encode( //Front end for encryption without compression
		BYTE * sourceanddest,
		unsigned int nSourceAndDestSize);

	inline BOOL Decode( //Front end for encryption without compression
		BYTE * sourceanddest,
		unsigned int nSourceAndDestSize)
	{	//For all currently implemented ciphers, encryption is the same as decryption: just XOR.
		return Encode(sourceanddest, nSourceAndDestSize);
	}

	inline COMPRESSION_TYPE GetCompressionType() {return m_eCompressionType;}
	inline ENCRYPTION_TYPE GetEncryptionType() {return m_eEncryptionType;}

	Encoder(
		BYTE * Key, 
		size_t nBytes,
		COMPRESSION_TYPE eCompressionType,
		ENCRYPTION_TYPE eEncryptionType = ENCRYPTION_TYPE_NONE)
	{
		Init(Key, nBytes, eCompressionType, eEncryptionType);
	}
	Encoder(
		COMPRESSION_TYPE eCompressionType = COMPRESSION_TYPE_NONE)
	{
		Init(eCompressionType);
	}

	inline static MEMORYPOOL* GetMemoryPool() {return m_pMemoryPool;}
};
#endif