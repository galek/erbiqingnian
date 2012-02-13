//------------------------------------------------------------------------------
//  stringbuffer.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------

#include "util/stringbuffer.h"

#include <string.h>

namespace Philo
{

//------------------------------------------------------------------------------
/**
*/
StringBuffer::StringBuffer() :
    chunkSize(0), 
    curPointer(0)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
StringBuffer::~StringBuffer()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: this method must be called before any threads are spawned.
*/
void
StringBuffer::Setup(SizeT size)
{
    ph_assert(!this->IsValid());
    ph_assert(size > 0);
    this->chunkSize = size;    
    this->AllocNewChunk();
}

//------------------------------------------------------------------------------
/**
*/
void
StringBuffer::Discard()
{
    ph_assert(this->IsValid());
    IndexT i;
    for (i = 0; i < this->chunks.Size(); i++)
    {
		free(chunks[i]);
        this->chunks[i] = 0;
    }
    this->chunks.Clear();
    this->chunkSize = 0;
    this->curPointer = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
StringBuffer::AllocNewChunk()
{
	char* newChunk = (char*) malloc (this->chunkSize);
    this->chunks.Append(newChunk);
    this->curPointer = newChunk;
}

//------------------------------------------------------------------------------
/**
    Copies a string to the end of the string buffer, returns pointer to
    copied string.
*/
const char*
StringBuffer::AddString(const char* str)
{
    ph_assert(0 != str);
    ph_assert(this->IsValid());

    // get string length, must be less then chunk size
    SizeT strLength = strlen(str) + 1;
    ph_assert(strLength < this->chunkSize);

    // check if a new buffer must be allocated
    if ((this->curPointer + strLength) >= (this->chunks.Back() + this->chunkSize))
    {
        #if NEBULA3_ENABLE_GLOBAL_STRINGBUFFER_GROWTH
        this->AllocNewChunk();
        #else
        ph_error("String buffer full when adding string '%s' (string buffer growth is disabled)!\n", str);
        #endif
    }

    // copy string into string buffer
    char* dstPointer = this->curPointer;
    strcpy(dstPointer, str);
    this->curPointer += strLength;
    return dstPointer;
}

} // namespace Philo
