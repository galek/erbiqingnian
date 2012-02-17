
#include "renderNode.h"

_NAMESPACE_BEGIN

NameGenerator				RenderNode::msNameGenerator("nd_");
RenderNode::QueuedUpdates	RenderNode::msQueuedUpdates;

RenderNode::RenderNode()
	:mParent(0),
	mNeedParentUpdate(false),
	mNeedChildUpdate(false),
	mParentNotified(false),
	mQueuedForUpdate(false),
	mOrientation(Quaternion::IDENTITY),
	mPosition(Vector3::ZERO),
	mScale(Vector3::UNIT_SCALE),
	mInheritOrientation(true),
	mInheritScale(true),
	mDerivedOrientation(Quaternion::IDENTITY),
	mDerivedPosition(Vector3::ZERO),
	mDerivedScale(Vector3::UNIT_SCALE),
	mInitialPosition(Vector3::ZERO),
	mInitialOrientation(Quaternion::IDENTITY),
	mInitialScale(Vector3::UNIT_SCALE),
	mCachedTransformOutOfDate(true),
	mListener(0),
	mNodeType(NT_UNKNOW)
{
	// Generate a name
	mName = msNameGenerator.generate();

	needUpdate();

}
//-----------------------------------------------------------------------
RenderNode::RenderNode(const String& name)
	:
	mParent(0),
	mNeedParentUpdate(false),
	mNeedChildUpdate(false),
	mParentNotified(false),
	mQueuedForUpdate(false),
	mName(name),
	mOrientation(Quaternion::IDENTITY),
	mPosition(Vector3::ZERO),
	mScale(Vector3::UNIT_SCALE),
	mInheritOrientation(true),
	mInheritScale(true),
	mDerivedOrientation(Quaternion::IDENTITY),
	mDerivedPosition(Vector3::ZERO),
	mDerivedScale(Vector3::UNIT_SCALE),
	mInitialPosition(Vector3::ZERO),
	mInitialOrientation(Quaternion::IDENTITY),
	mInitialScale(Vector3::UNIT_SCALE),
	mCachedTransformOutOfDate(true),
	mListener(0),
	mNodeType(NT_UNKNOW)
{
	needUpdate();
}

//-----------------------------------------------------------------------
RenderNode::~RenderNode()
{
	// Call listener (note, only called if there's something to do)
	if (mListener)
	{
		mListener->nodeDestroyed(this);
	}

	removeAllChildren();

	if(mParent)
		mParent->removeChild(this);

	if (mQueuedForUpdate)
	{
		// Erase from queued updates
		QueuedUpdates::Iterator it = msQueuedUpdates.Find(this);

		ph_assert(it != msQueuedUpdates.End());

		if (it != msQueuedUpdates.End())
		{
			// Optimised algorithm to erase an element from unordered vector.
			*it = msQueuedUpdates.Back();
			msQueuedUpdates.PopBack();
		}
	}

}
//-----------------------------------------------------------------------
RenderNode* RenderNode::getParent(void) const
{
	return mParent;
}

//-----------------------------------------------------------------------
void RenderNode::setParent(RenderNode* parent)
{
	bool different = (parent != mParent);

	mParent = parent;
	// Request update from parent
	mParentNotified = false ;
	needUpdate();

	// Call listener (note, only called if there's something to do)
	if (mListener && different)
	{
		if (mParent)
			mListener->nodeAttached(this);
		else
			mListener->nodeDetached(this);
	}
}

