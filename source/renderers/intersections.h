//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef INTERSECTIONS_HEADER
#define INTERSECTIONS_HEADER

//--- Standard Includes ---
#include <cassert>

//--- Framework Includes ---
#include <primitives.h>
#include <matrix.h>

namespace gfx
{
	//--- Intersection Tests ---
	[[maybe_unused]]
	static bool HitTestSphere
	(
		const Sphere& sphere,
		const Ray& ray,
		RayHitRecord& hit_record,
		const bool ignore_hit_record = false
	)
	{
		const Vector3 origin_difference{sphere.origin - ray.origin};
		const float projection_size{Vector3::Dot(origin_difference, ray.direction)};
		const float rejection_size
		{
			Vector3::Reject(origin_difference, ray.direction).Magnitude()
		};
		// r == reject for tangential hit
		if (sphere.radius <= rejection_size) return false;
		if (ignore_hit_record) return true;
		const float projection_distance_difference
		{
			std::sqrtf(std::pow(sphere.radius, 2) - std::pow(rejection_size, 2))
		};
		const float t{projection_size - projection_distance_difference};
		hit_record.ray = ray;
		hit_record.t = t;
		return true;
	}

	[[maybe_unused]]
	static bool HitTestPlane
	(
		const Plane& plane,
		const Ray& ray,
		RayHitRecord& hit_record,
		const bool ignore_hit_record = false
	)
	{
		Vector3 plane_normal{plane.normal};
		float dn{Vector3::Dot(ray.direction, plane.normal)};

		//flip double-sided plane if facing away
		if (plane.double_sided && dn < 0.0f)
		{
			plane_normal = -plane_normal;
			dn = -dn;
		}

		//backface culling
		if (dn <= 0.0f) return false;

		const float t{Vector3::Dot(plane.origin - ray.origin, plane_normal) / dn};
		if (t <= 0.0f) return false; //behind camera

		if (plane.half_extent.has_value()) //finite plane
		{
			const Vector3 point{ray.origin + t * ray.direction};
			const Vector3 p{plane.origin - point};
			const float t_prime{Vector3::Dot(p, plane.tangent)};
			const Vector3 b{Vector3::Cross(plane.normal, plane.tangent)};
			const float b_prime{Vector3::Dot(p, b)};
			const bool in_bounds
			{
				std::abs(t_prime) <= plane.half_extent.value().x &&
				std::abs(b_prime) <= plane.half_extent.value().y
			};
			if (!in_bounds) return false;
		}
		if (ignore_hit_record) return true;
		hit_record.ray = ray;
		hit_record.t = t;
		return true;
	}

	[[maybe_unused]]
	static bool HitTestTriangle
	(
		const Triangle& triangle,
		const Ray& ray,
		RayHitRecord& hit_record,
		const bool ignore_hit_record = false
	)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)triangle;
		(void)ray;
		(void)hit_record;
		(void)ignore_hit_record;
		return false;
	}

	[[maybe_unused]]
	static bool HitTestAABB(const AABB& aabb, const Ray& ray)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)aabb;
		(void)ray;
		return false;
	}

	[[maybe_unused]]
	static bool HitTestPrimitive
	(
		const Primitive* primitive,
		const Ray& ray,
		RayHitRecord& hit_record,
		const bool ignore_hit_record = false
	)
	{
		switch (primitive->type)
		{
		case PrimitiveType::kPlane:
			{
				const Plane plane{*static_cast<const Plane*>(primitive)};
				return HitTestPlane(plane, ray, hit_record, ignore_hit_record);
			}
		case PrimitiveType::kSphere:
			{
				const Sphere sphere{*static_cast<const Sphere*>(primitive)};
				return HitTestSphere(sphere, ray, hit_record, ignore_hit_record);
			}
		case PrimitiveType::kTriangle:
			{
				const Triangle triangle{*static_cast<const Triangle*>(primitive)};
				return HitTestTriangle(triangle, ray, hit_record, ignore_hit_record);
			}
		default:
			assert(false && "Not Implemented");
			return false;
		}
	}
}
#endif //INTERSECTIONS_HEADER
