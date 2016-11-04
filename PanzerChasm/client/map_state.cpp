#include "../math_utils.hpp"

#include "map_state.hpp"

namespace PanzerChasm
{

static const float g_walls_coord_scale= 256.0f;

static const float g_animations_frames_per_second= 20.0f;

MapState::MapState(
	const MapDataConstPtr& map,
	const GameResourcesConstPtr& game_resources,
	const Time map_start_time )
	: map_data_(map)
	, game_resources_(game_resources)
	, map_start_time_(map_start_time)
{
	PC_ASSERT( map_data_ != nullptr );

	dynamic_walls_.resize( map_data_->dynamic_walls.size() );

	for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
	{
		const MapData::Wall& in_wall= map_data_->dynamic_walls[w];
		DynamicWall& out_wall= dynamic_walls_[w];

		out_wall.xy[0][0]= short( in_wall.vert_pos[0].x * g_walls_coord_scale );
		out_wall.xy[0][1]= short( in_wall.vert_pos[0].y * g_walls_coord_scale );
		out_wall.xy[1][0]= short( in_wall.vert_pos[1].x * g_walls_coord_scale );
		out_wall.xy[1][1]= short( in_wall.vert_pos[1].y * g_walls_coord_scale );
		out_wall.z= 0;
	}

	static_models_.resize( map_data_->static_models.size() );
	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		const MapData::StaticModel& in_model= map_data_->static_models[m];
		StaticModel& out_model= static_models_[m];

		out_model.pos= m_Vec3( in_model.pos, 0.0f );
		out_model.angle= in_model.angle;
		out_model.model_id= in_model.model_id;
		out_model.animation_frame= 0u;
	}

	items_.resize( map_data_->items.size() );
	for( unsigned int i= 0u; i < items_.size(); i++ )
	{
		const MapData::Item& in_item= map_data_->items[i];
		Item& out_item= items_[i];

		out_item.pos= m_Vec3( in_item.pos, 0.0f );
		out_item.angle= in_item.angle;
		out_item.item_id= in_item.item_id;
		out_item.picked_up= false;
		out_item.animation_frame= 0;
	}
}

MapState::~MapState()
{}

const MapState::DynamicWalls& MapState::GetDynamicWalls() const
{
	return dynamic_walls_;
}

const MapState::StaticModels& MapState::GetStaticModels() const
{
	return static_models_;
}

const MapState::Items& MapState::GetItems() const
{
	return items_;
}

void MapState::Tick( Time current_time )
{
	const float time_since_map_start_s= ( current_time - map_start_time_ ).ToSeconds();
	for( Item& item : items_ )
	{
		const unsigned int animation_frame=
			static_cast<unsigned int>( std::round( g_animations_frames_per_second * time_since_map_start_s ) );

		if( item.item_id < game_resources_->items_models.size() )
			item.animation_frame= animation_frame % game_resources_->items_models[ item.item_id ].frame_count;
		else
			item.animation_frame= 0;
	}
}

void MapState::ProcessMessage( const Messages::EntityState& message )
{
	// TODO
	PC_UNUSED(message);
}

void MapState::ProcessMessage( const Messages::WallPosition& message )
{
	if( message.wall_index >= dynamic_walls_.size() )
		return; // Bad wall index.

	DynamicWall& wall= dynamic_walls_[ message.wall_index ];

	wall.xy[0][0]= message.vertices_xy[0][0];
	wall.xy[0][1]= message.vertices_xy[0][1];
	wall.xy[1][0]= message.vertices_xy[1][0];
	wall.xy[1][1]= message.vertices_xy[1][1];
	wall.z= message.z;
}

void MapState::ProcessMessage( const Messages::StaticModelState& message )
{
	if( message.static_model_index >= static_models_.size() )
		return;

	StaticModel& static_model= static_models_[ message.static_model_index ];

	static_model.angle= float(message.angle) / 65536.0f * Constants::two_pi;

	static_model.pos.x= float(message.xyz[0]) / 256.0f;
	static_model.pos.y= float(message.xyz[1]) / 256.0f;
	static_model.pos.z= float(message.xyz[2]) / 256.0f;

	static_model.animation_frame= message.animation_frame;
}

void MapState::ProcessMessage( const Messages::EntityBirth& message )
{
	// TODO
	PC_UNUSED(message);
}

void MapState::ProcessMessage( const Messages::EntityDeath& message )
{
	// TODO
	PC_UNUSED(message);
}

} // namespace PanzerChasm
