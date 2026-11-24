//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SOFTWARE_RASTERIZER_HEADER
#define SOFTWARE_RASTERIZER_HEADER

//--- Standard Includes ---

//--- Framework Includes ---
#include <renderer.h>

namespace gfx
{
	//--- Software Rasterizer ---
	class SoftwareRasterizer final : public Renderer
	{
	public:
		//--- Construction / Destruction ---
		explicit SoftwareRasterizer(Context* context);
		~SoftwareRasterizer() override;

		SoftwareRasterizer(const SoftwareRasterizer&) = delete;
		SoftwareRasterizer& operator=(const SoftwareRasterizer&) = delete;
		SoftwareRasterizer(SoftwareRasterizer&&) = delete;
		SoftwareRasterizer& operator=(SoftwareRasterizer&&) = delete;

		//--- Public Functions ---
		void Render() override;

	private:
		[[nodiscard]] uint32_t
		GetPixelIndex(const uint32_t screen_x, const uint32_t screen_y) const
		{ return screen_x + context_->surface_info.width * screen_y; }

		void NdcToScreen(Vector4& ndc) const
		{
			ndc.x =
			  (ndc.x + 1.f) / 2.f * static_cast<float>(context_->surface_info.width);
			ndc.y =
			  (1.f - ndc.y) / 2.f * static_cast<float>(context_->surface_info.height);
		}

		void RasterizeTriangle(
		  const Vertex& v0,
		  const Vertex& v1,
		  const Vertex& v2,
		  //uint32_t object_id,
		  CullMode cull_mode
		) const;

		static bool PointInTriangle(
		  const Vertex& v0,
		  const Vertex& v1,
		  const Vertex& v2,
		  const Vector2& point,
		  CullMode cull_mode,
		  RasterHitRecord& hit_record
		);

		void WriteColorToScreen(uint32_t pixel_index, const ColorRgba& color) const;

		void WriteColorToScreen(
		  const uint32_t screen_x,
		  const uint32_t screen_y,
		  const ColorRgba& color
		) const
		{ WriteColorToScreen(GetPixelIndex(screen_x, screen_y), color); }
	};
} //namespace gfx
#endif //SOFTWARE_RASTERIZER_HEADER