//-----------------------------------------------------------------------
const Matrix4& RenderNode::_getFullTransform(void) const
{
	if (mCachedTransformOutOfDate)
	{
		// Use derived values
		mCachedTransform.makeTransform(
			_getDerivedPosition(),
			_getDerivedScale(),
			_getDerivedOrientation());
		mCachedTransformOutOfDate = false;
	}
	return mCachedTransform;
}
//-----------------------------------------------------------------------
void RenderNode::_update(bool updateChildren, bool parentHasChanged)
{
	// always clear information about parent notification
	mParentNotified = false ;

	// Short circuit the off case
	if (!updateChildren && !mNeedParentUpdate && !mNeedChildUpdate && !parentHasChanged )
	{
		return;
	}


	// See if we should process everyone
	if (mNeedParentUpdate || parentHasChanged)
	{
		// Update transforms from parent
		_updateFromParent();
	}

	if (mNeedChildUpdate || parentHasChanged)
	{

		ChildNodeIterator it, itend;
		itend = mChildren.End();
		for (it = mChildren.Begin(); it != itend; ++it)
		{
			RenderNode* child = it->Value();
			child->_update(true, true);
		}
		mChildrenToUpdate.clear();
	}
	else
	{
		// Just update selected children

		ChildUpdateSet::iterator it, itend;
		itend = mChildrenToUpdate.end();
		for(it = mChildrenToUpdate.begin(); it != itend; ++it)
		{
			RenderNode* child = *it;
			child->_update(true, false);
		}

		mChildrenToUpdate.clear();
	}

	mNeedChildUpdate = false;

}
//-----------------------------------------------------------------------
void RenderNode::_updateFromParent(void) const
{
	updateFromParentImpl();

	// Call listener (note, this method only called if there's something to do)
	if (mListener)
	{
		mListener->nodeUpdated(this);
	}
}
//-----------------------------------------------------------------------
void RenderNode::updateFromParentImpl(void) const
{
	if (mParent)
	{
		// Update orientation
		const Quaternion& parentOrientation = mParent->_getDerivedOrientation();
		if (mInheritOrientation)
		{
			// Combine orientation with that of parent
			mDerivedOrientation = parentOrientation * mOrientation;
		}
		else
		{
			// No inheritance
			mDerivedOrientation = mOrientation;
		}

		// Update scale
		const Vector3& parentScale = mParent->_getDerivedScale();
		if (mInheritScale)
		{
			// Scale own position by parent scale, NB just combine
			// as equivalent axes, no shearing
			mDerivedScale = parentScale * mScale;
		}
		else
		{
			// No inheritance
			mDerivedScale = mScale;
		}

		// Change position vector based on parent's orientation & scale
		mDerivedPosition = parentOrientation * (parentScale * mPosition);

		// Add altered position vector to parents
		mDerivedPosition += mParent->_getDerivedPosition();
	}
	else
	{
		// Root node, no parent
		mDerivedOrientation = mOrientation;
		mDerivedPosition = mPosition;
		mDerivedScale = mScale;
	}

	mCachedTransformOutOfDate = true;
	mNeedParentUpdate = false;

}
//-----------------------------------------------------------------------
RenderNode* RenderNode::createChild(const Vector3& inTranslate, const Quaternion& inRotate)
{
	RenderNode* newNode = createChildImpl();
	newNode->translate(inTranslate);
	newNode->rotate(inRotate);
	this->addChild(newNode);

	return newNode;
}
//-----------------------------------------------------------------------
RenderNode* RenderNode::createChild(const String& name, const Vector3& inTranslate, const Quaternion& inRotate)
{
	RenderNode* newNode = createChildImpl(name);
	newNode->translate(inTranslate);
	newNode->rotate(inRotate);
	this->addChild(newNode);

	return newNode;
}
//-----------------------------------------------------------------------
void RenderNode::addChild(RenderNode* child)
{
	if (child->mParent)
	{
		PH_EXCEPT(ERR_RENDER,
			"RenderNode '" + child->getName() + "' already was a child of '" +
			child->mParent->getName() + "'.");
	}

	mChildren.Add(child->getName(), child);
	child->setParent(this);

}
//-----------------------------------------------------------------------
unsigned short RenderNode::numChildren(void) const
{
	return static_cast< unsigned short >( mChildren.Size() );
}
//-----------------------------------------------------------------------
RenderNode* RenderNode::getChild(unsigned short index)
{
	if( index < mChildren.Size() )
	{
		ChildNodeMap::Iterator i = mChildren.Begin();
		while (index--) ++i;
		return i->Value();
	}
	else
		return NULL;
}
//-----------------------------------------------------------------------
RenderNode* RenderNode::removeChild(unsigned short index)
{
	RenderNode* ret;
	if (index < mChildren.Size())
	{
		ChildNodeMap::Iterator i = mChildren.Begin();
		while (index--) ++i;
		ret = i->Value();
		// cancel any pending update
		cancelUpdate(ret);

		mChildren.Erase(i);
		ret->setParent(NULL);
		return ret;
	}
	else
	{
		PH_EXCEPT(ERR_RENDER,"Child index out of bounds.");
	}
	return 0;
}
//-----------------------------------------------------------------------
RenderNode* RenderNode::removeChild(RenderNode* child)
{
	if (child)
	{
		ChildNodeMap::Iterator i = mChildren.Find(child->getName());
		// ensure it's our child
		if (i != mChildren.End() && i->Value() == child)
		{
			// cancel any pending update
			cancelUpdate(child);

			mChildren.Erase(i);
			child->setParent(NULL);
		}
	}
	return child;
}
//-----------------------------------------------------------------------
const Quaternion& RenderNode::getOrientation() const
{
	return mOrientation;
}

