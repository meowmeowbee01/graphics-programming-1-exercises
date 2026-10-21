//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <intersections.h>
#include <scenes.h>
#include <software_path_tracer.h>

#include <ranges>
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
#ifndef GFX_DEBUG
	#pragma omp parallel for schedule(auto) default(none) collapse(2)
#endif
	for (
	  uint32_t screen_x = 0; screen_x < context_->surface_info.width; ++screen_x
	)
		for (
		  uint32_t screen_y = 0; screen_y < context_->surface_info.height;
		  ++screen_y
		)
			RenderPixel(screen_x, screen_y);
}

//private
void SoftwarePathTracer::RenderPixel(
  const uint32_t screen_x,
  const uint32_t screen_y
) const
{
	const auto ray {GetRay(
	  screen_x,
	  screen_y,
	  context_->surface_info.width,
	  context_->surface_info.height
	)};
	RayHitRecord hit_record {};
	if (HitTest(ray, hit_record))
	{
		const auto shading_input {ConstructShadingInput(hit_record)};
		auto color {GetColor(hit_record, shading_input)};
		WriteColor(screen_x, screen_y, color);
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
	const auto& matrix {camera.GetView()};
	const Vector3 ray_direction_view {ndc_x, ndc_y, 1.f};
	const auto ray_direction {
	  matrix.TransformVector(ray_direction_view).Normalized()
	};
	const Vector3 origin {camera.GetPosition()};
	return Ray {
	  .origin = origin,
	  .direction = ray_direction,
	  .min = 1e0f,
	  .max = 5e1f
	};
}

bool SoftwarePathTracer::HitTest(
  const Ray& ray,
  RayHitRecord& closest_hit_record,
  const bool ignore_hit_record
) const
{
	const auto& scene {*context_->scene_manager->GetActiveScene()};
	bool did_hit {false};
	for (size_t i {0}; i < scene.objects.size(); ++i)
	{
		const auto& object {scene.objects.at(i)};
		auto transformed_ray {
		  object.instance_transformation.has_value()
		    ? object.instance_transformation->TransformRay(ray)
		    : ray
		};
		transformed_ray.max = std::min(ray.max, closest_hit_record.t);
		const auto& primitive {
		  *scene.primitives_factory.Get(object.primitive_index)
		};
		RayHitRecord hit_record {.object_index = static_cast<uint32_t>(i)};
		if (
		  HitTestPrimitive(
		    primitive,
		    transformed_ray,
		    hit_record,
		    ignore_hit_record
		  )
		)
		{
			if (ignore_hit_record) return true;
			did_hit = true;
			closest_hit_record = hit_record;
		}
	}
	return did_hit;
}

ShadingInput
SoftwarePathTracer::ConstructShadingInput(RayHitRecord hit_record) const
{
#ifdef BVH_VIZ_DEPTH
	return {.world_normal = {0.f, 0.f, 0.f}, .world_position = {0.f, 0.f, 0.f}};
#endif
#ifdef BVH_VIZ_NODE_OWNER
	return {.world_normal = {0.f, 0.f, 0.f}, .world_position = {0.f, 0.f, 0.f}};
#endif
	const auto& scene {*context_->scene_manager->GetActiveScene()};
	const auto& object {scene.objects.at(hit_record.object_index)};
	const auto& primitive {*scene.primitives_factory.Get(object.primitive_index)};
	auto point {hit_record.ray.origin + hit_record.ray.direction * hit_record.t};
	auto normal {GetNormal(
	  primitive,
	  point,
	  hit_record.vertex_indices,
	  hit_record.barycentric_coordinates
	)};
	if (object.instance_transformation.has_value())
	{
		const auto& matrix {object.instance_transformation->GetInverse()};
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
		  normal0.has_value() &&
		  normal1.has_value() &&
		  normal2.has_value() &&
		  "no normals"
		);
		assert(barycentric_coordinates.has_value() && "no barycentric coordinates");
		const auto& bary {barycentric_coordinates.value()};
		return {(normal0.value() * (1.f - bary.x - bary.y) +
		         normal1.value() * bary.x +
		         normal2.value() * bary.y)
		          .Normalized()};
	}
	default: return {};
	}
}

ColorRgba SoftwarePathTracer::CalculateRadiance(
  const Light& light,
  const Vector3& world_position
)
{
	switch (light.type)
	{
	case LightType::kDirectional: return light.color * light.intensity;
	case LightType::kPoint:
	{
		const auto difference {light.origin - world_position};
		return light.color * light.intensity / Vector3::Dot(difference, difference);
	}
	case LightType::kNone: return ColorRgba {0.f};
	default: return light.color * light.intensity;
	}
}

ColorRgba SoftwarePathTracer::GetColor(
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
		return ColorRgba {scaled_t};
	}
	break;
	case VisualizationMode::kNormals:
	{
		return ColorRgba {
		  (shading_input.world_normal.x + 1.f) * 0.5f,
		  (shading_input.world_normal.y + 1.f) * 0.5f,
		  (shading_input.world_normal.z + 1.f) * 0.5f
		};
	}
	break;
	default:
	{
		const auto index {hit_record.object_index};
		ColorRgba color {
		  static_cast<float>(index & 1) -
		    0.5f * static_cast<float>((index >> 3) & 1),
		  static_cast<float>((index >> 1) & 1) -
		    0.5f * static_cast<float>((index >> 3) & 1),
		  static_cast<float>((index >> 2) & 1) -
		    0.5f * static_cast<float>((index >> 3) & 1),
		};
		//ColorRgba color {};
		//const auto& scene {*context_->scene_manager->GetActiveScene()};
		//const auto deref {[]<typename T>(const T* p) { return *p; }};
		//const auto& lights {
		//  scene.lights_factory.GetAll() | std::views::transform(deref)
		//};
		//for (const auto& light : lights)
		//{
		//	auto light_direction {
		//	  light.type == LightType::kDirectional
		//	    ? -light.direction
		//	    : light.origin - shading_input.world_position
		//	};
		//	const auto light_distance {light_direction.Normalize()};
		//	const auto cos_theta {
		//	  Vector3::Dot(shading_input.world_normal, light_direction)
		//	};
		//	if (cos_theta <= 0.f) continue;
		//	const Ray shadow_ray {
		//	  .origin = shading_input.world_position,
		//	  .direction = light_direction,
		//	  .min = 1e-3f,
		//	  .max = light.type == LightType::kDirectional
		//	    ? std::numeric_limits<float>::max()
		//	    : light_distance
		//	};
		//	RayHitRecord shadow_hit_record {};
		//	if (HitTest(shadow_ray, shadow_hit_record, true)) continue;
		//	const auto rad {CalculateRadiance(light, shading_input.world_position)};
		//	color += rad * cos_theta;
		//}
		color = color.Tonemap(context_->debug_params.tone_map_operator).ToSrgb();
		color.Clamp();
		return color;
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
