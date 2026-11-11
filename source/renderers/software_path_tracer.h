//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SOFTWARE_PATH_TRACER_HEADER
#define SOFTWARE_PATH_TRACER_HEADER

//--- Framework Includes ---
#include "samplers.h"

#include <renderer.h>

namespace gfx
{
	//--- Software Path Tracer ---
	class SoftwarePathTracer final : public Renderer
	{
	public:
		//--- Construction / Destruction ---
		explicit SoftwarePathTracer(Context* context);
		~SoftwarePathTracer() override;

		SoftwarePathTracer(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer& operator=(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer(SoftwarePathTracer&&) = delete;
		SoftwarePathTracer& operator=(SoftwarePathTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;

	private:
		const uint32_t ao_sample_count_ {1};
		const float ao_radius_ {10.f};

		VisualizationMode last_vis_mode_ {VisualizationMode::kNone};
		SamplingStrategy last_sampling_strategy_ {SamplingStrategy::kCosine};
		std::vector<ColorRgba> accumulation_buffer_ {};

		[[nodiscard]] bool IsDebugView() const
		{ return gfx::IsDebugView(context_->debug_params.visualization_mode); }

		void RenderPixel(uint32_t screen_x, uint32_t screen_y);

		[[nodiscard]] Ray
		GetRay(uint32_t screen_x, uint32_t screen_y, uint32_t& hash_state) const;

		bool HitTest(
		  const Ray& ray,
		  RayHitRecord& closest_hit_record,
		  bool ignore_hit_record = false
		) const;

		[[nodiscard]] ShadingInput
		ConstructShadingInput(const RayHitRecord& hit_record) const;

		static ColorRgba
		CalculateRadiance(const Light& light, const Vector3& world_position);

		[[nodiscard]] ColorRgba DirectIllumination(
		  const ShadingInput& shading_input,
		  const Scene& scene,
		  const Material& material,
		  const SurfaceProperties& surface
		) const;

		[[nodiscard]] ColorRgba GetLinearColor(
		  const RayHitRecord& hit_record,
		  const ShadingInput& shading_input,
		  uint32_t& hash_state
		) const;

		void WriteColor(uint32_t pixel_index, const ColorRgba& color);

		void WriteColorToScreen(uint32_t pixel_index, const ColorRgba& color) const;

		void WriteColorToBuffer(uint32_t pixel_index, const ColorRgba& color);

		void WriteColorFromBufferToScreen(uint32_t pixel_index) const;

		[[nodiscard]] float
		TraceAo(const ShadingInput& shading_input, uint32_t& hash_state) const;

		[[nodiscard]] HemiSphereSample
		GetHemiSphereSample(const Vector3& normal, uint32_t& hash_state) const;

		[[nodiscard]] static Vector2 GenerateSubPixelJitter(uint32_t& hash_state);

		[[nodiscard]] uint32_t
		GetPixelIndex(uint32_t screen_x, uint32_t screen_y) const;
	};
} //namespace gfx
#endif //SOFTWARE_PATH_TRACER_HEADER
