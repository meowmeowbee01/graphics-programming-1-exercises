//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef HARDWARE_RASTERIZER_HEADER
#define HARDWARE_RASTERIZER_HEADER

//--- Standard Includes ---

//--- External Includes ---

//--- Framework Includes ---
#include <renderer.h>

namespace gfx
{
	//--- Hardware Rasterizer ---
	class HardwareRasterizer final : public Renderer
	{
	public:
		//--- Construction / Destruction ---
		HardwareRasterizer(Context* context);
		~HardwareRasterizer() override;

		HardwareRasterizer(const HardwareRasterizer&) = delete;
		HardwareRasterizer& operator=(const HardwareRasterizer&) = delete;
		HardwareRasterizer(HardwareRasterizer&&) = delete;
		HardwareRasterizer& operator=(HardwareRasterizer&&) = delete;

		//--- Public Functions ---
		void Render() override;
	};
} //namespace gfx
#endif //HARDWARE_RASTERIZER_HEADER
