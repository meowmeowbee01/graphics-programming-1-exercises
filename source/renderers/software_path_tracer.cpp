//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <intersections.h>
#include <scenes.h>
#include <software_path_tracer.h>
using namespace gfx;

//=============================================================================
//Construction / Destruction
//=============================================================================
SoftwarePathTracer::SoftwarePathTracer(Context* const context)
  : Renderer(context)
{
}

SoftwarePathTracer::~SoftwarePathTracer() = default;

//=============================================================================
//Public Functions
//=============================================================================
void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	const uint32_t screen_width {context_->surface_info.width};
	const uint32_t screen_height {context_->surface_info.height};
	for (uint32_t screen_x {0}; screen_x < screen_width; ++screen_x)
		for (uint32_t screen_y {0}; screen_y < screen_height; ++screen_y)
			RenderPixel(screen_x, screen_y);
}

//private
void SoftwarePathTracer::RenderPixel(
  const uint32_t screen_x,
  const uint32_t screen_y
) const
{
	const Ray ray {GetRay(
	  screen_x,
	  screen_y,
	  context_->surface_info.width,
	  context_->surface_info.height
	)};
	RayHitRecord hit_record {};
	if (HitTest(ray, hit_record))
	{
		const ShadingInput shading_input {ConstructShadingInput(ray, hit_record)};
		Visualize(screen_x, screen_y, hit_record, shading_input);
	}
	else
		WriteColor(
		  screen_x,
		  screen_y,
		  context_->scene_manager->GetActiveScene()->background_color
		);
}

Ray SoftwarePathTracer::GetRay(
  const uint32_t screen_x,
  const uint32_t screen_y,
  const uint32_t screen_width,
  const uint32_t screen_height
) const
{
	const float aspect_ratio {
	  static_cast<float>(screen_width) / static_cast<float>(screen_height)
	};
	const auto& camera {context_->scene_manager->GetActiveScene()->camera};
	const float fov_radians {
	  std::numbers::pi_v<float> / 180.f * camera.GetFovAngle()
	};
	const float fov {std::tanf(fov_radians / 2)};
	const float ndc_x {
	  (2.f *
	     ((static_cast<float>(screen_x) + 0.5f) /
	      static_cast<float>(screen_width)) -
	   1.f) *
	  aspect_ratio *
	  fov
	};
	const float ndc_y {
	  (1.f -
	   2.f *
	     ((static_cast<float>(screen_y) + 0.5f) /
	      static_cast<float>(screen_height))) *
	  fov
	};
	const auto& inv_matrix {camera.GetView()};
	const Vector3 ray_direction_view {ndc_x, ndc_y, 1.f};
	const Vector3 ray_direction {
	  inv_matrix.TransformVector(ray_direction_view).Normalized()
	};
	const Vector3 origin {camera.GetPosition()};
	return Ray {.origin = origin, .direction = ray_direction};
}

bool SoftwarePathTracer::HitTest(
  const Ray& ray,
  RayHitRecord& closest_hit_record
) const
{
	const auto& scene {*context_->scene_manager->GetActiveScene()};
	for (size_t i {0}; i < scene.objects.size(); ++i)
	{
		const auto& object {scene.objects.at(i)};
		Ray transformed_ray {
		  object.instance_transformation.has_value()
		    ? object.instance_transformation.value().TransformRay(ray)
		    : ray
		};
		transformed_ray.max = std::min(ray.max, closest_hit_record.t);
		const auto& primitive {
		  *scene.primitives_factory.Get(object.primitive_index)
		};
		RayHitRecord hit_record {.object_index = static_cast<uint32_t>(i)};
		if (HitTestPrimitive(primitive, transformed_ray, hit_record))
			closest_hit_record = hit_record;
	}
	return closest_hit_record.t < std::numeric_limits<float>::max();
}

