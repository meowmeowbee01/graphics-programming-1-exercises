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
		RayHitRecord hit_record{};
		HitTest(ray, hit_record)
	)
	{
		const auto& scene{*context_->scene_manager->GetActiveScene()};
		const auto object{scene.objects.at(hit_record.object_index)};
		const auto* primitive{scene.primitives_factory.Get(object.primitive_index)};
		const auto point{ray.origin + ray.direction * hit_record.t};
		const auto normal{GetNormal(primitive, point)};
		const ShadingInput shading_input
		{
			.world_normal = normal,
			.world_position = point
		};
		Visualize(screen_x, screen_y, hit_record, shading_input);
	}
	else WriteColor(screen_x, screen_y, ColorRgba{0.f});
}

Ray SoftwarePathTracer::GetRay
(
	const uint32_t screen_x,
	const uint32_t screen_y,
	const uint32_t screen_width,
	const uint32_t screen_height
) const
{
	const float aspect_ratio
	{
		static_cast<float>(screen_width) / static_cast<float>(screen_height)
	};
	const auto camera{context_->scene_manager->GetActiveScene()->camera};
	const float fov_radians
	{
		std::numbers::pi_v<float> / 180.f * camera.GetFovAngle()
	};
	const float fov{std::tanf(fov_radians / 2)};
	const float ndc_x
	{
		(2.f * ((screen_x + 0.5f) / screen_width) - 1.f) * aspect_ratio * fov
	};
	const float ndc_y{(1.f - 2.f * ((screen_y + 0.5f) / screen_height)) * fov};
	Vector3 ray_direction{ndc_x, ndc_y, 1.f};
	ray_direction.Normalize();
	const Vector3 origin{camera.GetPosition()};
	return Ray
	{
		.origin = origin,
		.direction = ray_direction
	};
}

bool SoftwarePathTracer::HitTest
(
	const Ray& ray,
	RayHitRecord& closest_hit_record
) const
{
	bool did_hit{false};
	const auto& scene{*context_->scene_manager->GetActiveScene()};
	for (size_t i{0}; i < scene.objects.size(); ++i)
	{
		const auto& object{scene.objects.at(i)};
		const auto primitive{scene.primitives_factory.Get(object.primitive_index)};
		if
		(
			RayHitRecord hit_record{.object_index = static_cast<uint32_t>(i)};
			HitTestPrimitive(primitive, ray, hit_record) &&
			(!did_hit || closest_hit_record.t > hit_record.t)
		)
		{
			closest_hit_record = hit_record;
			did_hit = true;
		}
	}
	return did_hit;
}

Vector3 SoftwarePathTracer::GetNormal
(
	const Primitive* primitive,
	Vector3 point
)
{
	switch (primitive->type)
	{
	case PrimitiveType::kSphere:
		{
			const Sphere sphere{*static_cast<const Sphere*>(primitive)};
			return (point - sphere.origin).Normalized();
		}
	case PrimitiveType::kPlane:
		{
			const Plane plane{*static_cast<const Plane*>(primitive)};
			return plane.normal;
		}
	case PrimitiveType::kTriangle:
		{
			const Triangle triangle{*static_cast<const Triangle*>(primitive)};
			return triangle.normal;
		}
	default:
		assert(false && "no normal");
		return Vector3{};
	}
}

void SoftwarePathTracer::Visualize
(
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
			constexpr float max_depth{100.f};
			const float scaled_t
			{
				1.f - std::clamp(hit_record.t / max_depth, 0.f, 1.f)
			};
			WriteColor(screen_x, screen_y, ColorRgba{scaled_t});
		}
		break;
	case VisualizationMode::kNormals:
		{
			const ColorRgba color
			{
				(shading_input.world_normal.x + 1.f) * 0.5f,
				(shading_input.world_normal.y + 1.f) * 0.5f,
				(shading_input.world_normal.z + 1.f) * 0.5f
			};
			WriteColor(screen_x, screen_y, color);
		}
		break;
	default:
		{
			const auto index{hit_record.object_index};
			const ColorRgba color{
				static_cast<float>(index & 1),
				static_cast<float>((index >> 1) & 1),
				static_cast<float>((index >> 2) & 1)
			};
			WriteColor(screen_x, screen_y, color);
		}
		break;
	}
}

void SoftwarePathTracer::WriteColor
(
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
