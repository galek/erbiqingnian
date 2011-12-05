//----------------------------------------------------------------------------
// facelist.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once 

#include "boundingbox.h"
#include "array.h"

class CFaceList
{

	// FACE_LIST_NODE
	//
	struct FACE_LIST_NODE
	{
		BOUNDING_BOX			Bounds;				// The bounding box which encloses the face
		VECTOR					Normal;
		int						VertexIndexA;
		int						VertexIndexB;
		int						VertexIndexC;
		int						MaterialIndex;
	};

	typedef BUCKET_ARRAY<FACE_LIST_NODE, 256> FACE_LIST_NODE_ARRAY;
	typedef BUCKET_ARRAY<VECTOR, 256> VERTEX_ARRAY; 


	public:

		CFaceList(MEMORYPOOL* allocator);
		~CFaceList();

	public:
	

		unsigned int 						GetVertexCount() { return m_VertexCount; }
		const BOUNDING_BOX*					GetBounds(unsigned int faceIndex) const;
		void								AddVertex(const VECTOR& vertex);	
		unsigned int						AddFace(int vertexIndexA, int vertexIndexB, int vertexIndexC, int materialIndex);
		bool								RayCollision(const SIMPLE_DYNAMIC_ARRAY<unsigned int>& faceIndices, const VECTOR& start, 
												const VECTOR& end, const BOUNDING_BOX& rayBounds, VECTOR& impactPoint, 
												VECTOR* impactNormal, int& faceMaterialIndex, float& bestDistance, int ignoreMaterial = -1);

	private:
	
		bool								PointIn(const VECTOR& point, const FACE_LIST_NODE* face);			
		const FACE_LIST_NODE*				GetFace(unsigned int faceIndex) const;

	private:

		MEMORYPOOL*							m_Allocator;
		
		BOUNDING_BOX						m_Bounds;
		
		VERTEX_ARRAY						m_Vertices;
		unsigned int						m_VertexCount;
		
		FACE_LIST_NODE_ARRAY				m_Faces;
		unsigned int						m_FaceCount;
};
