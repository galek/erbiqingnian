#pragma once

// Precompiler options
#include "mathprerequisites.h"

#include "vector3.h"

namespace Philo {

    class Sphere
    {
    protected:
        scalar mRadius;
        Vector3 mCenter;
    public:
        /** Standard constructor - creates a unit sphere around the origin.*/
        Sphere() : mRadius(1.0), mCenter(Vector3::ZERO) {}
        /** Constructor allowing arbitrary spheres. 
            @param center The center point of the sphere.
            @param radius The radius of the sphere.
        */
        Sphere(const Vector3& center, scalar radius)
            : mRadius(radius), mCenter(center) {}

        /** Returns the radius of the sphere. */
        scalar getRadius(void) const { return mRadius; }

        /** Sets the radius of the sphere. */
        void setRadius(scalar radius) { mRadius = radius; }

        /** Returns the center point of the sphere. */
        const Vector3& getCenter(void) const { return mCenter; }

        /** Sets the center point of the sphere. */
        void setCenter(const Vector3& center) { mCenter = center; }

		/** Returns whether or not this sphere intersects another sphere. */
		bool intersects(const Sphere& s) const
		{
            return (s.mCenter - mCenter).squaredLength() <=
                Math::Sqr(s.mRadius + mRadius);
		}
		/** Returns whether or not this sphere intersects a box. */
		bool intersects(const AxisAlignedBox& box) const
		{
			return Math::intersects(*this, box);
		}
		/** Returns whether or not this sphere intersects a plane. */
		bool intersects(const Plane& plane) const
		{
			return Math::intersects(*this, plane);
		}
		/** Returns whether or not this sphere intersects a point. */
		bool intersects(const Vector3& v) const
		{
            return ((v - mCenter).squaredLength() <= Math::Sqr(mRadius));
		}
        

    };
	/** @} */
	/** @} */

}