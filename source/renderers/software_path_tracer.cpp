//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <software_path_tracer.h>
#include <scenes.h>
#include <intersections.h>
using namespace gfx;

// =============================================================================
// Construction / Destruction
// =============================================================================
SoftwarePathTracer::SoftwarePathTracer(Context* const context)
	: Renderer(context)
{
}

SoftwarePathTracer::~SoftwarePathTracer() = default;


// =============================================================================
// Public Functions
// =============================================================================
void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	const uint32_t screen_width{context_->surface_info.width};
	const uint32_t screen_height{context_->surface_info.height};
	for (uint32_t screen_x{0}; screen_x < screen_width; ++screen_x)
		for (uint32_t screen_y{0}; screen_y < screen_height; ++screen_y)
			RenderPixel(screen_x, screen_y);
}

//private

void SoftwarePathTracer::RenderPixel
(
	const uint32_t screen_x,
	const uint32_t screen_y
) const
{
	// const Sphere test_sphere{{0.f, 0.f, 100.f}, 50.f};
	// const Plane test_plane{{0.f, -50.f, 0.f}, {0.f, 1.f, 0.f}};
	const Plane test_plane
	{
		{0.f, -50.f, 0.f},
		{0.f, 1.f, 0.f},
		true,
		Vector2{100.f, 100.f},
	};
	const Ray ray
	{
		GetRay
		(
			screen_x,
			screen_y,
			context_->surface_info.width,
			context_->surface_info.height
		)
	};
	if
	(
		RayHitRecord closest_hit_record{};
		HitTestPlane(test_plane, ray, closest_hit_record)
		// HitTestSphere(test_sphere, ray, closest_hit_record)
	)
	{
		WriteColor(screen_x, screen_y, GetColor(closest_hit_record, test_plane));
	}
}

Ray SoftwarePathTracer::GetRay
(
	const uint32_t screen_x,
	const uint32_t screen_y,
	const uint32_t screen_width,
	const uint32_t screen_height
)
{
	const float aspect_ratio
	{
		static_cast<float>(screen_width) / static_cast<float>(screen_height)
	};
	const float ndc_x
	{
		(2.f * ((screen_x + 0.5f) / screen_width) - 1.f) * aspect_ratio
	};
	const float ndc_y{1.f - 2.f * ((screen_y + 0.5f) / screen_height)};
	Vector3 ray_direction{ndc_x, ndc_y, 1.f};
	ray_direction.Normalize();
	const Ray ray
	{
		.origin = {0.f, 0.f, 0.f},
		.direction = ray_direction
	};
	return ray;
}

ColorRgba SoftwarePathTracer::GetColor
(
	const RayHitRecord& closest_hit_record,
	const Sphere& sphere
) const
{
	const Vector3 world_position
	{
		closest_hit_record.ray.origin +
		closest_hit_record.ray.direction * closest_hit_record.t
	};
	const Vector3 world_normal{(world_position - sphere.origin).Normalized()};
	const ShadingInput shading_input
	{
		.view_direction{},
		.light_direction{},
		.world_normal{world_normal},
		.world_position{world_position},
	};
	if (context_->debug_params.visualization_mode == VisualizationMode::kDepth)
		return GetDepthColor(closest_hit_record);
	if (context_->debug_params.visualization_mode == VisualizationMode::kNormals)
		return GetNormalColor(shading_input);
	//context_->debug_params.visualization_mode == VisualizationMode::kNone
	return ColorRgba{1.f, 0.f, 0.f};
}

ColorRgba SoftwarePathTracer::GetColor
(
	const RayHitRecord& closest_hit_record,
	const Plane& plane
) const
{
	const Vector3 world_position
	{
		closest_hit_record.ray.origin +
		closest_hit_record.ray.direction * closest_hit_record.t
	};
	const ShadingInput shading_input
	{
		.view_direction{},
		.light_direction{},
		.world_normal{plane.normal},
		.world_position{world_position},
	};
	if (context_->debug_params.visualization_mode == VisualizationMode::kDepth)
		return GetDepthColor(closest_hit_record);
	if (context_->debug_params.visualization_mode == VisualizationMode::kNormals)
		return GetNormalColor(shading_input);
	//context_->debug_params.visualization_mode == VisualizationMode::kNone
	return ColorRgba{1.f, 0.f, 0.f};
}

ColorRgba SoftwarePathTracer::GetDepthColor
(
	const RayHitRecord& closest_hit_record
)
{
	constexpr float max_depth{100.f};
	const float scaled_t
	{
		1.f - std::clamp(closest_hit_record.t / max_depth, 0.f, 1.f)
	};
	return ColorRgba{scaled_t};
}

ColorRgba SoftwarePathTracer::GetNormalColor(const ShadingInput& shading_input)
{
	const Vector3& n{shading_input.world_normal};
	return ColorRgba
	{
		(n.x + 1.f) * 0.5f,
		(n.y + 1.f) * 0.5f,
		(n.z + 1.f) * 0.5f,
	};
}

void SoftwarePathTracer::WriteColor(
	const uint32_t screen_x,
	const uint32_t screen_y,
	const ColorRgba color
) const
{
	const uint32_t pixel_index
	{
		screen_x + screen_y * context_->surface_info.width
	};
	context_->surface_info.pixel_buffer[pixel_index] =
		SDL_MapRGB
		(
			context_->surface_info.pixel_format_details,
			nullptr,
			static_cast<uint8_t>(color.r * 255.f),
			static_cast<uint8_t>(color.g * 255.f),
			static_cast<uint8_t>(color.b * 255.f)
		);
}
