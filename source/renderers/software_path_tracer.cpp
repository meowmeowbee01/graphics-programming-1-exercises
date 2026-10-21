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
SoftwarePathTracer::ConstructShadingInput(const RayHitRecord& hit_record) const
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
	ShadingInput shading_input {};
	auto& point {shading_input.world_position};
	auto& normal {shading_input.world_normal};
	auto& shadow_origin {shading_input.shadow_origin};
	point = hit_record.ray.origin + hit_record.ray.direction * hit_record.t;
	switch (primitive.type)
	{
	case PrimitiveType::kSphere:
	{
		const auto& sphere {dynamic_cast<const Sphere&>(primitive)};
		normal = (point - sphere.origin).Normalized();
		shadow_origin = point + normal * 1e-3f;
	}
	break;
	case PrimitiveType::kPlane:
	{
		const auto& plane {dynamic_cast<const Plane&>(primitive)};
		normal = plane.normal;
		shadow_origin = point + normal * 1e-3f;
	}
	break;
	case PrimitiveType::kTriangle:
	{
		const auto& triangle {dynamic_cast<const Triangle&>(primitive)};
		normal = triangle.normal;
		shadow_origin = point + normal * 1e-3f;
	}
	break;
	case PrimitiveType::kTriangleMesh:
	{
		assert(hit_record.vertex_indices.has_value() && "no indices");
		assert(
		  hit_record.barycentric_coordinates.has_value() &&
		  "no barycentric coordinates"
		);
		const auto& mesh {dynamic_cast<const TriangleMesh&>(primitive)};
		const auto& indices {hit_record.vertex_indices.value()};
		const auto& vertex0 {mesh.vertices.at(indices.at(0))};
		const auto& vertex1 {mesh.vertices.at(indices.at(1))};
		const auto& vertex2 {mesh.vertices.at(indices.at(2))};
		assert(
		  vertex0.normal.has_value() &&
		  vertex1.normal.has_value() &&
		  vertex2.normal.has_value() &&
		  "no normals"
		);
		const auto& normal0 {vertex0.normal.value()};
		const auto& normal1 {vertex1.normal.value()};
		const auto& normal2 {vertex2.normal.value()};
		const auto& bary {hit_record.barycentric_coordinates.value()};
		const auto& w {1.f - bary.x - bary.y};
		const auto& u {bary.x};
		const auto& v {bary.y};
		normal = (normal0 * w + normal1 * u + normal2 * v).Normalized();
		//todo: check this in the w04 solution
		auto offset0 {point - Vector3 {vertex0.position}};
		auto offset1 {point - Vector3 {vertex1.position}};
		auto offset2 {point - Vector3 {vertex2.position}};
		const auto& dot0 {std::min(0.f, Vector3::Dot(offset0, normal0))};
		const auto& dot1 {std::min(0.f, Vector3::Dot(offset1, normal1))};
		const auto& dot2 {std::min(0.f, Vector3::Dot(offset2, normal2))};
		offset0 -= dot0 * normal0;
		offset1 -= dot1 * normal1;
		offset2 -= dot2 * normal2;
		const Vector3& offset {w * offset0 + u * offset1 + v * offset2};
		shadow_origin = point + offset + normal * 1e-3f;
	}
	break;
	default: break;
	}
	if (object.instance_transformation.has_value())
	{
		const auto& matrix {object.instance_transformation.value()};
		point = matrix.TransformPoint(point);
		shadow_origin = matrix.TransformPoint(shadow_origin);
		normal = matrix.TransformNormal(normal);
	}
	return shading_input;
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

ColorRgba SoftwarePathTracer::GetLitColor(
  const ShadingInput& shading_input,
  const ColorRgba& brdf
) const
{
	ColorRgba color {};
	const auto& scene {*context_->scene_manager->GetActiveScene()};
	const auto deref {[]<typename T>(const T* p) { return *p; }};
	const auto& lights {
	  scene.lights_factory.GetAll() | std::views::transform(deref)
	};
	for (const auto& light : lights)
	{
		auto point_to_light_direction {
		  light.type == LightType::kDirectional
		    ? -light.direction
		    : light.origin - shading_input.world_position
		};
		const auto light_distance {point_to_light_direction.Normalize()};
		//light_direction.Normalize();
		const auto cos_theta {
		  Vector3::Dot(shading_input.world_normal, point_to_light_direction)
		};
		if (cos_theta <= 0.f) continue;
		const Ray shadow_ray {
		  .origin = shading_input.shadow_origin,
		  .direction = point_to_light_direction,
		  .min = 1e-3f,
		  .max = light.type == LightType::kDirectional
		    ? std::numeric_limits<float>::max()
		    : light_distance
		};
		RayHitRecord shadow_hit_record {};
		if (HitTest(shadow_ray, shadow_hit_record, true)) continue;
		const auto rad {CalculateRadiance(light, shading_input.world_position)};
		color += rad * brdf * cos_theta;
	}
	return color;
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
		const auto& scene {*context_->scene_manager->GetActiveScene()};
		const auto& object {scene.objects.at(hit_record.object_index)};
		const auto& material {*scene.materials_factory.Get(object.material_index)};
		const auto& surface {
		  material.GetSurfaceProperties(shading_input, scene.textures_factory)
		};
		const auto& brdf {surface.albedo};
		auto color = GetLitColor(shading_input, brdf)
		               .Tonemap(context_->debug_params.tone_map_operator)
		               .ToSrgb();
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
