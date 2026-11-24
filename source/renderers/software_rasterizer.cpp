//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <scenes.h>
#include <software_rasterizer.h>

#include <numeric>
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
	depth_buffer = std::vector(
	  static_cast<size_t>(context_->surface_info.width) *
	    static_cast<size_t>(context_->surface_info.height),
	  kClearDepth
	);

	auto& scene {*context_->scene_manager->GetActiveScene()};
	const auto aspect_ratio {
	  static_cast<float>(context_->surface_info.width) /
	  static_cast<float>(context_->surface_info.height)
	};
	const auto fov_angle {
	  scene.camera.GetFovAngle() * std::numbers::pi_v<float> / 180.f
	};
	const auto tan_half_fov {std::tanf(fov_angle * 0.5f)};
	const auto view {scene.camera.GetView().GetInverse()};
	const auto projection {
	  scene.camera.GetProjection(tan_half_fov, aspect_ratio)
	};

	for (const auto& object : scene.objects)
	{
		const auto world {
		  object.instance_transformation.has_value()
		    ? object.instance_transformation.value()
		    : Matrix::CreateIdentity()
		};
		const auto mesh {
		  *scene.primitives_factory.GetAs<TriangleMesh>(object.primitive_index)
		};
		auto mesh_clone {mesh.CloneAs<TriangleMesh>()};
		TransformMesh(*mesh_clone, world, view, projection);
		for (size_t i {0}; i < mesh_clone->indices.size(); i += 3)
		{
			auto v0 {mesh_clone->vertices.at(mesh_clone->indices.at(i))};
			auto v1 {mesh_clone->vertices.at(mesh_clone->indices.at(i + 1))};
			auto v2 {mesh_clone->vertices.at(mesh_clone->indices.at(i + 2))};
			if (
			  NotInFrustum(v0.position) ||
			  NotInFrustum(v1.position) ||
			  NotInFrustum(v2.position)
			)
				continue;
			NdcToScreen(v0.position);
			NdcToScreen(v1.position);
			NdcToScreen(v2.position);
			RasterizeTriangle(v0, v1, v2, mesh_clone->cull_mode);
		}
	}
	if (context_->debug_params.visualization_mode == VisualizationMode::kDepth)
		for (size_t i {0}; i < depth_buffer.size(); ++i)
			WriteColorToScreen(
			  i,
			  ColorRgba {std::clamp(depth_buffer.at(i) * 50.f, 0.f, 1.f)}
			);
}

void SoftwareRasterizer::RasterizeTriangle(
  const Vertex& v0,
  const Vertex& v1,
  const Vertex& v2,
  const CullMode cull_mode
)
{
	const auto& surface_info = context_->surface_info;
	const auto min_x {std::min(
	  {v0.position.x,
	   v1.position.x,
	   v2.position.x,
	   static_cast<float>(surface_info.width - 1)}
	)};
	const auto max_x {
	  std::max({v0.position.x, v1.position.x, v2.position.x, 0.f})
	};
	const auto min_y {std::min(
	  {v0.position.y,
	   v1.position.y,
	   v2.position.y,
	   static_cast<float>(surface_info.height - 1)}
	)};
	const auto max_y {
	  std::max({v0.position.y, v1.position.y, v2.position.y, 0.f})
	};
	for (
	  uint32_t screen_y {static_cast<uint32_t>(std::floor(min_y))};
	  screen_y < static_cast<uint32_t>(std::ceil(max_y));
	  ++screen_y
	)
		for (
		  uint32_t screen_x {static_cast<uint32_t>(std::floor(min_x))};
		  screen_x < static_cast<uint32_t>(std::ceil(max_x));
		  ++screen_x
		)
		{
			if (context_->debug_params.show_bounding_boxes)
			{
				WriteColorToScreen(screen_x, screen_y, ColorRgba {v0.position});
				continue;
			}
			Vector2 position {
			  static_cast<float>(screen_x) + .5f,
			  static_cast<float>(screen_y) + .5f
			};
			RasterHitRecord hit {};
			if (!PointInTriangle(v0, v1, v2, position, cull_mode, hit)) continue;
			assert(
			  hit.barycentric_coordinates.has_value() && "no barycentric coordinates"
			);
			const auto depth {InterpolateAttribute(
			  std::array {v0.position.z, v1.position.z, v2.position.z},
			  hit.barycentric_coordinates.value()
			)};
			if (depth < depth_buffer.at(GetPixelIndex(screen_x, screen_y))) continue;
			depth_buffer.at(GetPixelIndex(screen_x, screen_y)) = depth;
			auto color {ColorRgba::White()};
			if (v0.color.has_value() && v1.color.has_value() && v2.color.has_value())
				color = ColorRgba {InterpolateAttribute(
				  std::array {v0.color.value(), v1.color.value(), v2.color.value()},
				  hit.barycentric_coordinates.value()
				)};
			WriteColorToScreen(screen_x, screen_y, color);
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
	if (std::abs(signed_area) < 1e-4f) return false;

	const Vector2 p0 {v0.position};
	const Vector2 p1 {v1.position};
	const Vector2 p2 {v2.position};
	const auto c0 {Vector2::Cross(point - p0, p0 - p1)};
	const auto c1 {Vector2::Cross(point - p1, p1 - p2)};
	const auto c2 {Vector2::Cross(point - p2, p2 - p0)};
	hit_record.barycentric_coordinates = {c2 / signed_area, c0 / signed_area};
	return (signed_area > 0.f && c0 > 0.f && c1 > 0.f && c2 > 0.f) ||
	  (signed_area < 0.f && c0 < 0.f && c1 < 0.f && c2 < 0.f);
}

void SoftwareRasterizer::TransformMesh(
  TriangleMesh& mesh,
  const Matrix& world_matrix,
  const Matrix& view_matrix,
  const Matrix& projection_matrix
)
{
	const auto wvp {world_matrix * view_matrix * projection_matrix};
	for (auto& vertex : mesh.vertices)
	{
		vertex.position = wvp.TransformPoint(vertex.position);
		vertex.position.x /= vertex.position.w;
		vertex.position.y /= vertex.position.w;
		vertex.position.z /= vertex.position.w;
	}
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

template<typename T>
T SoftwareRasterizer::InterpolateAttribute(
  const std::array<T, 3>& attributes,
  const Vector2& barycentric_coordinates
)
{
	const auto w1 {barycentric_coordinates.x};
	const auto w2 {barycentric_coordinates.y};
	const auto w0 {1.f - w1 - w2};
	return attributes.at(0) * w0 + attributes.at(1) * w1 + attributes.at(2) * w2;
}
