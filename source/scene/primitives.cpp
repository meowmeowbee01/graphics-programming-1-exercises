//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <primitives.h>

#include <cassert>
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

void TriangleMesh::UpdateAabb()
{
	assert(!vertices.empty());
	Aabb aabb {
	  vertices.at(0).position.AsVector3(),
	  vertices.at(0).position.AsVector3()
	};
	for (const auto& vertex : vertices)
		aabb.ExpandToInclude(Vector3 {vertex.position});
	bounding_box = aabb;
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
