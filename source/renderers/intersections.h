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
		// //analytic solution: confused about tMin and tMax
		// //t is intersection point(s)
		// //at^2+bt+c=0
		// const float a{Vector3::Dot(ray.direction, ray.direction)};
		// const float b
		// {
		// 	Vector3::Dot(ray.direction * 2.f, ray.origin - sphere.origin)
		// };
		// const float c
		// {
		// 	Vector3::Dot(ray.origin - sphere.origin, ray.origin - sphere.origin) -
		// 	sphere.radius * sphere.radius
		// };
		// const float discriminant{b * b - 4.f * a * c};
		// if (discriminant <= 0.f) return false;
		// const float t1{(-b + discriminant) / (2.f * a)};
		// const float t2{(-b - discriminant) / (2.f * a)};
		// const Vector3 intersection_point1{ray.origin + ray.direction * t1};
		// const Vector3 intersection_point2{ray.origin + ray.direction * t2};
		// //tMin tMax???
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
		const float distance{projection_size - projection_distance_difference};
		hit_record.ray = ray;
		hit_record.t = distance;
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
			const bool inBounds
			{
				std::abs(t_prime) <= plane.half_extent.value().x &&
				std::abs(b_prime) <= plane.half_extent.value().y
			};
			if (!inBounds) return false;
		}
		if (ignore_hit_record) return true;
		hit_record.ray = ray;
		hit_record.t = t;
		return true;
	}

	[[maybe_unused]]
	static bool HitTestTriangle(const Triangle& triangle, const Ray& ray,
															RayHitRecord& hit_record, const bool ignore_hit_record = false)
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
}
#endif //INTERSECTIONS_HEADER
