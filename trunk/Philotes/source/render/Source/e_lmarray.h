
/************************************************************************************
 *																					*
 *      .o@@@@@@@@@@@@@@@@@@@@@@@@@@)												*
 *      @@@@@'                .o@@@'												*
 *      '@@@.              .o@@@(													*
 *       '@@@.             o@@@@o													*
 *        '@@@.              _____    __________  _________     __________			*
 *          '@@@.     .-----|    ¬\__/    _____/_/   __   ¬\___\\       ¬/			*
 *           '@@@.   /   :. |      \______   . /  .: ______/      ..:   /			*
 *             '@@@. \   .: |      /     . ..: \  :.       \ \  ..:.:  /			*
 *              '@@@. \___________/\___________/\__________/\_________/mcl			*
 *               '@@@.																*
 *              @@@@@@@@ [ f ] · [ u ] · [ s ] · [ e ] · [ d ] · [ .de ]			*
 *                '@@@'																*
 *                 @@'																*
 *																					*
 *  coded by Questor / Avena								  Copyright Kai Jourdan	*
 *																					*
 ************************************************************************************

   Dynamic Array-Template

  V1.0       Creation
  V1.01      Fixed bug in Operator=
  V1.1       Insert, AddUnique and Remove included
  V1.2       SortedArray included
  V1.21		 Fixed Bug with c++-types
  */


#ifndef __ARRAY_H__
#define __ARRAY_H__

//#include "debug.h"

//#include <string.h>

