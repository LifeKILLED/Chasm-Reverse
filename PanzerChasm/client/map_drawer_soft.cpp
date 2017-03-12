#include "../assert.hpp"
#include "../map_loader.hpp"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: game_resources_( game_resources )
	, rendering_context_( rendering_context )
	, rasterizer_(
		rendering_context.viewport_size.Width(), rendering_context.viewport_size.Height(),
		rendering_context.row_pixels, rendering_context.window_surface_data )
{
	PC_ASSERT( game_resources_ != nullptr );

	// TODO
	PC_UNUSED( settings );
}

MapDrawerSoft::~MapDrawerSoft()
{}

void MapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	current_map_data_= map_data;
}

void MapDrawerSoft::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const EntityId player_monster_id )
{
	PC_UNUSED( player_monster_id );

	if( current_map_data_ == nullptr )
		return;

	const float viewport_size_x= float(rendering_context_.viewport_size.Width ());
	const float viewport_size_y= float(rendering_context_.viewport_size.Height());
	const float screen_transform_x= viewport_size_x * 0.5f;
	const float screen_transform_y= viewport_size_y * 0.5f;

	m_Mat4 cam_shift_mat, cam_mat, screen_flip_mat;
	cam_shift_mat.Translate( -camera_position );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	cam_mat= cam_shift_mat * view_rotation_and_projection_matrix * screen_flip_mat;

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= current_map_data_->models_description.size() )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( static_model.angle );
		translate_mat.Translate( static_model.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		const Model& model= current_map_data_->models[ static_model.model_id ];
		const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * static_model.animation_frame;

		for( unsigned int t= 0u; t < model.regular_triangles_indeces.size(); t+= 3u )
		{
			RasterizerVertexSimple vertices_fixed[3];
			bool clipped= false;
			for( unsigned int tv= 0u; tv < 3u; tv++ )
			{
				const Model::Vertex& vertex= model.vertices[ model.regular_triangles_indeces[t + tv] ];
				const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];


				const m_Vec3 vertex_pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;
				m_Vec3 vertex_projected= vertex_pos * view_mat;
				const float w= vertex_pos.x * view_mat.value[3] + vertex_pos.y * view_mat.value[7] + vertex_pos.z * view_mat.value[11] + view_mat.value[15];
				if( w <= 0.25f )
				{
					clipped= true;
					break;
				}

				vertex_projected/= w;
				vertex_projected.z= w;

				vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x;
				vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y;

				if( vertex_projected.x < 0.0f || vertex_projected.x > viewport_size_x ||
					vertex_projected.y < 0.0f || vertex_projected.y > viewport_size_y )
				{
					clipped= true;
					break;
				}

				vertices_fixed[tv].x= fixed16_t( vertex_projected.x * 65536.0f );
				vertices_fixed[tv].y= fixed16_t( vertex_projected.y * 65536.0f );
			}
			if( clipped ) continue;

			const uint32_t color= 0xFF00FF00u;
			rasterizer_.DrawAffineColoredTriangle( vertices_fixed, color );
		}
	}
}

void MapDrawerSoft::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle )
{
	// TODO
	PC_UNUSED( weapon_state );
	PC_UNUSED( projection_matrix );
	PC_UNUSED( camera_position );
	PC_UNUSED( x_angle );
	PC_UNUSED( z_angle );
}

} // PanzerChasm
