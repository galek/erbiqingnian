
#include "renderTransform.h"
#include "renderMeshContext.h"

_NAMESPACE_BEGIN

RenderTransform::RenderTransform()
	:RenderNode()
{
	needUpdate();
}

RenderTransform::RenderTransform( const String& name )
	:RenderNode(name)
{
	needUpdate();
}

RenderTransform::~RenderTransform()
{
	ObjectMap::Iterator itr;
	RenderElement* ret;
	for ( itr = mObjectsByName.Begin(); itr != mObjectsByName.End(); ++itr )
	{
		ret = itr->Value();
		ret->_notifyAttached(NULL);
	}
	mObjectsByName.Clear();
}

RenderNode* RenderTransform::createChildImpl( void )
{
	return new RenderTransform();
}

RenderNode* RenderTransform::createChildImpl( const String& name )
{
	return new RenderTransform(name);
}

void RenderTransform::attachObject( RenderElement* obj )
{
	if (obj->isAttached())
	{
		PH_EXCEPT(ERR_RENDER,"Object already attached to a node");
	}

	obj->_notifyAttached(this);

	// Also add to name index
	if(mObjectsByName.Contains(obj->getName()))
	{
		ph_assert(!"已存在同名物件");
	}

	mObjectsByName.Add(obj->getName(),obj);

	needUpdate();
}

unsigned short RenderTransform::numAttachedObjects( void ) const
{
	return static_cast< unsigned short >( mObjectsByName.Size() );
}

RenderElement* RenderTransform::getAttachedObject( unsigned short index )
{
	if (index < mObjectsByName.Size())
	{
		ObjectMap::Iterator i = mObjectsByName.Begin();
		// Increment (must do this one at a time)            
		while (index--)++i;

		return i->Value();
	}
	else
	{
		PH_EXCEPT(ERR_RENDER, "Object index out of bounds.");
	}
}

RenderElement* RenderTransform::getAttachedObject( const String& name )
{
	ObjectMap::Iterator i = mObjectsByName.Find(name);

	if (i == mObjectsByName.End())
	{
		PH_EXCEPT(ERR_RENDER, "Attached object " + name + " not found.");
	}

	return i->Value();
}

RenderElement* RenderTransform::detachObject( unsigned short index )
{
	RenderElement* ret;
	if (index < mObjectsByName.Size())
	{

		ObjectMap::Iterator i = mObjectsByName.Begin();
		// Increment (must do this one at a time)            
		while (index--)++i;

		ret = i->Value();
		mObjectsByName.Erase(i);

		ret->_notifyAttached(NULL);

		needUpdate();

		return ret;

	}
	else
	{
		PH_EXCEPT(ERR_RENDER, "Object index out of bounds.");
	}
}

void RenderTransform::detachObject( RenderElement* obj )
{
	ObjectMap::Iterator i, iend;
	iend = mObjectsByName.End();
	for (i = mObjectsByName.Begin(); i != iend; ++i)
	{
		if (i->Value() == obj)
		{
			mObjectsByName.Erase(i);
			break;
		}
	}
	obj->_notifyAttached(NULL);

	needUpdate();
}

RenderElement* RenderTransform::detachObject( const String& name )
{
	ObjectMap::Iterator it = mObjectsByName.Find(name);
	if (it == mObjectsByName.End())
	{
		PH_EXCEPT(ERR_RENDER, "Object " + name + " is not attached to this node.");
	}
	RenderElement* ret = it->Value();
	mObjectsByName.Erase(it);
	ret->_notifyAttached(NULL);

	needUpdate();

	return ret;
}

void RenderTransform::detachAllObjects( void )
{
	ObjectMap::Iterator itr;
	RenderElement* ret;
	for ( itr = mObjectsByName.Begin(); itr != mObjectsByName.End(); ++itr )
	{
		ret = itr->Value();
		ret->_notifyAttached(NULL);
	}
	mObjectsByName.Clear();

	needUpdate();
}

_NAMESPACE_END