namespace LMSPLINE
{;

template <class T> class array {
public:
	array(int startsize=20,int newgranularity=10) {
		size = 0;
		reserved = startsize;
		granularity = newgranularity;
		data = new T[reserved];
	};

	~array() {
		reserved = 0;
		size = 0;
		delete[] data;
	};

	bool isValidIndex(int i) {
		return i>=0 && i<size;
	}

	int getSize() {
		return size;
	}

	T* getData() {
		return data;
	}

	void insert(int position, const T& toInsert) {
		ASSERT(position>=0);
		ASSERT(position<=size);
		++size;
		if(size>=reserved) {			//get new memory
			T* tempBuffer = new T[size+granularity];

			memcpy(tempBuffer, data, sizeof(T)*position);									//Copy first part
			memcpy(&tempBuffer[position],&toInsert,sizeof(T));								//Copy new element
			memcpy(&tempBuffer[position+1], &data[position], sizeof(T)*(size-1-position));	//Copy last part

			delete[] data;
			data = tempBuffer;
			reserved += granularity;
			return;
		}
		memcpy(&data[position+1], &data[position], sizeof(T)*(size-position-1));
		memcpy(&data[position],&toInsert,sizeof(T));
	}

	void remove(int position, int length=1) {
		ASSERT(position >= 0);
		ASSERT(length > 0);
		ASSERT(position < size);
		memcpy(&data[position], &data[position+length],sizeof(T)*(size-position-length));
		size -= length;
	}

	void addUnique(const T& toInsert) {
		int i=0;
		while((i<size) && (data[i] != toInsert))
			i++;
		if(i==size)		//not found?
			add(toInsert);
	}

	void add(const T& toInsert) {
		++size;
		if(size>=reserved) {
			//get new memory
			T* tempBuffer = new T[size + granularity];
			memcpy(tempBuffer, data, sizeof(T)*(size-1));
			delete[] data;
			data = tempBuffer;
			reserved += granularity;
		}
		memcpy(&data[size-1],&toInsert,sizeof(T));
	}

	void compact(int extrasize=0) {
		ASSERT(extrasize>=0);
		if(size<=reserved-granularity) {
			//shrink memory
			T* tempBuffer = new T[size+extrasize];
			memcpy(tempBuffer,data,sizeof(T)*size);
			reserved = size + extrasize;
			delete[] data;
			data = tempBuffer;
		}
	}

	void empty() {
		size = 0;
		compact(granularity);
	}

	array& operator=(array& rhs) {
#ifdef _DEBUG
		if(this == &rhs) {
			trace("Argument at Operator= in array is THIS");
			return *this;
		}
#endif
		delete[] data;
		size = rhs.getSize();
		reserved = size + granularity;
		data = new T[reserved];

		memcpy(data, rhs.getData(), sizeof(T)*size);
		return *this;
	}

	T& operator[](const int rhs) {
		ASSERT(rhs>=0);
		ASSERT(rhs<size);
		return data[rhs];
	}

	const T &operator[](const int rhs) const {
		ASSERT(rhs>=0);
		ASSERT(rhs<size);
		return data[rhs];
	}


protected:
	int size;
	int reserved;
	int granularity;
	T* data;
};

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

template <class T> class sortedArray {
public:
	sortedArray(int startsize=20,int newgranularity=10) {
		size = 0;
		reserved = startsize;
		granularity = newgranularity;
		data = new T[reserved];
	};

	~sortedArray() {
		reserved = 0;
		size = 0;
		delete[] data;
	};

	bool isValidIndex(int i) {
		return i>=0 && i<size;
	}

	int getSize() {
		return size;
	}

	T* getData() {
		return data;
	}

	void remove(int position, int length=1) {
		ASSERT(position >= 0);
		ASSERT(length > 0);
		ASSERT(position < size);
		memcpy(&data[position], &data[position+length],sizeof(T)*(size-position-length));
		size -= length;
	}

	void addUnique(const T& toInsert) {
		int left=0;
		int right = size;
		bool found=false;
		int midpoint;
		while((left <= right) && (!found)) {
			midpoint = (left+right)/2;
			if(data[midpoint] == toInsert)
				found = true;
			else {
				if(toInsert < data[midpoint])
					right = midpoint-1;
				else
					left = midpoint+1;
			}
		}
		if(!found)
			insert(left,toInsert);
	}

	void add(const T& toInsert) {
		int left=0;
		int right = size;
		bool found=false;
		int midpoint;
		while((left <= right) && (!found)) {
			midpoint = (left+right)/2;
			if(data[midpoint] == toInsert)
				found = true;
			else {
				if(toInsert < data[midpoint])
					right = midpoint-1;
				else
					left = midpoint+1;
			}
		}
		if(found)
			insert(midpoint,toInsert);
		else
			insert(midpoint,toInsert);
	}

	void compact(int extrasize=0) {
		ASSERT(extrasize>=0);
		if(size<=reserved-granularity) {
			//shrink memory
			T* tempBuffer = new T[size+extrasize];
			memcpy(tempBuffer,data,sizeof(T)*size);
			reserved = size + extrasize;
			delete[] data;
			data = tempBuffer;
		}
	}

	int binarySearch(const T& toSearch) {
		//returns -1 if not in array
		int left=0;
		int right = size;
		bool found=false;
		int midpoint;
		while((left <= right) && (!found)) {
			midpoint = (left+right)/2;
			if(data[midpoint] == toSearch)
				found = true;
			else {
				if(toSearch < data[midpoint])
					right = midpoint-1;
				else
					left = midpoint+1;
			}
		}
		if(found)
			return midpoint;
		else
			return -1;
	}

	void empty() {
		size = 0;
		compact(granularity);
	}

	array& operator=(array& rhs) {
		if(this == &rhs) 
			return *this;
		delete[] data;
		size = rhs.getSize();
		reserved = size + granularity;
		data = new T[reserved];

		memcpy(data, rhs.getData(), sizeof(T)*size);
		return *this;
	}

	T& operator[](const int rhs) {
		ASSERT(rhs>=0);
		ASSERT(rhs<size);
		return data[rhs];
	}

	const T &operator[](const int rhs) const {
		ASSERT(rhs>=0);
		ASSERT(rhs<size);
		return data[rhs];
	}


protected:

	void insert(int position, const T& toInsert) {
		ASSERT(position>=0);
		ASSERT(position<=size);
		++size;
		if(size>=reserved) {			//get new memory
			T* tempBuffer = new T[size+granularity];

			memcpy(tempBuffer, data, sizeof(T)*position);									//Copy first part
			memcpy(&tempBuffer[position],&toInsert,sizeof(T));								//Copy new element
			memcpy(&tempBuffer[position+1], &data[position], sizeof(T)*(size-1-position));	//Copy last part

			delete[] data;
			data = tempBuffer;
			reserved += granularity;
			return;
		}
		memcpy(&data[position+1], &data[position], sizeof(T)*(size-position-1));
		memcpy(&data[position],&toInsert,sizeof(T));
	}


	int size;
	int reserved;
	int granularity;
	T* data;		
};

}; // LMSPLINE

#endif