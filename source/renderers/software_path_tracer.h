//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SOFTWARE_PATH_TRACER_HEADER
#define SOFTWARE_PATH_TRACER_HEADER

//--- Standard Includes ---

//--- Framework Includes ---
#include <renderer.h>

namespace gfx
{
	//--- Software Path Tracer ---
	class SoftwarePathTracer final : public Renderer
	{
	public:
		//--- Construction / Destruction ---
		SoftwarePathTracer(Context* context);
		~SoftwarePathTracer() override;

		SoftwarePathTracer(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer& operator=(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer(SoftwarePathTracer&&) = delete;
		SoftwarePathTracer& operator=(SoftwarePathTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;

	private:
		void RenderPixel
		(
			uint32_t screen_x,
			uint32_t screen_y
		) const;

		static Ray GetRay
		(
			uint32_t screen_x,
			uint32_t screen_y,
			uint32_t screen_width,
			uint32_t screen_height
		);

		ColorRgba GetColor
		(
			const RayHitRecord& closest_hit_record, const Sphere& sphere
		) const;
		ColorRgba GetColor
		(
			const RayHitRecord& closest_hit_record, const Plane& plane
		) const;
		static ColorRgba GetDepthColor(const RayHitRecord& closest_hit_record);
		static ColorRgba GetNormalColor(const ShadingInput& shading_input);

		void WriteColor
		(
			uint32_t screen_x,
			uint32_t screen_y,
			ColorRgba color
		) const;
	};
}
#endif //SOFTWARE_PATH_TRACER_HEADER
