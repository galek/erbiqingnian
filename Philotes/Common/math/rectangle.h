///PHILOTES Source Code.  (C)2012 PhiloLabs
#pragma once

#include "scalar.h"

namespace Math {

	struct Rectangle
	{
		scalar left;
		scalar top;
		scalar right;
		scalar bottom;

		__forceinline bool inside(scalar x, scalar y) const { return x >= left && x <= right && y >= top && y <= bottom; }
	};


	__forceinline Rectangle intersect(const Rectangle& lhs, const Rectangle& rhs)
	{
		Rectangle r;

		r.left   = lhs.left   > rhs.left   ? lhs.left   : rhs.left;
		r.top    = lhs.top    > rhs.top    ? lhs.top    : rhs.top;
		r.right  = lhs.right  < rhs.right  ? lhs.right  : rhs.right;
		r.bottom = lhs.bottom < rhs.bottom ? lhs.bottom : rhs.bottom;

		return r;
	}

}


// #pragma once
// 
// 
// #include "core/types.h"

// ------------------------------------------------------------------------------
// namespace Math
// {
// template<class TYPE> class rectangle
// {
// public:
//     /// default constructor
//     rectangle();
//     /// constructor 1
//     rectangle(TYPE l, TYPE t, TYPE r, TYPE b);
//     /// set content
//     void set(TYPE l, TYPE t, TYPE r, TYPE b);
//     /// return true if point is inside
//     bool inside(TYPE x, TYPE y) const;
//     /// return width
//     TYPE width() const;
//     /// return height
//     TYPE height() const;
//     /// return center x
//     TYPE centerX() const;
//     /// return center y
//     TYPE centerY() const;
// 
//     TYPE left;
//     TYPE top;
//     TYPE right;
//     TYPE bottom;
// };
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE>
// rectangle<TYPE>::rectangle()
// {
//     // empty
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE>
// rectangle<TYPE>::rectangle(TYPE l, TYPE t, TYPE r, TYPE b) :
//     left(l),
//     top(t),
//     right(r),
//     bottom(b)
// {
//     ph_assert(this->left <= this->right);
//     ph_assert(this->top <= this->bottom);
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> void
// rectangle<TYPE>::set(TYPE l, TYPE t, TYPE r, TYPE b)
// {
//     ph_assert(l <= r);
//     ph_assert(t <= b);
//     this->left = l;
//     this->top = t;
//     this->right = r;
//     this->bottom = b;
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> bool
// rectangle<TYPE>::inside(TYPE x, TYPE y) const
// {
//     return (this->left <= x) && (x <= this->right) && (this->top <= y) && (y <= this->bottom);
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> TYPE
// rectangle<TYPE>::width() const
// {
//     return this->right - this->left;
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> TYPE
// rectangle<TYPE>::height() const
// {
//     return this->bottom - this->top;
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> TYPE
// rectangle<TYPE>::centerX() const
// {
//     return (this->left + this->right) / 2;
// }
// 
// ------------------------------------------------------------------------------
// /**
// */
// template<class TYPE> TYPE
// rectangle<TYPE>::centerY() const
// {
//     return (this->top + this->bottom) / 2;
// }
// } // namespace Math
// ------------------------------------------------------------------------------
// 
// 


