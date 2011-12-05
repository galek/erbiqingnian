//----------------------------------------------------------------------------
// quadtree.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

#include "VECTOR.h"
#include "boundingbox.h"
#include "array.h"

// Forward Declares
//
template<typename T, typename FN_GET_BOUNDS>
class CQuadtree;

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CQuadtreeNode
//
// Parameters
//	T : The item type contained within the node
//	FN_GET_BOUNDS : A function object which allows an item's bounds to be retrieved
//		from the item
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
class CQuadtreeNode
{

	private:

		enum QUADRANT
		{
			UPPER_LEFT,
			UPPER_RIGHT,
			LOWER_LEFT,
			LOWER_RIGHT
		};

	public:

		void 								Init(CQuadtree<T, FN_GET_BOUNDS>* tree);
		void 								Free(CQuadtree<T, FN_GET_BOUNDS>* tree);
		void 								Add(CQuadtree<T, FN_GET_BOUNDS>* tree, const T& item, const BOUNDING_BOX& itemBounds, float minimumWidth, const BOUNDING_BOX& nodeBounds);
		void 								Remove(const T& item, const BOUNDING_BOX& itemBounds, const BOUNDING_BOX& nodeBounds);
		void 								GetItems(const BOUNDING_BOX& bounds, SIMPLE_DYNAMIC_ARRAY<T>& items, bool checkItemBounds, const BOUNDING_BOX& nodeBounds, FN_GET_BOUNDS& fnGetBounds) const;

	private:

		void								GetBounds(BOUNDING_BOX& bounds, const BOUNDING_BOX& parentBounds, QUADRANT quadrant) const;
		
	private:

