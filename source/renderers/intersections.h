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

namespace gfx
{
	//--- Intersection Tests ---
	[[maybe_unused]] static bool HitTestSphere(
	  const Sphere& sphere,
	  const Ray& ray,
	  RayHitRecord& hit_record,
	  const bool ignore_hit_record = false
	)
	{
		const Vector3 origin_difference {sphere.origin - ray.origin};
		const float projection_size {
		  Vector3::Dot(origin_difference, ray.direction)
		};
		const float rejection_size {
		  Vector3::Reject(origin_difference, ray.direction).Magnitude()
		};
		//r == reject for tangential hit
		if (sphere.radius <= rejection_size) return false;
		const float projection_distance_difference {
		  std::sqrtf(std::powf(sphere.radius, 2) - std::powf(rejection_size, 2))
		};
		const float t {projection_size - projection_distance_difference};
		if (ray.min > t || t > ray.max) return false;
		if (ignore_hit_record) return true;
		hit_record.ray = ray;
		hit_record.t = t;
		return true;
	}

	[[maybe_unused]] static bool HitTestPlane(
	  const Plane& plane,
	  const Ray& ray,
	  RayHitRecord& hit_record,
	  const bool ignore_hit_record = false
	)
	{
		Vector3 normal {plane.normal};
		float dn {Vector3::Dot(ray.direction, plane.normal)};

		//flip double-sided plane if facing away
		if (plane.double_sided && dn > 0.0f)
		{
			normal = -normal;
			dn = -dn;
		}

		//backface culling
		if (dn >= 0.0f) return false;

		const float t {Vector3::Dot(plane.origin - ray.origin, normal) / dn};
		if (ray.min > t || t > ray.max) return false;

		if (plane.half_extent.has_value()) //finite plane
		{
			const Vector3 point {ray.origin + t * ray.direction};
			const Vector3 p {plane.origin - point};
			const float t_prime {Vector3::Dot(p, plane.tangent)};
			const Vector3 b {Vector3::Cross(plane.normal, plane.tangent)};
			const float b_prime {Vector3::Dot(p, b)};
			const bool in_bounds {
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

	[[maybe_unused]] static bool HitTestTriangle(
	  const Triangle& triangle,
	  const Ray& ray,
	  RayHitRecord& hit_record,
	  const bool ignore_hit_record = false
	)
	{
		const auto dn {Vector3::Dot(ray.direction, triangle.normal)};

		if (std::abs(dn) < 1e-4f) return false; //parallel

		if (
		  (triangle.cull_mode == CullMode::kBackFaceCulling && dn >= 0.f) ||
		  (triangle.cull_mode == CullMode::kFrontFaceCulling && dn <= 0.f)
		)
			return false;

		const auto edge1 {triangle.v1 - triangle.v0};
		const auto edge2 {triangle.v2 - triangle.v0};
		const auto h {Vector3::Cross(ray.direction, edge2)};
		const auto det {Vector3::Dot(edge1, h)};
		const auto inv_det {1.f / det};

		const auto s {ray.origin - triangle.v0};
		const auto u {Vector3::Dot(s, h) * inv_det};
		if (u < 0 || u > 1) return false;

		const auto q {Vector3::Cross(s, edge1)};
		const auto v {Vector3::Dot(ray.direction, q) * inv_det};
		if (v < 0 || (u + v) > 1) return false;

		const auto t {Vector3::Dot(edge2, q) * inv_det};
		if (ray.min > t || t > ray.max) return false;

		if (ignore_hit_record) return true;
		hit_record.t = t;
		hit_record.ray = ray;
		hit_record.barycentric_coordinates = {u, v};
		return true;
	}

	static bool HitTestMesh(
	  const TriangleMesh& mesh,
	  const Ray& ray,
	  RayHitRecord& closest_hit_record,
	  const bool ignore_hit_record = false
	)
	{
		bool did_hit {false};
		for (size_t i {0}; i < mesh.indices.size() - 2; i += 3)
		{
			const auto index_v0 {mesh.indices.at(i)};
			const auto index_v1 {mesh.indices.at(i + 1)};
			const auto index_v2 {mesh.indices.at(i + 2)};
			const Triangle triangle {
			  Vector3 {mesh.vertices.at(index_v0).position},
			  Vector3 {mesh.vertices.at(index_v1).position},
			  Vector3 {mesh.vertices.at(index_v2).position},
			  mesh.cull_mode
			};
			RayHitRecord hit_record {
			  .object_index = closest_hit_record.object_index,
			  .vertex_indices = {{index_v0, index_v1, index_v2}},
			};
			if (
			  HitTestTriangle(triangle, ray, hit_record, ignore_hit_record) &&
			  (!did_hit || closest_hit_record.t > hit_record.t)
			)
			{
				did_hit = true;
				closest_hit_record = hit_record;
			}
		}
		return did_hit;
	}

	[[maybe_unused]] static bool HitTestAabb(const AABB& aabb, const Ray& ray)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)aabb;
		(void)ray;
		return false;
	}

	[[maybe_unused]] static bool HitTestPrimitive(
	  const Primitive& primitive,
	  const Ray& ray,
	  RayHitRecord& hit_record,
	  const bool ignore_hit_record = false
	)
	{
		switch (primitive.type)
		{
		case PrimitiveType::kPlane:
		{
			const auto& plane {dynamic_cast<const Plane&>(primitive)};
			return HitTestPlane(plane, ray, hit_record, ignore_hit_record);
		}
		case PrimitiveType::kSphere:
		{
			const auto& sphere {dynamic_cast<const Sphere&>(primitive)};
			return HitTestSphere(sphere, ray, hit_record, ignore_hit_record);
		}
		case PrimitiveType::kTriangle:
		{
			const auto& triangle {dynamic_cast<const Triangle&>(primitive)};
			return HitTestTriangle(triangle, ray, hit_record, ignore_hit_record);
		}
		case PrimitiveType::kTriangleMesh:
		{
			const auto& mesh {dynamic_cast<const TriangleMesh&>(primitive)};
			return HitTestMesh(mesh, ray, hit_record, ignore_hit_record);
		}
		default: return false;
		}
	}
} //namespace gfx
#endif //INTERSECTIONS_HEADER