//-----------------------------------------------------------------------
void RenderNode::setOrientation( const Quaternion & q )
{
	ph_assert(!q.isNaN() && "Invalid orientation supplied as parameter");
	mOrientation = q;
	mOrientation.normalise();
	needUpdate();
}
//-----------------------------------------------------------------------
void RenderNode::setOrientation( scalar w, scalar x, scalar y, scalar z)
{
	setOrientation(Quaternion(w, x, y, z));
}
//-----------------------------------------------------------------------
void RenderNode::resetOrientation(void)
{
	mOrientation = Quaternion::IDENTITY;
	needUpdate();
}

//-----------------------------------------------------------------------
void RenderNode::setPosition(const Vector3& pos)
{
	assert(!pos.isNaN() && "Invalid vector supplied as parameter");
	mPosition = pos;
	needUpdate();
}


//-----------------------------------------------------------------------
void RenderNode::setPosition(scalar x, scalar y, scalar z)
{
	Vector3 v(x,y,z);
	setPosition(v);
}

//-----------------------------------------------------------------------
const Vector3 & RenderNode::getPosition(void) const
{
	return mPosition;
}
//-----------------------------------------------------------------------
Matrix3 RenderNode::getLocalAxes(void) const
{
	Vector3 axisX = Vector3::UNIT_X;
	Vector3 axisY = Vector3::UNIT_Y;
	Vector3 axisZ = Vector3::UNIT_Z;

	axisX = mOrientation * axisX;
	axisY = mOrientation * axisY;
	axisZ = mOrientation * axisZ;

	return Matrix3(axisX.x, axisY.x, axisZ.x,
		axisX.y, axisY.y, axisZ.y,
		axisX.z, axisY.z, axisZ.z);
}

//-----------------------------------------------------------------------
void RenderNode::translate(const Vector3& d, TransformSpace relativeTo)
{
	switch(relativeTo)
	{
	case TS_LOCAL:
		// position is relative to parent so transform downwards
		mPosition += mOrientation * d;
		break;
	case TS_WORLD:
		// position is relative to parent so transform upwards
		if (mParent)
		{
			mPosition += (mParent->_getDerivedOrientation().Inverse() * d)
				/ mParent->_getDerivedScale();
		}
		else
		{
			mPosition += d;
		}
		break;
	case TS_PARENT:
		mPosition += d;
		break;
	}
	needUpdate();

}
//-----------------------------------------------------------------------
void RenderNode::translate(scalar x, scalar y, scalar z, TransformSpace relativeTo)
{
	Vector3 v(x,y,z);
	translate(v, relativeTo);
}
//-----------------------------------------------------------------------
void RenderNode::translate(const Matrix3& axes, const Vector3& move, TransformSpace relativeTo)
{
	Vector3 derived = axes * move;
	translate(derived, relativeTo);
}
//-----------------------------------------------------------------------
void RenderNode::translate(const Matrix3& axes, scalar x, scalar y, scalar z, TransformSpace relativeTo)
{
	Vector3 d(x,y,z);
	translate(axes,d,relativeTo);
}
//-----------------------------------------------------------------------
void RenderNode::roll(const Radian& angle, TransformSpace relativeTo)
{
	rotate(Vector3::UNIT_Z, angle, relativeTo);
}
//-----------------------------------------------------------------------
void RenderNode::pitch(const Radian& angle, TransformSpace relativeTo)
{
	rotate(Vector3::UNIT_X, angle, relativeTo);
}
//-----------------------------------------------------------------------
void RenderNode::yaw(const Radian& angle, TransformSpace relativeTo)
{
	rotate(Vector3::UNIT_Y, angle, relativeTo);

}
//-----------------------------------------------------------------------
void RenderNode::rotate(const Vector3& axis, const Radian& angle, TransformSpace relativeTo)
{
	Quaternion q;
	q.FromAngleAxis(angle,axis);
	rotate(q, relativeTo);
}

