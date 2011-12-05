//----------------------------------------------------------------------------
// facelist.cpp
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "facelist.h"


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CFaceList Constructor
//
// Parameters
//	allocator : The memory allocator that will be used for all internal allocations
//
/////////////////////////////////////////////////////////////////////////////
CFaceList::CFaceList(MEMORYPOOL* allocator) :
	m_Allocator(allocator),
	m_VertexCount(0),
	m_FaceCount(0)
{
	m_Bounds.vMin.Set(0.0f, 0.0f, 0.0f);
	m_Bounds.vMax.Set(0.0f, 0.0f, 0.0f);

	m_Vertices.Init(allocator);
	m_Faces.Init(allocator);	
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CFaceList Destructor
//
/////////////////////////////////////////////////////////////////////////////
CFaceList::~CFaceList()
{
	m_Vertices.Free();
	m_Faces.Free();
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBounds
//
// Parameters
//	faceIndex : The index of the face for which the bounds will be returned
//
// Returns
//	A pointer to the bounding box for the face, for NULL if the index is out of bounds
//
/////////////////////////////////////////////////////////////////////////////
const BOUNDING_BOX* CFaceList::GetBounds(unsigned int faceIndex) const
{
	if(faceIndex >= m_FaceCount)
	{
		return NULL;
	}
	else
	{
		return &m_Faces[faceIndex].Bounds;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFace
//
// Parameters
//	faceIndex : The index of the face for which the face list node will be returned
//
// Returns
//	A pointer to the face list node for the face, for NULL if the index is out of bounds
//
/////////////////////////////////////////////////////////////////////////////
const CFaceList::FACE_LIST_NODE* CFaceList::GetFace(unsigned int faceIndex) const
{
	if(faceIndex >= m_FaceCount)
	{
		return NULL;
	}
	else
	{
		return &m_Faces[faceIndex];
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetFace
//
// Parameters
//	vertex : A reference to the vertex to add
//
/////////////////////////////////////////////////////////////////////////////
void CFaceList::AddVertex(const VECTOR& vertex)		
{
	// Adjust the bounding box for the face list
	//
	if(m_VertexCount == 0 )
	{
		m_Bounds.vMin = vertex;
		m_Bounds.vMax = vertex;
	}
	else
	{
		ExpandBounds(m_Bounds.vMin, m_Bounds.vMax, vertex);
	}

	++m_VertexCount;
	m_Vertices.PushBack(MEMORY_FUNC_FILELINE(vertex));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	AddFace
//
// Parameters
//	vertexIndexA : The vertex index of the first vertex for the face
//	vertexIndexB : The vertex index of the second vertex for the face
//	vertexIndexC : The vertex index of the third vertex for the face
//	materialIndex : The material index for the face
//
// Returns
//	The index of the face that was added
//
// Remarks
//	The vertices must have already been added
//
/////////////////////////////////////////////////////////////////////////////
unsigned int CFaceList::AddFace(int vertexIndexA, int vertexIndexB, int vertexIndexC, int materialIndex)
{
	// Generate a bounding box for the face
	//
	BOUNDING_BOX faceBounds = {m_Vertices[vertexIndexA], m_Vertices[vertexIndexA]};
	ExpandBounds(faceBounds.vMin, faceBounds.vMax, m_Vertices[vertexIndexB]);
	ExpandBounds(faceBounds.vMin, faceBounds.vMax, m_Vertices[vertexIndexC]);

	// Generate a normal for the face
	//
	VECTOR	TempVEC3D1	( 0, 0, 0 );
	VECTOR	TempVEC3D2	( 0, 0, 0 );
	VECTOR	Normal		( 0, 0, 0 );
	TempVEC3D1 = m_Vertices[vertexIndexC];
	TempVEC3D2 = m_Vertices[vertexIndexB];
	TempVEC3D1 -= m_Vertices[vertexIndexB];
	TempVEC3D2 -= m_Vertices[vertexIndexA];

	VectorCross( Normal, TempVEC3D1, TempVEC3D2 );
	VectorNormalize( Normal, Normal );

	// Assemble a face node
	//
	FACE_LIST_NODE faceNode;
	faceNode.VertexIndexA = vertexIndexA;
	faceNode.VertexIndexB = vertexIndexB;
	faceNode.VertexIndexC = vertexIndexC;
	faceNode.MaterialIndex = materialIndex;
	faceNode.Bounds = faceBounds;
	faceNode.Normal = Normal;

	// Add the face node to the face list
	//
	++m_FaceCount;
	return m_Faces.PushBack(MEMORY_FUNC_FILELINE(faceNode));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	RayCollision
//
// Parameters
//	faceIndices : A reference to a list of face indices which are a subset of all the faces in
//		the face list.  These are the pre-culled faces for which the ray will be tested against.
//	start : The start of the ray
//	end : The end of the ray
//	rayBounds : The bounding box of the ray
//	impactPoint : [out] A reference to a point which will be set to the point of impact if the 
//		ray collides with any of the faces.
//	impactNormal : [out/optional] A pointer to a vector which will be set to the normal
//		of the impact point if the ray collides with any of the faces.
//	faceMaterialIndex : [out] A reference to a material index which will be set to the material
//		index of the face for which the ray collides.
//	bestDistance : [out] Returns the distance between the ray start and the closest colliding
//		face if at all.
//	ignoreMaterialIndex : [optional] Specifies a material index for which faces that have the
//		material are ignored
//
// Returns
//	True if the ray collides with any of the faces, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CFaceList::RayCollision(const SIMPLE_DYNAMIC_ARRAY<unsigned int>& faceIndices, const VECTOR& start, const VECTOR& end,	 
	const BOUNDING_BOX& rayBounds, VECTOR& impactPoint, VECTOR* impactNormal, int& faceMaterialIndex, 
	float& bestDistance, int ignoreMaterialIndex)
{
	// occasionally we get meshes that are empty - skip these
	if(faceIndices.Count() > 0)
	{
		// ensure that the bounds of the ray intersect the bounds of the entire mesh
		if(BoundsIntersect(m_Bounds.vMin, m_Bounds.vMax, rayBounds.vMin, rayBounds.vMax))
		{
			for(unsigned int i = 0; i < faceIndices.Count(); ++i)
			{
				unsigned int faceIndex = faceIndices[i];
				const FACE_LIST_NODE* face = GetFace(faceIndex);
			
				// for each face, make sure that the ray-sphere bounds intersect the bounds of the face
				// before doing the heavy lifting
				if(face->MaterialIndex != ignoreMaterialIndex && BoundsIntersect(face->Bounds.vMin, face->Bounds.vMax, rayBounds.vMin, rayBounds.vMax))
				{
					// determine whether the ray passes through the plane, by classifying the start and
					// end of the ray
					PLANE_CLASSIFICATION StartClassification = ClassifyPoint(start, m_Vertices[face->VertexIndexA], face->Normal);
					PLANE_CLASSIFICATION EndClassification = ClassifyPoint(end, m_Vertices[face->VertexIndexA], face->Normal);

					// if it intersects, and the start point is on the front side of the plane,
					if((StartClassification != EndClassification) && (StartClassification != KPlaneBack))
					{
						VECTOR	TemporaryVEC3D(0, 0, 0);
						VECTOR	TemporaryVEC3D2(end); 
						// Find the intersection point
						if(GetLinePlaneIntersection(start, end, m_Vertices[face->VertexIndexA], face->Normal, TemporaryVEC3D))
						{
							// determine if the point lies within the bounds of this face
							if(PointIn(TemporaryVEC3D, face))
							{
								TemporaryVEC3D2 = TemporaryVEC3D - start;
								float Length	= VectorLength(TemporaryVEC3D2);
								// and if it is closer than previous collisions, we keep it
								if(Length < bestDistance)
								{
									bestDistance = Length;
									impactPoint = TemporaryVEC3D;
									if(impactNormal)
									{
										*impactNormal = face->Normal;
									}
									faceMaterialIndex = face->MaterialIndex;
								}
							}
						}
					}
				}
			}
		}
	}

	if(bestDistance != INFINITY)
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PointIn
//
// Parameters
//	point : The point that will be checked against the specified face
//	face : A pointer to the face for which the point will be checked against
//
// Returns
//	True if the point lies on the plane of the specified face, false otherwise
//
/////////////////////////////////////////////////////////////////////////////
bool CFaceList::PointIn(const VECTOR& point, const FACE_LIST_NODE* face)
{
	VECTOR Edge1 = m_Vertices[face->VertexIndexB] - m_Vertices[face->VertexIndexA];
	VECTOR Edge2 = m_Vertices[face->VertexIndexC] - m_Vertices[face->VertexIndexA];

	VECTOR DeterminantVec;
	VectorCross( DeterminantVec, face->Normal, Edge2 );

	float Determinant = VectorDot( Edge1, DeterminantVec );

	VECTOR Ray = point - m_Vertices[face->VertexIndexA];

	float u = VectorDot( Ray, DeterminantVec );

	if( u < 0 || u > Determinant )
	{
		return false;
	}

	VECTOR QVector;
	VectorCross( QVector, Ray, Edge1 );

	float v = VectorDot( face->Normal, QVector );
	if( v < 0 || u + v > Determinant )
	{
		return false;
	}

	return true;
}

