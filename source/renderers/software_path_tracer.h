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
		explicit SoftwarePathTracer(Context* context);
		~SoftwarePathTracer() override;

		SoftwarePathTracer(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer& operator=(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer(SoftwarePathTracer&&) = delete;
		SoftwarePathTracer& operator=(SoftwarePathTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;

	private:
		void RenderPixel(uint32_t screen_x, uint32_t screen_y) const;

		[[nodiscard]] Ray GetRay(uint32_t screen_x, uint32_t screen_y) const;

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

		[[nodiscard]] ColorRgba GetColor(
		  const RayHitRecord& hit_record,
		  const ShadingInput& shading_input,
		  uint32_t pixel_index
		) const;

		void
		WriteColor(uint32_t screen_x, uint32_t screen_y, ColorRgba color) const;

		const uint32_t ao_sample_count_ {32};
		const float ao_radius_ {10.f};

		[[nodiscard]] float
		TraceAo(const ShadingInput& shading_input, uint32_t pixel_index) const;

		struct HemiSphereSample
		{
			Vector3 direction {};
			float pdf {};
		};

		[[nodiscard]] HemiSphereSample
		GetHemiSphereSample(const Vector3& normal, uint32_t seed) const;

		static HemiSphereSample
		UniformHemiSphereSample(const Vector3& normal, uint32_t seed);

		static HemiSphereSample
		CosineHemiSphereSample(const Vector3& normal, uint32_t seed);
	};
} //namespace gfx
#endif //SOFTWARE_PATH_TRACER_HEADER
