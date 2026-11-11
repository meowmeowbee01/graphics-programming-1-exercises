#ifndef SAMPLERS_HEADER
#define SAMPLERS_HEADER

//--- Framework Includes ---
#include <primitives.h>

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

	[[maybe_unused]] static HemiSphereSample
	UniformHemiSphereSample(const Vector3& normal, const uint32_t seed)
	{
		auto h {LowBias32(seed)};
		const auto u1 {HashToFloat(h)};
		h = LowBias32(h);
		const auto u2 {HashToFloat(h)};
		const auto cos_theta {u2};
		const auto sin_theta {std::sqrtf(1.f - u2 * u2)};
		const auto phi {u1 * std::numbers::pi_v<float> * 2.f};
		const auto cos_phi {std::cosf(phi)};
		const auto sin_phi {std::sinf(phi)};
		const Vector3 local {sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};
		constexpr auto pdf {1.f / (2.f * std::numbers::pi_v<float>)};
		return {.direction = LocalToWorld(local, normal), .pdf = pdf};
	}

	[[maybe_unused]] static HemiSphereSample
	CosineHemiSphereSample(const Vector3& normal, const uint32_t seed)
	{
		auto h {LowBias32(seed)};
		const auto u1 {HashToFloat(h)};
		h = LowBias32(h);
		const auto u2 {HashToFloat(h)};
		const auto cos_theta {std::sqrtf(1.f - u2)};
		const auto sin_theta {std::sqrtf(u2)};
		const auto phi {u1 * std::numbers::pi_v<float> * 2.f};
		const auto cos_phi {std::cosf(phi)};
		const auto sin_phi {std::sinf(phi)};
		const Vector3 local {sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};
		const auto pdf {cos_theta / std::numbers::pi_v<float>};
		return {.direction = LocalToWorld(local, normal), .pdf = pdf};
	}
} //namespace gfx
#endif //SAMPLERS_HEADER