//-----------------------------------------------------------------------
void RenderNode::rotate(const Quaternion& q, TransformSpace relativeTo)
{
	// Normalise quaternion to avoid drift
	Quaternion qnorm = q;
	qnorm.normalise();

	switch(relativeTo)
	{
	case TS_PARENT:
		// Rotations are normally relative to local axes, transform up
		mOrientation = qnorm * mOrientation;
		break;
	case TS_WORLD:
		// Rotations are normally relative to local axes, transform up
		mOrientation = mOrientation * _getDerivedOrientation().Inverse()
			* qnorm * _getDerivedOrientation();
		break;
	case TS_LOCAL:
		// Note the order of the mult, i.e. q comes after
		mOrientation = mOrientation * qnorm;
		break;
	}
	needUpdate();
}


//-----------------------------------------------------------------------
void RenderNode::_setDerivedPosition( const Vector3& pos )
{
	//find where the node would end up in parent's local space
	setPosition( mParent->convertWorldToLocalPosition( pos ) );
}
//-----------------------------------------------------------------------
void RenderNode::_setDerivedOrientation( const Quaternion& q )
{
	//find where the node would end up in parent's local space
	setOrientation( mParent->convertWorldToLocalOrientation( q ) );
}

//-----------------------------------------------------------------------
const Quaternion & RenderNode::_getDerivedOrientation(void) const
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedOrientation;
}
//-----------------------------------------------------------------------
const Vector3 & RenderNode::_getDerivedPosition(void) const
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedPosition;
}
//-----------------------------------------------------------------------
const Vector3 & RenderNode::_getDerivedScale(void) const
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedScale;
}
//-----------------------------------------------------------------------
Vector3 RenderNode::convertWorldToLocalPosition( const Vector3 &worldPos )
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedOrientation.Inverse() * (worldPos - mDerivedPosition) / mDerivedScale;
}
//-----------------------------------------------------------------------
Vector3 RenderNode::convertLocalToWorldPosition( const Vector3 &localPos )
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return (mDerivedOrientation * localPos * mDerivedScale) + mDerivedPosition;
}
//-----------------------------------------------------------------------
Quaternion RenderNode::convertWorldToLocalOrientation( const Quaternion &worldOrientation )
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedOrientation.Inverse() * worldOrientation;
}
//-----------------------------------------------------------------------
Quaternion RenderNode::convertLocalToWorldOrientation( const Quaternion &localOrientation )
{
	if (mNeedParentUpdate)
	{
		_updateFromParent();
	}
	return mDerivedOrientation * localOrientation;

}
//-----------------------------------------------------------------------
void RenderNode::removeAllChildren(void)
{
	ChildNodeMap::Iterator i, iend;
	iend = mChildren.End();
	for (i = mChildren.Begin(); i != iend; ++i)
	{
		i->Value()->setParent(0);
	}
	mChildren.Clear();
	mChildrenToUpdate.clear();
}
//-----------------------------------------------------------------------
void RenderNode::setScale(const Vector3& inScale)
{
	ph_assert2(!inScale.isNaN() ,"Invalid vector supplied as parameter");
	mScale = inScale;
	needUpdate();
}
//-----------------------------------------------------------------------
void RenderNode::setScale(scalar x, scalar y, scalar z)
{
	setScale(Vector3(x, y, z));
}
//-----------------------------------------------------------------------
const Vector3 & RenderNode::getScale(void) const
{
	return mScale;
}
//-----------------------------------------------------------------------
void RenderNode::setInheritOrientation(bool inherit)
{
	mInheritOrientation = inherit;
	needUpdate();
}
//-----------------------------------------------------------------------
bool RenderNode::getInheritOrientation(void) const
{
	return mInheritOrientation;
}
//-----------------------------------------------------------------------
void RenderNode::setInheritScale(bool inherit)
{
	mInheritScale = inherit;
	needUpdate();
}
//-----------------------------------------------------------------------
bool RenderNode::getInheritScale(void) const
{
	return mInheritScale;
}
//-----------------------------------------------------------------------
void RenderNode::scale(const Vector3& inScale)
{
	mScale = mScale * inScale;
	needUpdate();

}
//-----------------------------------------------------------------------
void RenderNode::scale(scalar x, scalar y, scalar z)
{
	mScale.x *= x;
	mScale.y *= y;
	mScale.z *= z;
	needUpdate();

}
//-----------------------------------------------------------------------
const String& RenderNode::getName(void) const
{
	return mName;
}
//-----------------------------------------------------------------------
void RenderNode::setInitialState(void)
{
	mInitialPosition = mPosition;
	mInitialOrientation = mOrientation;
	mInitialScale = mScale;
}
//-----------------------------------------------------------------------
void RenderNode::resetToInitialState(void)
{
	mPosition = mInitialPosition;
	mOrientation = mInitialOrientation;
	mScale = mInitialScale;

	needUpdate();
}
//-----------------------------------------------------------------------
const Vector3& RenderNode::getInitialPosition(void) const
{
	return mInitialPosition;
}
//-----------------------------------------------------------------------
const Quaternion& RenderNode::getInitialOrientation(void) const
{
	return mInitialOrientation;

}
//-----------------------------------------------------------------------
const Vector3& RenderNode::getInitialScale(void) const
{
	return mInitialScale;
}
//-----------------------------------------------------------------------
RenderNode* RenderNode::getChild(const String& name)
{
	ChildNodeMap::Iterator i = mChildren.Find(name);

	if (i == mChildren.End())
	{
		PH_EXCEPT(ERR_RENDER, "Child node named " + name + " does not exist.");
	}
	return i->Value();

}
//-----------------------------------------------------------------------
RenderNode* RenderNode::removeChild(const String& name)
{
	ChildNodeMap::Iterator i = mChildren.Find(name);

	if (i == mChildren.End())
	{
		PH_EXCEPT(ERR_RENDER, "Child node named " + name + " does not exist.");
	}

	RenderNode* ret = i->Value();
	// Cancel any pending update
	cancelUpdate(ret);

	mChildren.Erase(i);
	ret->setParent(NULL);

	return ret;
}
//-----------------------------------------------------------------------
// scalar RenderNode::getSquaredViewDepth(const Camera* cam) const
// {
// 	Vector3 diff = _getDerivedPosition() - cam->getDerivedPosition();
// 
// 	// NB use squared length rather than real depth to avoid square root
// 	return diff.squaredLength();
// }
//-----------------------------------------------------------------------
void RenderNode::needUpdate(bool forceParentUpdate)
{

	mNeedParentUpdate = true;
	mNeedChildUpdate = true;
	mCachedTransformOutOfDate = true;

	// Make sure we're not root and parent hasn't been notified before
	if (mParent && (!mParentNotified || forceParentUpdate))
	{
		mParent->requestUpdate(this, forceParentUpdate);
		mParentNotified = true ;
	}

	// all children will be updated
	mChildrenToUpdate.clear();
}
//-----------------------------------------------------------------------
void RenderNode::requestUpdate(RenderNode* child, bool forceParentUpdate)
{
	// If we're already going to update everything this doesn't matter
	if (mNeedChildUpdate)
	{
		return;
	}

	mChildrenToUpdate.insert(child);
	// Request selective update of me, if we didn't do it before
	if (mParent && (!mParentNotified || forceParentUpdate))
	{
		mParent->requestUpdate(this, forceParentUpdate);
		mParentNotified = true ;
	}

}
//-----------------------------------------------------------------------
void RenderNode::cancelUpdate(RenderNode* child)
{
	mChildrenToUpdate.erase(child);

	// Propagate this up if we're done
	if (mChildrenToUpdate.empty() && mParent && !mNeedChildUpdate)
	{
		mParent->cancelUpdate(this);
		mParentNotified = false ;
	}
}
//-----------------------------------------------------------------------
void RenderNode::queueNeedUpdate(RenderNode* n)
{
	// Don't queue the node more than once
	if (!n->mQueuedForUpdate)
	{
		n->mQueuedForUpdate = true;
		msQueuedUpdates.Append(n);
	}
}
//-----------------------------------------------------------------------
void RenderNode::processQueuedUpdates(void)
{
	for (QueuedUpdates::Iterator i = msQueuedUpdates.Begin();
		i != msQueuedUpdates.End(); ++i)
	{
		// Update, and force parent update since chances are we've ended
		// up with some mixed state in there due to re-entrancy
		RenderNode* n = *i;
		n->mQueuedForUpdate = false;
		n->needUpdate(true);
	}
	msQueuedUpdates.Reset();
}

_NAMESPACE_END