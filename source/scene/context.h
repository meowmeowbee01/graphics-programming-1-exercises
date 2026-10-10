//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef CONTEXT_HEADER
#define CONTEXT_HEADER

//--- Standard Includes ---
#include <memory>

//--- External Includes---
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

//--- Framework Includes ---
#include <debug_views.h>
#include <leak_detector.h>
#include <logger.h>
#include <scene_manager.h>
#include <timer.h>

namespace gfx
{
	//--- Additional Helpers ---
	struct SurfaceInfo final
	{
		const SDL_PixelFormatDetails* pixel_format_details {nullptr};
		uint32_t* pixel_buffer {nullptr};
		void* window_os_handle {nullptr};
		void* display_os_handle {nullptr};
		uint32_t width {0};
		uint32_t height {0};
	};

	//--- Context ---
	struct Context final
	{
		SurfaceInfo surface_info {};
		std::unique_ptr<Logger> logger {nullptr};
		std::unique_ptr<LeakDetector> leak_detector {nullptr};
		std::unique_ptr<Timer> timer {nullptr};
		std::unique_ptr<SceneManager> scene_manager {nullptr};
		DebugParams debug_params {};
	};
} //namespace gfx
#endif //CONTEXT_HEADER
