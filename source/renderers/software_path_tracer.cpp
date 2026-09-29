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

	const SurfaceInfo& surface_info = context_->surface_info;
	for (uint32_t screen_x = 0; screen_x < surface_info.width; ++screen_x)
	{
		for (uint32_t screen_y = 0; screen_y < surface_info.height; ++screen_y)
		{
			float ndc_x = 2.f * ((screen_x + 0.5f) / surface_info.width) - 1.f;
			float ndc_y = 1.f - 2.f * ((screen_y + 0.5f) / surface_info.height);
			Vector3 ray_direction
			{
				ndc_x,
				ndc_y,
				1.f
			};
			ray_direction.Normalize();
			ColorRgba color
			{
				ray_direction.x,
				ray_direction.y,
				ray_direction.z
			};
			color.MaxToOne();
			surface_info.pixel_buffer[screen_x + (screen_y * surface_info.width)] =
				SDL_MapRGB
				(
					surface_info.pixel_format_details,
					nullptr,
					static_cast<uint8_t>(color.r * 255),
					static_cast<uint8_t>(color.g * 255),
					static_cast<uint8_t>(color.b * 255)
				);
		}
	}
}
