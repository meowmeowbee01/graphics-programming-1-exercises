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
#include <stack>

namespace gfx
{
	//--- Intersection Tests ---
	[[maybe_unused]] static bool HitTestAabb(const Aabb& aabb, const Ray& ray)
	{
		//not divide by zero safe
		float t_min = (aabb.min.x - ray.origin.x) / ray.direction.x;
		float t_max = (aabb.max.x - ray.origin.x) / ray.direction.x;

		if (t_min > t_max) std::swap(t_min, t_max);

		float t_y_min = (aabb.min.y - ray.origin.y) / ray.direction.y;
		float t_y_max = (aabb.max.y - ray.origin.y) / ray.direction.y;

		if (t_y_min > t_y_max) std::swap(t_y_min, t_y_max);

		if (t_min > t_y_max || t_y_min > t_max) return false;

		if (t_y_min > t_min) t_min = t_y_min;
		if (t_y_max < t_max) t_max = t_y_max;

		float t_z_min = (aabb.min.z - ray.origin.z) / ray.direction.z;
		float t_z_max = (aabb.max.z - ray.origin.z) / ray.direction.z;

		if (t_z_min > t_z_max) std::swap(t_z_min, t_z_max);

		if (t_min > t_z_max || t_z_min > t_max) return false;

		if (t_z_min > t_min) t_min = t_z_min;
		if (t_z_max < t_max) t_max = t_z_max;

		return t_min > ray.min || t_max < ray.max;
	}

	[[maybe_unused]] static bool
	HitTestAabb(const Aabb& aabb, const Ray& ray, float& t)
	{
		//not divide by zero safe
		float t_min = (aabb.min.x - ray.origin.x) / ray.direction.x;
		float t_max = (aabb.max.x - ray.origin.x) / ray.direction.x;

		if (t_min > t_max) std::swap(t_min, t_max);

		float t_y_min = (aabb.min.y - ray.origin.y) / ray.direction.y;
		float t_y_max = (aabb.max.y - ray.origin.y) / ray.direction.y;

		if (t_y_min > t_y_max) std::swap(t_y_min, t_y_max);

		if (t_min > t_y_max || t_y_min > t_max) return false;

		if (t_y_min > t_min) t_min = t_y_min;
		if (t_y_max < t_max) t_max = t_y_max;

		float t_z_min = (aabb.min.z - ray.origin.z) / ray.direction.z;
		float t_z_max = (aabb.max.z - ray.origin.z) / ray.direction.z;

		if (t_z_min > t_z_max) std::swap(t_z_min, t_z_max);

		if (t_min > t_z_max || t_z_min > t_max) return false;

		if (t_z_min > t_min) t_min = t_z_min;
		if (t_z_max < t_max) t_max = t_z_max;

		if (t_min < ray.min && t_max > ray.max) return false;

		t = t_min;
		const auto p {ray.origin + t * ray.direction};
		constexpr float thickness {3e-2f};
		const bool close_to_x {
		  std::min(std::abs(p.x - aabb.min.x), std::abs(p.x - aabb.max.x)) <
		  thickness
		};
		const bool close_to_y {
		  std::min(std::abs(p.y - aabb.min.y), std::abs(p.y - aabb.max.y)) <
		  thickness
		};
		const bool close_to_z {
		  std::min(std::abs(p.z - aabb.min.z), std::abs(p.z - aabb.max.z)) <
		  thickness
		};
		return static_cast<int>(close_to_x) +
		  static_cast<int>(close_to_y) +
		  static_cast<int>(close_to_z) >
		  1;
	}

