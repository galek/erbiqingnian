
#pragma once

// Precompiler options
#include "mathprerequisites.h"

#include "mathmisc.h"
#include "vector3.h"
#include "planeBoundedVolume.h"

namespace Math {

    class _PhiloCommonExport Ray
    {
    protected:
        Vector3 mOrigin;
        Vector3 mDirection;
    public:
        Ray():mOrigin(Vector3::ZERO), mDirection(Vector3::UNIT_Z) {}
        Ray(const Vector3& origin, const Vector3& direction)
            :mOrigin(origin), mDirection(direction) {}

        /** Sets the origin of the ray. */
        void setOrigin(const Vector3& origin) {mOrigin = origin;} 
        /** Gets the origin of the ray. */
        const Vector3& getOrigin(void) const {return mOrigin;} 

        /** Sets the direction of the ray. */
        void setDirection(const Vector3& dir) {mDirection = dir;} 
        /** Gets the direction of the ray. */
        const Vector3& getDirection(void) const {return mDirection;} 

		/** Gets the position of a point t units along the ray. */
		Vector3 getPoint(scalar t) const { 
			return Vector3(mOrigin + (mDirection * t));
		}
		
		/** Gets the position of a point t units along the ray. */
		Vector3 operator*(scalar t) const { 
			return getPoint(t);
		}

		/** Tests whether this ray intersects the given plane. 
		@returns A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, scalar> intersects(const Plane& p) const
		{
			return MathMisc::intersects(*this, p);
		}
        /** Tests whether this ray intersects the given plane bounded volume. 
        @returns A pair structure where the first element indicates whether
        an intersection occurs, and if true, the second element will
        indicate the distance along the ray at which it intersects. 
        This can be converted to a point in space by calling getPoint().
        */
        std::pair<bool, scalar> intersects(const PlaneBoundedVolume& p) const
        {
            return MathMisc::intersects(*this, p.planes, p.outside == Plane::POSITIVE_SIDE);
        }
		/** Tests whether this ray intersects the given sphere. 
		@returns A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, scalar> intersects(const Sphere& s) const
		{
			return MathMisc::intersects(*this, s);
		}
		/** Tests whether this ray intersects the given box. 
		@returns A pair structure where the first element indicates whether
			an intersection occurs, and if true, the second element will
			indicate the distance along the ray at which it intersects. 
			This can be converted to a point in space by calling getPoint().
		*/
		std::pair<bool, scalar> intersects(const AxisAlignedBox& box) const
		{
			return MathMisc::intersects(*this, box);
		}

    };
	/** @} */
	/** @} */

}
