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
		[[nodiscard]] const Scene& GetScene() const
		{ return *context_->scene_manager->GetActiveScene(); }

		[[nodiscard]] const SceneObject& GetObject(const size_t index) const
		{ return GetScene().objects.at(index); }

		[[nodiscard]] const SceneObject& GetObject(const RayHitRecord& hit) const
		{ return GetObject(hit.object_index); }

		[[nodiscard]] const Primitive& GetPrimitive(const uint32_t index) const
		{ return *GetScene().primitives_factory.Get(index); }

		[[nodiscard]] const Primitive& GetPrimitive(const RayHitRecord& hit) const
		{ return GetPrimitive(GetObject(hit).primitive_index); }

		[[nodiscard]] const Material& GetMaterial(const uint32_t index) const
		{ return *GetScene().materials_factory.Get(index); }

		[[nodiscard]] const Material& GetMaterial(const SceneObject& obj) const
		{ return GetMaterial(obj.material_index); }

		[[nodiscard]] const Material& GetMaterial(const RayHitRecord& hit) const
		{ return GetMaterial(GetObject(hit)); }

		[[nodiscard]] SurfaceProperties GetSurfaceProperties(
		  const Material& material,
		  const ShadingInput& input
		) const
		{
			return material.GetSurfaceProperties(input, GetScene().textures_factory);
		}

		[[nodiscard]] SurfaceProperties GetSurfaceProperties(
		  const RayHitRecord& hit,
		  const ShadingInput& input
		) const
		{
			return GetMaterial(hit).GetSurfaceProperties(
			  input,
			  GetScene().textures_factory
			);
		}

		[[nodiscard]] bool IsAccumulatedVisMode() const
		{ return IsAccumulated(context_->debug_params.visualization_mode); }

		[[nodiscard]] bool IsBouncedVisMode() const
		{ return IsBounced(context_->debug_params.visualization_mode); }

		[[nodiscard]] bool IsToneMappedVisMode() const
		{ return IsToneMapped(context_->debug_params.visualization_mode); }

		[[nodiscard]] uint32_t
		GetPixelIndex(const uint32_t screen_x, const uint32_t screen_y) const
		{ return screen_x + context_->surface_info.width * screen_y; }

		[[nodiscard]] static Vector2 GenerateSubPixelJitter(uint32_t& hash_state)
		{ return {NextFloat(hash_state), NextFloat(hash_state)}; }

		[[nodiscard]] HemiSphereSample GetHemiSphereSample(
		  const ShadingInput& input,
		  const SurfaceProperties& surface,
		  uint32_t& hash_state
		) const
		{
			switch (context_->debug_params.sampling_strategy)
			{
			case SamplingStrategy::kUniform:
				return SampleUniform(surface.shading_normal, hash_state);
			case SamplingStrategy::kGGX:
				return SampleSpecularOrDiffuse(input, surface, hash_state);
			default: return SampleCosine(surface.shading_normal, hash_state);
			}
		}

		[[nodiscard]] static bool IsThinWalled(const SurfaceProperties& surface)
		{
			return surface.thickness <= 0.f &&
			  surface.attenuation_distance >= std::numeric_limits<float>::max();
		}

		[[nodiscard]] static ColorRgba GetHeat(const float count, const float max)
		{
			const auto t {std::clamp(count / max, 0.f, 1.f)};
			return {
			  std::clamp(2.f * t - 1.f, 0.f, 1.f),
			  t < .5f ? 2.f * t : 2.f * (1.f - t),
			  std::clamp(1.f - 2.f * t, 0.f, 1.f)
			};
		}

		const uint32_t ao_sample_count_ {1};
		const float ao_radius_ {10.f};

		VisualizationMode last_vis_mode_ {VisualizationMode::kNone};
		SamplingStrategy last_sampling_strategy_ {SamplingStrategy::kCosine};
		std::vector<ColorRgba> accumulation_buffer_ {};

		struct BounceSample
		{
			Vector3 direction {};
			ColorRgba weight {ColorRgba::Black()};
		};

		struct MediumState
		{
			bool inside {false};
			ColorRgba attenuation_color {ColorRgba::White()};
			float attenuation_distance {std::numeric_limits<float>::max()};
		};

		void RenderPixel(uint32_t screen_x, uint32_t screen_y);

		[[nodiscard]] Ray
		GetRay(uint32_t screen_x, uint32_t screen_y, uint32_t& hash_state) const;

		ColorRgba
		TracePath(const Ray& ray, uint32_t& hash_state, int max_bounces = 8) const;

		bool IsFinalBounce(
		  ColorRgba& accumulated_color,
		  ColorRgba& throughput,
		  Ray& current_ray,
		  MediumState& medium,
		  int bounces,
		  uint32_t& hash_state
		) const;

		bool HitTest(
		  const Ray& ray,
		  RayHitRecord& closest_hit_record,
		  bool ignore_hit_record = false,
		  bool reverse_culling = false
		) const;

		[[nodiscard]] ColorRgba EnvironmentRadiance(const Vector3& direction) const;

		[[nodiscard]] ShadingInput
		ConstructShadingInput(const RayHitRecord& hit_record) const;

		static void ConstructShadingInputFromMesh(
		  const RayHitRecord& hit_record,
		  ShadingInput& shading_input,
		  const TriangleMesh& mesh
		);

		static void Attenuate(
		  ColorRgba& throughput,
		  const MediumState& medium,
		  const RayHitRecord& hit_record
		);

		[[nodiscard]] ColorRgba GetLinearColor(
		  const RayHitRecord& hit_record,
		  const ShadingInput& shading_input,
		  const Material& material,
		  const SurfaceProperties& surface,
		  uint32_t& hash_state
		) const;

		[[nodiscard]] float
		TraceAo(const ShadingInput& shading_input, uint32_t& hash_state) const;

		[[nodiscard]] ColorRgba DirectIllumination(
		  const ShadingInput& shading_input,
		  const Material& material,
		  const SurfaceProperties& surface
		) const;

		static BounceSample SampleReflectOrRefract(
		  const ShadingInput& input,
		  const SurfaceProperties& surface,
		  MediumState& medium,
		  uint32_t& hash_state
		);

		std::optional<BounceSample> GetBounceSample(
		  ShadingInput& input,
		  const SurfaceProperties& surface,
		  const Material& material,
		  MediumState& medium,
		  uint32_t& hash_state
		) const;

		static ColorRgba
		CalculateRadiance(const Light& light, const Vector3& world_position);

		void WriteColor(uint32_t pixel_index, const ColorRgba& color);

		void WriteColorToScreen(uint32_t pixel_index, const ColorRgba& color) const;

		void WriteColorToBuffer(uint32_t pixel_index, const ColorRgba& color);

		void WriteColorFromBufferToScreen(uint32_t pixel_index) const;
	};
} //namespace gfx
#endif //SOFTWARE_PATH_TRACER_HEADER
