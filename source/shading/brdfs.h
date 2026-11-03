//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef BRDFS_HEADER
#define BRDFS_HEADER

//--- Standard Includes ---
#include <cassert>

//--- Framework Includes ---
#include <color.h>
#include <vector2.h>

namespace gfx
{
	//--- BRDFs ---
	/**
	 * \param kd Diffuse Reflection Coefficient
	 * \param cd Diffuse Color
	 */
	[[maybe_unused]] static ColorRgba Lambert(const float kd, const ColorRgba& cd)
	{ return cd * kd / std::numbers::pi_v<float>; }

	[[maybe_unused]] static ColorRgba
	Lambert(const ColorRgba& kd, const ColorRgba& cd)
	{ return cd * kd / std::numbers::pi_v<float>; }

	/**
	* \brief BRDF Fresnel Function >> Schlick
	* \param h Normalized Half vector between View and Light directions
	* \param v Normalized View direction
	* \param f0 Base reflectivity of a surface based on IOR (Indices Of
	Refraction), this is different for Dielectrics (Non-Metal) and Conductors
	(Metal)
	*/
	[[maybe_unused]] static ColorRgba FresnelFunctionSchlick(
	  const Vector3& h,
	  const Vector3& v,
	  const ColorRgba& f0
	)
	{
		const auto hv {std::max(Vector3::Dot(h, v), 0.f)};
		const auto penteracted {std::powf(1.f - hv, 5.f)};
		return f0 + (ColorRgba {1.f} - f0) * penteracted;
	}

	/**
	 * \brief BRDF Normal Distribution >> Trowbridge-Reitz GGX (UE4 implementation
	 * - squared(roughness))
	 * \param n Surface normal
	 * \param h Normalized half vector
	 * \param roughness Roughness of the material
	 */
	[[maybe_unused]] static float NormalDistributionGgx(
	  const Vector3& n,
	  const Vector3& h,
	  const float roughness
	)
	{
		const auto alpha_squared {std::powf(roughness, 4.0f)};
		const auto nh_squared {std::powf(std::max(Vector3::Dot(n, h), 0.f), 2.f)};
		const auto intermediate {nh_squared * (alpha_squared - 1.f) + 1.f};
		const auto denominator {
		  std::numbers::pi_v<float> * std::powf(intermediate, 2.f)
		};
		return alpha_squared / denominator;
	}

	/**
	 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4
	 * implementation - squared(roughness))
	 * \param n Normal of the surface
	 * \param v Normalized direction (view or light)
	 * \param roughness Roughness of the material
	 * \param is_indirect If true, should use indirect k remapping
	 */
	[[maybe_unused]] static float GeometryFunctionSchlickGgx(
	  const Vector3& n,
	  const Vector3& v,
	  const float roughness,
	  [[maybe_unused]] const bool is_indirect = false
	)
	{
		const auto k {
		  is_indirect ? powf(roughness, 2.f) / 2.f
		              : powf(roughness + 1.f, 2.f) / 8.f
		};
		const auto nv {std::max(Vector3::Dot(n, v), 0.f)};
		const auto denominator {nv * (1.f - k) + k};
		return nv / denominator;
	}

	/**
	 * \brief BRDF Geometry Function >> Smith (separable form)
	 * \param n Normal of the surface
	 * \param v Normalized view direction
	 * \param l Normalized light direction
	 * \param roughness Roughness of the material
	 * \param is_indirect (see GeometryFunction_SchlickGGX).
	 */
	[[maybe_unused]] static float GeometryFunctionSmith(
	  const Vector3& n,
	  const Vector3& v,
	  const Vector3& l,
	  const float roughness,
	  [[maybe_unused]] const bool is_indirect = false
	)
	{
		return GeometryFunctionSchlickGgx(n, v, roughness, is_indirect) *
		  GeometryFunctionSchlickGgx(n, l, roughness, is_indirect);
	}
} //namespace gfx
#endif