		CQuadtreeNode*						m_pChildNodes[4];
		SIMPLE_DYNAMIC_ARRAY<T>				m_pMemberData;	
		
}; 


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Parameters
//	tree : A pointer to the tree to which the node belongs
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::Init(CQuadtree<T, FN_GET_BOUNDS>* tree)			
{
	ArrayInit(m_pMemberData, tree->GetAllocator(), 1);

	for(int i = 0; i < 4; ++i)
	{
		m_pChildNodes[i] = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
// Parameters
//	tree : A pointer to the tree to which the node belongs
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::Free(CQuadtree<T, FN_GET_BOUNDS>* tree)
{
	m_pMemberData.Destroy();
	
	for(int i = 0; i < 4; ++i)
	{
		if(m_pChildNodes[i])
		{
			m_pChildNodes[i]->Free(tree);
			tree->PutNode(m_pChildNodes[i]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Add
//
// Parameters
//	tree : A pointer to the tree to which the node belongs
//	item : The item to add
//	itemBounds : The bounding box of the item
//	minimumWidth : The node width for which no nodes in the tree will be smaller.
//		This size is provided by the tree and determines whether calling AddData will
//		create additional sub-nodes.
//	nodeBounds : The bounds for the current node as calculated by the parent
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::Add(CQuadtree<T, FN_GET_BOUNDS>* tree, const T& item, const BOUNDING_BOX& itemBounds, float minimumWidth, const BOUNDING_BOX& nodeBounds)
{
	float nodeWidth = nodeBounds.vMax.fX - nodeBounds.vMin.fX;
	float childNodeWidth = nodeWidth * 0.5f;

	// If the data can go in any of the child nodes, place it there
	//
	if(childNodeWidth >= minimumWidth)
	{
		for(int i = 0; i < 4; ++i)
		{
			BOUNDING_BOX childBounds;
			GetBounds(childBounds, nodeBounds, (QUADRANT)i);

			// If the child node completely contains the item
			//
			if(BoundingBoxContainsXY(&childBounds, &itemBounds))
			{
				// Create the child if necessary
				//
				if(m_pChildNodes[i] == NULL)
				{
					m_pChildNodes[i] = tree->GetNode();
					m_pChildNodes[i]->Init(tree);
				}
				
				// Add the item to the child node
				//
				m_pChildNodes[i]->Add(tree, item, itemBounds, minimumWidth, childBounds);
				return;
			}
		}
	}

	// Unable to add the item to a child node, so add it to the current node
	//
	ArrayAddItem(m_pMemberData, item);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Remove
//
// Parameters
//	item : The item to remove
//	itemBounds : The bounding box of the item
//	nodeBounds : The bounds for the current node as calculated by the parent
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::Remove(const T& item, const BOUNDING_BOX& itemBounds, const BOUNDING_BOX& nodeBounds)
{
	// Find the index of the item
	//
	int index = m_pMemberData.GetIndex(item);

	// If the item is in the current node
	//
	if(index != INVALID_INDEX)
	{
		m_pMemberData[index] = m_pMemberData[m_pMemberData.Count() - 1];
		m_pMemberData.RemoveByIndex(MEMORY_FUNC_FILELINE(m_pMemberData.Count() - 1));
		return;
	}
	// Otherwise check the child nodes
	//
	for(int i = 0; i < 4; ++i)
	{
		if(m_pChildNodes[i])
		{
			BOUNDING_BOX childBounds;
			GetBounds(childBounds, nodeBounds, (QUADRANT)i);
		
			// If the item is within the child node
			//
			if(BoundingBoxCollideXY(&childBounds, &itemBounds))
			{
				m_pChildNodes[i]->Remove(item, itemBounds, childBounds);	
				return;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetItems
//
// Parameters
//	bounds : The bounding box for which items that are either partially or completely
//		contained within will be added to the items array
//	items : [out] An array used to add items that either partially or completely are contained
//		within the bounds of the node
//	checkItemBounds : Determines whether the item bounds are checked against
//		the argument bounds before adding them to the item array
//	nodeBounds : The bounds for the current node as calculated by the parent
//	fnGetBounds : A reference to the function object that can be used to retrieve an item's
//		bounds from the item.
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::GetItems(const BOUNDING_BOX& bounds, SIMPLE_DYNAMIC_ARRAY<T>& items, bool checkItemBounds, const BOUNDING_BOX& nodeBounds, FN_GET_BOUNDS& fnGetBounds) const
{
	// If no item bound checking is required
	//
	if(checkItemBounds == false)
	{
		// Add all items to the item array
		//
		for(unsigned int i = 0; i < m_pMemberData.Count(); ++i)
		{
			ArrayAddItem(items, m_pMemberData[i]);
		}
	}
	// Otherwise we need to check each item in the node to see if it is within the argument bounds
	//
	else
	{
		for(unsigned int i = 0; i < m_pMemberData.Count(); ++i)
		{
			// Add any items that are contained or partially contained by the argument bounds
			//
			const BOUNDING_BOX& itemBounds = fnGetBounds(m_pMemberData[i]);
			if(BoundingBoxCollideXY(&bounds, &itemBounds))
			{
				ArrayAddItem(items, m_pMemberData[i]);
			}
		}
	}

	// Recurse into child nodes
	//
	for(int i = 0; i < 4; ++i)
	{
		if(m_pChildNodes[i])
		{
			BOUNDING_BOX childBounds;
			GetBounds(childBounds, nodeBounds, (QUADRANT)i);

			// If no item bound checking is requires or the child bounds are completely 
			// contained within the argument bounds
			//
			if(checkItemBounds == false || BoundingBoxContainsXY(&bounds, &childBounds))
			{
				// Add all child items to the item array without checking it's item bounds
				//
				m_pChildNodes[i]->GetItems(bounds, items, false, childBounds, fnGetBounds);
			}
			// Otherwise if child bounds are either partically or completely contained 
			// within the argument bounds
			//
			else if(BoundingBoxCollideXY(&bounds, &childBounds))
			{
				// Allow the child nodes to check it's item bounds when adding them to the array
				//
				m_pChildNodes[i]->GetItems(bounds, items, true, childBounds, fnGetBounds);				
			}
		}
	}

}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetBounds
//
// Parameters
//	bounds : [out] Returns the bounds for the node
//	parentBounds : The bounds of the parent node
//	quadrant : The quadrant for the node
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtreeNode<T, FN_GET_BOUNDS>::GetBounds(BOUNDING_BOX& bounds, const BOUNDING_BOX& parentBounds, QUADRANT quadrant) const
{
	VECTOR parentCenter;
	parentBounds.Center(parentCenter);

	switch(quadrant)
	{
		case UPPER_LEFT:
		{
			bounds.vMin.fX = parentBounds.vMin.fX;
			bounds.vMin.fY = parentCenter.fY;

			bounds.vMax.fX = parentCenter.fX;
			bounds.vMax.fY = parentBounds.vMax.fY;
			
			break;
		}
		case UPPER_RIGHT:
		{
			bounds.vMin = parentCenter;
			bounds.vMax = parentBounds.vMax;
		
			break;
		}
		case LOWER_LEFT:
		{
			bounds.vMin = parentBounds.vMin;
			bounds.vMax = parentCenter;
			
			break;
		}
		case LOWER_RIGHT:
		{
			bounds.vMin.fX = parentCenter.fX;
			bounds.vMin.fY = parentBounds.vMin.fY;

			bounds.vMax.fX = parentBounds.vMax.fX;
			bounds.vMax.fY = parentCenter.fY;
		
			break;
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	CQuadtree
//
// Parameters
//	T : The item type that the nodes will contain
//	FN_GET_BOUNDS : A function object that translates from an item to a BOUNDING_BOX for
//		the item
//
// Remarks
//
//	An example function object to get the bounding box for the item:
//
//	class CGetBounds
//	{
//		public:
//
//			const BOUNDING_BOX& operator()(const T& item) const
//			{
//				return T->GetBoundingBox();
//			}
//	}
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
class CQuadtree
{
	friend class CQuadtreeNode<T, FN_GET_BOUNDS>;

	public:

		~CQuadtree();
	
	public:

		void 								Init(const BOUNDING_BOX& bounds, float minimumNodeWidth, FN_GET_BOUNDS fnGetBounds, MEMORYPOOL* allocator = NULL);
		void 								Free();
		void 								Add(const T& item, const BOUNDING_BOX& itemBounds);
		void 								Remove(const T& item, const BOUNDING_BOX& itemBounds);
		void 								GetItems(const BOUNDING_BOX& bounds, SIMPLE_DYNAMIC_ARRAY<T>& items, bool bCheckItemBounds = true);

	private:

		CQuadtreeNode<T, FN_GET_BOUNDS>*	GetNode();
		void								PutNode(CQuadtreeNode<T, FN_GET_BOUNDS>* node);
		MEMORYPOOL*							GetAllocator();

	private:

		FN_GET_BOUNDS						m_fnGetBounds;
		BOUNDING_BOX						m_Bounds;
		float								m_MinimumNodeWidth;
		CQuadtreeNode<T, FN_GET_BOUNDS>*	m_RootNode;
		MEMORYPOOL*							m_Allocator;
		unsigned int						m_NodeCount;
};

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	~CQuadtree Destructor
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
CQuadtree<T, FN_GET_BOUNDS>::~CQuadtree()	
{
	Free();
} 

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Init
//
// Parameters
//	bounds : Specifies the outer bounds of the quadtree
//	minimumNodeWidth : Determines what the small node width will be
//	fnGetBounds : A function object which describes
//	allocator : [optional] The allocator that will be used for node memory
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtree<T, FN_GET_BOUNDS>::Init(const BOUNDING_BOX& bounds, float minimumNodeWidth, FN_GET_BOUNDS fnGetBounds, MEMORYPOOL* allocator)
{
	m_Allocator = allocator;
	m_MinimumNodeWidth = minimumNodeWidth;
	m_NodeCount = 0;
	m_fnGetBounds = fnGetBounds;
	
	// Create a square bounding box from the argument bounds
	//
	VECTOR nodeCenter;
	bounds.Center(nodeCenter);	
	float nodeWidth = MAX(bounds.vMax.fX - bounds.vMin.fX, bounds.vMax.fY - bounds.vMin.fY);
	float nodeHalfWidth = nodeWidth / 2;
	
	m_Bounds.vMin = nodeCenter - VECTOR(nodeHalfWidth, nodeHalfWidth, 0);
	m_Bounds.vMax = nodeCenter + VECTOR(nodeHalfWidth, nodeHalfWidth, 0);
	
	// Generate the parent node
	//
	m_RootNode = GetNode();
	m_RootNode->Init(this);
} 


/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Free
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtree<T, FN_GET_BOUNDS>::Free()
{
	if(m_RootNode)
	{
		// Allow the parent to free child nodes
		//
		m_RootNode->Free(this);

		// Free the parent node memory
		//
		PutNode(m_RootNode);
	}

} 

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Add
//
// Parameters
//	item : The item to add
//	itemBounds : The bounding box for the item
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtree<T, FN_GET_BOUNDS>::Add(const T& item, const BOUNDING_BOX& itemBounds)
{
	if(m_RootNode)
	{
		m_RootNode->Add(this, item, itemBounds, m_MinimumNodeWidth, m_Bounds);
	}
} 

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	Remove
//
// Parameters
//	item : The item to remove
//	itemBounds : The bounds of the item
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtree<T, FN_GET_BOUNDS>::Remove(const T& item, const BOUNDING_BOX& itemBounds)
{
	if(m_RootNode)
	{
		m_RootNode->Remove(item, itemBounds, m_Bounds);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetItems
//
// Parameters
//	bounds : A bounding box which determines which items are returned.  This items
//		which are completely or partially contained within the bounds are returned in
//		the items array.
//	items : [out] An array that will be filled with items that are either partially or
//		completely contained within the bounds
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
void CQuadtree<T, FN_GET_BOUNDS>::GetItems(const BOUNDING_BOX& bounds, SIMPLE_DYNAMIC_ARRAY<T>& items, bool bCheckItemBounds = true)
{
	if(m_RootNode)
	{
		m_RootNode->GetItems(bounds, items, bCheckItemBounds, m_Bounds, m_fnGetBounds);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetNode
//
// Returns
//	A newly allocated node
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
inline CQuadtreeNode<T, FN_GET_BOUNDS>* CQuadtree<T, FN_GET_BOUNDS>::GetNode()
{
	++m_NodeCount;
	return (CQuadtreeNode<T, FN_GET_BOUNDS>*)MALLOC(m_Allocator, sizeof(CQuadtreeNode<T, FN_GET_BOUNDS>));
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	PutNode
//
// Parameters
//	node : A pointer to the node to release
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
inline void CQuadtree<T, FN_GET_BOUNDS>::PutNode(CQuadtreeNode<T, FN_GET_BOUNDS>* node)
{
	--m_NodeCount;
	FREE(m_Allocator, node);
}

/////////////////////////////////////////////////////////////////////////////
//
// Name
//	GetAllocator
//
// Returns
//	A pointer to the allocator
//
/////////////////////////////////////////////////////////////////////////////
template<typename T, typename FN_GET_BOUNDS>
inline MEMORYPOOL* CQuadtree<T, FN_GET_BOUNDS>::GetAllocator()
{
	return m_Allocator;
}




