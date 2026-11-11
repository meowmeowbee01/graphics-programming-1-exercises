#ifndef SAMPLERS_HEADER
#define SAMPLERS_HEADER

namespace gfx
{
	inline uint32_t LowBias32(uint32_t h)
	{
		h ^= h >> 16u;
		h *= 0x7feb352du;
		h ^= h >> 15u;
		h *= 0x846ca68bu;
		h ^= h >> 16u;
		return h;
	}

	inline float HashToFloat(const uint32_t h)
	{ return static_cast<float>(h >> 8u) * (1.f / 16777216.f); }

	[[maybe_unused]] static float NextFloat(uint32_t& h)
	{
		h = LowBias32(h);
		return HashToFloat(h);
	}

	inline Vector3
	LocalToWorld(const Vector3& local_direction, const Vector3& normal)
	{
		const auto helper {
		  std::abs(normal.x) > .9f ? Vector3::UnitY() : Vector3::UnitX()
		};
		const auto t {Vector3::Cross(normal, helper).Normalized()};
		const auto b {Vector3::Cross(normal, t)};
		return {
		  t * local_direction.x + b * local_direction.y + normal * local_direction.z
		};
	}

	struct HemiSphereSample
	{
		Vector3 direction {};
		float pdf {};
	};

	static float PdfUniform(const Vector3& normal, const Vector3& direction)
	{
		return Vector3::Dot(normal, direction) > 0.f
		  ? 1.f / (2.f * static_cast<float>(std::numbers::pi))
		  : 0.f;
	}

	static HemiSphereSample
	SampleUniform(const Vector3& normal, uint32_t& hash_state)
	{
		const auto u1 {NextFloat(hash_state)};
		const auto u2 {NextFloat(hash_state)};
		const auto cos_theta {u2};
		const auto sin_theta {std::sqrtf(1.f - u2 * u2)};
		const auto phi {u1 * std::numbers::pi_v<float> * 2.f};
		const auto cos_phi {std::cosf(phi)};
		const auto sin_phi {std::sinf(phi)};
		const Vector3 local {sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};
		const auto dir {LocalToWorld(local, normal)};
		return {.direction = dir, .pdf = PdfUniform(normal, dir)};
	}

	static float PdfCosine(const Vector3& normal, const Vector3& direction)
	{
		return std::max(Vector3::Dot(normal, direction), 0.f) /
		  static_cast<float>(std::numbers::pi);
	}

	static HemiSphereSample
	SampleCosine(const Vector3& normal, uint32_t& hash_state)
	{
		const auto u1 {NextFloat(hash_state)};
		const auto u2 {NextFloat(hash_state)};
		const auto cos_theta {std::sqrtf(1.f - u2)};
		const auto sin_theta {std::sqrtf(u2)};
		const auto phi {u1 * std::numbers::pi_v<float> * 2.f};
		const auto cos_phi {std::cosf(phi)};
		const auto sin_phi {std::sinf(phi)};
		const Vector3 local {sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};
		const auto dir = LocalToWorld(local, normal);
		return {.direction = dir, .pdf = PdfCosine(normal, dir)};
	}

	static float PdfGgx(
	  const Vector3& normal,
	  const Vector3& view_dir,
	  const Vector3& light_dir,
	  const float roughness
	)
	{
		const Vector3 h {(view_dir + light_dir).Normalized()};
		const float n_dot_h {std::max(Vector3::Dot(normal, h), 0.0001f)};
		const float v_dot_h {std::max(Vector3::Dot(view_dir, h), 0.0001f)};
		return NormalDistributionGgx(normal, h, roughness) *
		  n_dot_h /
		  (4.f * v_dot_h);
	}

	static HemiSphereSample SampleGgx(
	  const Vector3& normal,
	  const Vector3& view,
	  const float roughness,
	  uint32_t& hash_state
	)
	{
		const auto a {roughness * roughness};
		const auto a_squared {a * a};
		const auto u1 {NextFloat(hash_state)};
		const auto u2 {NextFloat(hash_state)};
		const auto cos_theta_h {
		  std::sqrtf((1.f - u1) / (1.f + (a_squared - 1.f) * u1))
		};
		const auto sin_theta_h {std::sqrtf(1.f - cos_theta_h * cos_theta_h)};
		const auto phi {u2 * std::numbers::pi_v<float> * 2.f};
		const auto cos_phi {std::cosf(phi)};
		const auto sin_phi {std::sinf(phi)};
		const Vector3 local_h {
		  sin_theta_h * cos_phi,
		  sin_theta_h * sin_phi,
		  cos_theta_h
		};
		const auto h {LocalToWorld(local_h, normal)};
		const auto dir {Vector3::Reflect(-view, h)};
		return {.direction = dir, .pdf = PdfGgx(normal, view, dir, roughness)};
	}

	static float SpecularProbability(
	  const ShadingInput& input,
	  const SurfaceProperties& surface
	)
	{
		const auto fresnel {FresnelFunctionSchlick(
		                      input.world_normal,
		                      input.view_direction,
		                      surface.f0
		)
		                      .r};
		const auto probability {std::clamp(fresnel, .05f, .95f)};
		return std::lerp(probability, 1.f, surface.metallic);
	}

	static float PdfSpecularOrDiffuse(
	  const ShadingInput& input,
	  const SurfaceProperties& surface,
	  const Vector3& direction
	)
	{
		if (Vector3::Dot(input.world_normal, direction) <= 0.f)
			return 0.f; //below the horizon
		const float p {SpecularProbability(input, surface)};
		return std::lerp(
		  PdfCosine(input.world_normal, direction),
		  PdfGgx(
		    input.world_normal,
		    input.view_direction,
		    direction,
		    surface.roughness
		  ),
		  p
		);
	}

	static HemiSphereSample SampleSpecularOrDiffuse(
	  const ShadingInput& input,
	  const SurfaceProperties& surface,
	  uint32_t& hash_state
	)
	{
		const auto u {NextFloat(hash_state)};
		const auto p {SpecularProbability(input, surface)};
		const auto dir {
		  u < p ? SampleGgx(
		            input.world_normal,
		            input.view_direction,
		            surface.roughness,
		            hash_state
		          )
		            .direction
		        : SampleCosine(surface.shading_normal, hash_state).direction
		};
		return {.direction = dir, .pdf = PdfSpecularOrDiffuse(input, surface, dir)};
	}
} //namespace gfx
#endif //SAMPLERS_HEADER
