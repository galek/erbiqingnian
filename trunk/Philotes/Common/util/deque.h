// Added by Li  2011-11-25 0:24:19
//	(C) 2012 PhiloLabs

#pragma once

#include "core/types.h"
#include "memory/stl_allocator.h"

#include <deque>

//------------------------------------------------------------------------------
namespace Philo
{

template <typename T, typename A = 
	Memory::STLAllocator<T, Memory::STLAllocPolicy<Memory::ObjectArrayHeap> > >
struct Deque 
{ 
	typedef typename std::deque<T, A> type;    
}; 

}