//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <primitives.h>

using namespace gfx;

//--- Primitives AABB Updates ---
void Sphere::UpdateAabb()
{
	const Aabb aabb {
	  {origin.x - radius, origin.y - radius, origin.z - radius},
	  {origin.x + radius, origin.y + radius, origin.z + radius}
	};
	bounding_box = aabb;
}

void Triangle::UpdateAabb()
{
	const Aabb aabb {
	  {std::min({v0.x, v1.x, v2.x}),
	   std::min({v0.y, v1.y, v2.y}),
	   std::min({v0.z, v1.z, v2.z})},
	  {std::max({v0.x, v1.x, v2.x}),
	   std::max({v0.y, v1.y, v2.y}),
	   std::max({v0.z, v1.z, v2.z})}
	};
	bounding_box = aabb;
}

void TriangleMesh::BuildBvh(
  const std::vector<Vector3>& triangle_centers,
  const std::vector<Aabb>& triangle_bounds,
  const uint32_t parent_index
)
{
	const auto& [bounds, left_child, first_tri, tri_count] {
	  bvh_nodes.at(parent_index)
	};
	Aabb half {bounds};
	if (half.GetSize().x > half.GetSize().y)
		if (half.GetSize().x > half.GetSize().z)
			half.max.x -= half.GetSize().x / 2.f;
		else half.max.y -= half.GetSize().y / 2.f;
	else if (half.GetSize().y > half.GetSize().z)
		half.max.y -= half.GetSize().y / 2.f;
	else half.max.z -= half.GetSize().z / 2.f;

	auto right_index {first_tri + tri_count};
	auto left_index {first_tri};
	while (left_index < right_index)
	{
		const auto is_in_left_half {
		  half.Contains(triangle_centers.at(bvh_tri_indices.at(left_index)))
		};
		if (is_in_left_half) ++left_index;
		else
			std::swap(
			  bvh_tri_indices.at(left_index),
			  bvh_tri_indices.at(--right_index)
			);
	}

	auto left_count {left_index - first_tri};
	if (left_count == 0 || left_count == tri_count)
		left_index = first_tri + tri_count / 2;

	left_count = left_index - first_tri;
	const auto right_count {tri_count - left_count};
	Aabb left_bounds {};
	for (size_t i {0}; i < left_count; ++i)
	{
		const auto index {bvh_tri_indices.at(first_tri + i)};
		left_bounds.ExpandToInclude(triangle_bounds.at(index));
	}
	Aabb right_bounds {};
	for (size_t i {0}; i < right_count; ++i)
	{
		const auto index {bvh_tri_indices.at(first_tri + left_count + i)};
		right_bounds.ExpandToInclude(triangle_bounds.at(index));
	}

	//between here
	// for (size_t i = 0; i < bvh_nodes.at(parent_index).tri_count; ++i)
	// {
	// 	//calculate the triangles index
	// 	const auto tri_index {
	// 	  bvh_nodes.at(parent_index).first_tri + static_cast<uint32_t>(i)
	// 	};
	// 	//put it left or right
	// 	const auto is_in_half {half.Contains(triangle_centers.at(tri_index))};
	// 	if (is_in_half) left_indices.emplace_back(tri_index);
	// 	else right_indices.emplace_back(tri_index);
	// }
	//
	// //if either is empty: early exit
	// if (right_indices.empty() || left_indices.empty()) return;

	//smallest aabb for the left
	// Aabb left_bounds {};
	// for (const uint32_t index : left_indices)
	// 	left_bounds.ExpandToInclude(triangle_bounds.at(index));
	// //smallest aabb for the right
	// Aabb right_bounds {};
	// for (const uint32_t index : right_indices)
	// 	right_bounds.ExpandToInclude(triangle_bounds.at(index));
	//and here

	bvh_nodes.at(parent_index).left_child = bvh_nodes.size();

	BvhNode left {
	  .bounds = left_bounds,
	  .first_tri = first_tri,
	  .tri_count = left_count
	};
	BvhNode right {
	  .bounds = right_bounds,
	  .first_tri = left.first_tri + left.tri_count,
	  .tri_count = right_count
	};
	bvh_nodes.emplace_back(left);
	bvh_nodes.emplace_back(right);

	//for (size_t i = 0; i < left_indices.size(); ++i)
	//bvh_tri_indices.at(i + left.first_tri) = left_indices.at(i);
	//for (size_t i = 0; i < right_indices.size(); ++i)
	//bvh_tri_indices.at(i + right.first_tri) = right_indices.at(i);

	if (!left.IsTiny())
	{
		BuildBvh(
		  triangle_centers,
		  triangle_bounds,
		  bvh_nodes.at(parent_index).left_child
		);
	}
	if (!right.IsTiny())
		BuildBvh(
		  triangle_centers,
		  triangle_bounds,
		  bvh_nodes.at(parent_index).left_child + 1
		);
}

void TriangleMesh::UpdateAabb()
{
	bounding_box = Aabb {};
	for (const auto& vertex : vertices)
		bounding_box->ExpandToInclude(Vector3 {vertex.position});

	const auto triangle_count {static_cast<uint32_t>(indices.size()) / 3};

	bvh_tri_indices.clear();
	std::vector<Vector3> triangle_centers(triangle_count);
	std::vector<Aabb> triangle_bounds(triangle_count);
	for (size_t i = 0; i < triangle_count; ++i)
	{
		bvh_tri_indices.emplace_back(i);
		const auto v0 {vertices.at(indices.at(i * 3)).position.AsVector3()};
		const auto v1 {vertices.at(indices.at(i * 3 + 1)).position.AsVector3()};
		const auto v2 {vertices.at(indices.at(i * 3 + 2)).position.AsVector3()};
		triangle_bounds.at(i).ExpandToInclude(v0);
		triangle_bounds.at(i).ExpandToInclude(v1);
		triangle_bounds.at(i).ExpandToInclude(v2);
		triangle_centers.at(i) = triangle_bounds.at(i).GetCenter();
	}

	bvh_nodes.clear();
	BvhNode root {
	  .bounds = bounding_box.value(),
	  .first_tri = 0,
	  .tri_count = triangle_count
	};
	bvh_nodes.emplace_back(root);

	BuildBvh(triangle_centers, triangle_bounds, 0);
}

//--- AABB ---
void Aabb::ExpandToInclude(const Vector3& point)
{
	min = Vector3::Min(min, point);
	max = Vector3::Max(max, point);
}

void Aabb::ExpandToInclude(const Aabb& other)
{
	min = Vector3::Min(min, other.min);
	max = Vector3::Max(max, other.max);
}

bool Aabb::Contains(const Vector3& point) const
{
	return point.x >= min.x &&
	  point.x <= max.x &&
	  point.y >= min.y &&
	  point.y <= max.y &&
	  point.z >= min.z &&
	  point.z <= max.z;
}

bool Aabb::Overlaps(const Aabb& other) const
{
	return min.x <= other.max.x &&
	  max.x >= other.min.x &&
	  min.y <= other.max.y &&
	  max.y >= other.min.y &&
	  min.z <= other.max.z &&
	  max.z >= other.min.z;
}

Vector3 Aabb::GetCenter() const
{
	return {
	  (min.x + max.x) * 0.5f,
	  (min.y + max.y) * 0.5f,
	  (min.z + max.z) * 0.5f
	};
}

Vector3 Aabb::GetSize() const
{ return {max.x - min.x, max.y - min.y, max.z - min.z}; }

bool Aabb::IsValid() const
{ return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
