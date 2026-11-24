//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <scenes.h>
#include <software_rasterizer.h>
using namespace gfx;

//=============================================================================
//Construction / Destruction
//=============================================================================
SoftwareRasterizer::SoftwareRasterizer(Context* const context)
  : Renderer(context)
{
}

SoftwareRasterizer::~SoftwareRasterizer() = default;

//=============================================================================
//Public Functions
//=============================================================================
void SoftwareRasterizer::Render()
{
	assert(context_ && "Context not available!");
	std::fill_n(
	  context_->surface_info.pixel_buffer,
	  context_->surface_info.width * context_->surface_info.height,
	  SDL_MapRGB(
	    context_->surface_info.pixel_format_details,
	    nullptr,
	    static_cast<uint8_t>(.19f * 255.f),
	    static_cast<uint8_t>(.21f * 255.f),
	    static_cast<uint8_t>(.26f * 255.f)
	  )
	);

	for (
	  const auto& scene {*context_->scene_manager->GetActiveScene()};
	  const auto& object : scene.objects
	)
	{
		const auto mesh {
		  *scene.primitives_factory.GetAs<TriangleMesh>(object.primitive_index)
		};
		const auto mesh_clone {mesh.CloneAs<TriangleMesh>()};
		for (size_t i {0}; i < mesh_clone->indices.size(); i += 3)
		{
			auto v0 {mesh_clone->vertices.at(mesh_clone->indices.at(i))};
			auto v1 {mesh_clone->vertices.at(mesh_clone->indices.at(i + 1))};
			auto v2 {mesh_clone->vertices.at(mesh_clone->indices.at(i + 2))};
			NdcToScreen(v0.position);
			NdcToScreen(v1.position);
			NdcToScreen(v2.position);
			RasterizeTriangle(v0, v1, v2, mesh_clone->cull_mode);
		}
	}
}

void SoftwareRasterizer::RasterizeTriangle(
  const Vertex& v0,
  const Vertex& v1,
  const Vertex& v2,
  //uint32_t object_id,
  const CullMode cull_mode
) const
{
	const auto& surface_info = context_->surface_info;
	for (uint32_t screen_y {0}; screen_y < surface_info.height; ++screen_y)
		for (uint32_t screen_x {0}; screen_x < surface_info.width; ++screen_x)
		{
			Vector2 position {
			  static_cast<float>(screen_x) + .5f,
			  static_cast<float>(screen_y) + .5f
			};
			if (
			  RasterHitRecord hit {};
			  PointInTriangle(v0, v1, v2, position, cull_mode, hit)
			)
				WriteColorToScreen(screen_x, screen_y, ColorRgba::White());
		}
}

bool SoftwareRasterizer::PointInTriangle(
  const Vertex& v0,
  const Vertex& v1,
  const Vertex& v2,
  const Vector2& point,
  const CullMode cull_mode,
  RasterHitRecord& hit_record
)
{
	const auto signed_area {
	  Vector2::Cross(v1.position - v0.position, v2.position - v0.position)
	};
	if (cull_mode == CullMode::kBackFaceCulling && signed_area <= 0.f)
		return false;
	if (cull_mode == CullMode::kFrontFaceCulling && signed_area >= 0.f)
		return false;
	if (std::abs(signed_area) < 1e4f) return false;

	const Vector2 p0 {v0.position};
	const Vector2 p1 {v1.position};
	const Vector2 p2 {v2.position};
	const auto c0 {Vector2::Cross(point - p0, p0 - p1)};
	const auto c1 {Vector2::Cross(point - p1, p1 - p2)};
	const auto c2 {Vector2::Cross(point - p2, p2 - p0)};
	hit_record.barycentric_coordinates = {c0, c1};
	return (signed_area > 0.f && c0 > 0.f && c1 > 0.f && c2 > 0.f) ||
	  (signed_area < 0.f && c0 < 0.f && c1 < 0.f && c2 < 0.f);
}

void SoftwareRasterizer::WriteColorToScreen(
  const uint32_t pixel_index,
  const ColorRgba& color
) const
{
	context_->surface_info.pixel_buffer[pixel_index] = SDL_MapRGB(
	  context_->surface_info.pixel_format_details,
	  nullptr,
	  static_cast<uint8_t>(color.r * 255.f),
	  static_cast<uint8_t>(color.g * 255.f),
	  static_cast<uint8_t>(color.b * 255.f)
	);
}