ShadingInput SoftwarePathTracer::ConstructShadingInput(
  const Ray& ray,
  RayHitRecord hit_record
) const
{
	const auto& scene {*context_->scene_manager->GetActiveScene()};
	const auto& object {scene.objects.at(hit_record.object_index)};
	const auto& primitive {*scene.primitives_factory.Get(object.primitive_index)};
	auto point {ray.origin + ray.direction * hit_record.t};
	auto normal {GetNormal(
	  primitive,
	  point,
	  hit_record.vertex_indices,
	  hit_record.barycentric_coordinates
	)};
	if (object.instance_transformation.has_value())
	{
		const auto& matrix {object.instance_transformation.value().GetInverse()};
		point = matrix.TransformPoint(point);
		normal = matrix.TransformNormal(normal);
	}
	const ShadingInput shading_input {
	  .world_normal = normal,
	  .world_position = point
	};
	return shading_input;
}

Vector3 SoftwarePathTracer::GetNormal(
  const Primitive& primitive,
  const Vector3& point,
  const std::optional<std::array<uint32_t, 3>>& vertex_indices,
  const std::optional<Vector2> barycentric_coordinates
)
{
	switch (primitive.type)
	{
	case PrimitiveType::kSphere:
	{
		const auto& sphere {dynamic_cast<const Sphere&>(primitive)};
		return (point - sphere.origin).Normalized();
	}
	case PrimitiveType::kPlane:
	{
		const auto& plane {dynamic_cast<const Plane&>(primitive)};
		return plane.normal;
	}
	case PrimitiveType::kTriangle:
	{
		const auto& triangle {dynamic_cast<const Triangle&>(primitive)};
		return triangle.normal;
	}
	case PrimitiveType::kTriangleMesh:
	{
		assert(vertex_indices.has_value() && "no vertex indices");
		const auto& mesh {dynamic_cast<const TriangleMesh&>(primitive)};
		const auto& normal0 {mesh.vertices.at(vertex_indices.value().at(0)).normal};
		const auto& normal1 {mesh.vertices.at(vertex_indices.value().at(1)).normal};
		const auto& normal2 {mesh.vertices.at(vertex_indices.value().at(2)).normal};
		assert(
		  (normal0.has_value() && normal1.has_value() && normal2.has_value()) &&
		  "no normals"
		);
		assert(barycentric_coordinates.has_value() && "no barycentric coordinates");
		const auto& bary {barycentric_coordinates.value()};
		return {(normal0.value() * (1.f - bary.x - bary.y) +
		         normal1.value() * bary.x +
		         normal2.value() * bary.y)
		          .Normalized()};
	}
	default:
		//TODO: concepts?
		assert(false && "no normal");
		return {};
	}
}

void SoftwarePathTracer::Visualize(
  const uint32_t screen_x,
  const uint32_t screen_y,
  const RayHitRecord& hit_record,
  const ShadingInput& shading_input
) const
{
	switch (context_->debug_params.visualization_mode)
	{
	case VisualizationMode::kDepth:
	{
		constexpr float max_depth {100.f};
		const float scaled_t {1.f - std::clamp(hit_record.t / max_depth, 0.f, 1.f)};
		WriteColor(screen_x, screen_y, ColorRgba {scaled_t});
	}
	break;
	case VisualizationMode::kNormals:
	{
		const ColorRgba color {
		  (shading_input.world_normal.x + 1.f) * 0.5f,
		  (shading_input.world_normal.y + 1.f) * 0.5f,
		  (shading_input.world_normal.z + 1.f) * 0.5f
		};
		WriteColor(screen_x, screen_y, color);
	}
	break;
	default:
	{
		const auto index {hit_record.object_index};
		const ColorRgba color {
		  static_cast<float>(index & 1),
		  static_cast<float>((index >> 1) & 1),
		  static_cast<float>((index >> 2) & 1)
		};
		WriteColor(screen_x, screen_y, color);
	}
	break;
	}
}

void SoftwarePathTracer::WriteColor(
  const uint32_t screen_x,
  const uint32_t screen_y,
  const ColorRgba color
) const
{
	const uint32_t pixel_index {
	  screen_x + screen_y * context_->surface_info.width
	};
	context_->surface_info.pixel_buffer[pixel_index] = SDL_MapRGB(
	  context_->surface_info.pixel_format_details,
	  nullptr,
	  static_cast<uint8_t>(color.r * 255.f),
	  static_cast<uint8_t>(color.g * 255.f),
	  static_cast<uint8_t>(color.b * 255.f)
	);
}