	[[maybe_unused]] static bool HitTestSphere(
	  const Sphere& sphere,
	  const Ray& ray,
	  RayHitRecord& hit_record,
	  const bool ignore_hit_record = false
	)
	{
		//might be faster to not do this for spheres?
		if (
		  sphere.bounding_box.has_value() &&
		  !HitTestAabb(sphere.bounding_box.value(), ray)
		)
			return false;

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
		//might be faster to not do this for triangles?
		if (
		  triangle.bounding_box.has_value() &&
		  !HitTestAabb(triangle.bounding_box.value(), ray)
		)
			return false;

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
		if (v < 0 || u + v > 1) return false;

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
		assert(!mesh.bvh_nodes.empty() && "bvh not built");
//#define BVH_VIZ_DEPTH 3
#ifdef BVH_VIZ_DEPTH
		std::vector<std::vector<BvhNode>> bvh_nodes {};
		int depth {0};
		bvh_nodes.emplace_back();
		bvh_nodes.at(0).emplace_back(mesh.bvh_nodes.at(0));
		while (depth < BVH_VIZ_DEPTH)
		{
			bvh_nodes.emplace_back();
			for (const auto& bvh_node : bvh_nodes.at(depth))
			{
				if (!bvh_node.IsLeaf())
				{
					bvh_nodes.at(depth + 1).emplace_back(
					  mesh.bvh_nodes.at(bvh_node.left_child)
					);
					bvh_nodes.at(depth + 1).emplace_back(
					  mesh.bvh_nodes.at(bvh_node.left_child + 1)
					);
				}
			}
			++depth;
		}
	//#define BVH_VIZ_INDEX 0
	#ifdef BVH_VIZ_INDEX
		float t {std::numeric_limits<float>::max()};
		if (HitTestAabb(bvh_nodes.at(depth).at(BVH_VIZ_INDEX).bounds, ray, t))
		{
			closest_hit_record.ray = ray;
			closest_hit_record.t = t;
			closest_hit_record.barycentric_coordinates = {{}};
			closest_hit_record.vertex_indices = {{0, 0, 0}};
		}
		for (size_t i {0}; i < bvh_nodes.at(depth).at(BVH_VIZ_INDEX).tri_count; ++i)
		{
			const auto index_index {
			  bvh_nodes.at(depth).at(BVH_VIZ_INDEX).first_tri + i
			};
			const auto triangle_index {mesh.bvh_tri_indices.at(index_index) * 3};
			const auto vertex0_index {mesh.indices.at(triangle_index)};
			const auto vertex1_index {mesh.indices.at(triangle_index + 1)};
			const auto vertex2_index {mesh.indices.at(triangle_index + 2)};
			const auto& v0 {mesh.vertices.at(vertex0_index)};
			const auto& v1 {mesh.vertices.at(vertex1_index)};
			const auto& v2 {mesh.vertices.at(vertex2_index)};
			const Triangle triangle {
			  Vector3 {v0.position},
			  Vector3 {v1.position},
			  Vector3 {v2.position},
			  mesh.cull_mode
			};
			RayHitRecord hit_record {
			  .object_index = closest_hit_record.object_index,
			  .vertex_indices = {{vertex0_index, vertex1_index, vertex2_index}}
			};
			if (
			  HitTestTriangle(triangle, ray, hit_record, ignore_hit_record) &&
			  closest_hit_record.t > hit_record.t
			)
			{
				closest_hit_record = hit_record;
			}
		}
		return closest_hit_record.t < std::numeric_limits<float>::max();
	#else
		for (size_t i {0}; i < bvh_nodes.at(depth).size(); ++i)
		{
			const auto& bvh_node {bvh_nodes.at(depth).at(i)};
			float t {};
			if (!HitTestAabb(bvh_node.bounds, ray, t)) continue;
			if (closest_hit_record.t < t) continue;
			if (t < 0) continue;
			closest_hit_record.object_index = i;
			closest_hit_record.ray = ray;
			closest_hit_record.t = t;
			closest_hit_record.barycentric_coordinates = {{}};
			closest_hit_record.vertex_indices = {{0, 0, 0}};
		}
				//return closest_hit_record.t < std::numeric_limits<float>::max();
	#endif
#endif
		std::stack<uint32_t> node_index_stack {};
		node_index_stack.push(0);
		while (!node_index_stack.empty())
		{
			const auto node_index {node_index_stack.top()};
			const auto& node {mesh.bvh_nodes.at(node_index)};
			node_index_stack.pop();
			if (!HitTestAabb(node.bounds, ray)) continue;
			if (!node.IsLeaf())
			{
				node_index_stack.push(node.left_child);
				node_index_stack.push(node.left_child + 1);
				continue;
			}
			for (uint32_t i = 0; i < node.tri_count; ++i)
			{
				const auto index_index {node.first_tri + i};
				const auto triangle_index {mesh.bvh_tri_indices.at(index_index) * 3};
				const auto vertex0_index {mesh.indices.at(triangle_index)};
				const auto vertex1_index {mesh.indices.at(triangle_index + 1)};
				const auto vertex2_index {mesh.indices.at(triangle_index + 2)};
				const auto& v0 {mesh.vertices.at(vertex0_index)};
				const auto& v1 {mesh.vertices.at(vertex1_index)};
				const auto& v2 {mesh.vertices.at(vertex2_index)};
				const Triangle triangle {
				  Vector3 {v0.position},
				  Vector3 {v1.position},
				  Vector3 {v2.position},
				  mesh.cull_mode
				};
				RayHitRecord hit_record {
//#define BVH_VIZ_NODE_OWNER
#ifdef BVH_VIZ_NODE_OWNER
				  .object_index = node_index,
#else
				  .object_index = closest_hit_record.object_index,
#endif
				  .vertex_indices = {{vertex0_index, vertex1_index, vertex2_index}}
				};
				if (
				  HitTestTriangle(triangle, ray, hit_record, ignore_hit_record) &&
				  closest_hit_record.t > hit_record.t
				)
				{
					closest_hit_record = hit_record;
				}
			}
		}
		return closest_hit_record.t < std::numeric_limits<float>::max();
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
		break;
		case PrimitiveType::kSphere:
		{
			const auto& sphere {dynamic_cast<const Sphere&>(primitive)};
			return HitTestSphere(sphere, ray, hit_record, ignore_hit_record);
		}
		break;
		case PrimitiveType::kTriangle:
		{
			const auto& triangle {dynamic_cast<const Triangle&>(primitive)};
			return HitTestTriangle(triangle, ray, hit_record, ignore_hit_record);
		}
		break;
		case PrimitiveType::kTriangleMesh:
		{
			const auto& mesh {dynamic_cast<const TriangleMesh&>(primitive)};
			return HitTestMesh(mesh, ray, hit_record, ignore_hit_record);
		}
		break;
		default: return false; break;
		}
	}
} //namespace gfx
#endif //INTERSECTIONS_HEADER
