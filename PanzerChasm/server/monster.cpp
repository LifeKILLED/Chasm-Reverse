#include "../game_constants.hpp"
#include "math_utils.hpp"

#include "map.hpp"
#include "player.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

static const m_Vec3 g_see_point_delta( 0.0f, 0.0f, 0.5f );

// TODO - use different attack points for differnet mosnters.
// In original game this points are hardcoded.
static const m_Vec3 g_shoot_point_delta( 0.0f, 0.0f, 0.5f );

Monster::Monster(
	const MapData::Monster& map_monster,
	const float z,
	const GameResourcesConstPtr& game_resources,
	const LongRandPtr& random_generator,
	const Time spawn_time )
	: MonsterBase(
		game_resources,
		map_monster.monster_id,
		m_Vec3( map_monster.pos, z ),
		map_monster.angle )
	, random_generator_(random_generator)
	, current_animation_start_time_(spawn_time)
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( random_generator_ != nullptr );
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );

	current_animation_= GetAnyAnimation( { AnimationId::Idle0, AnimationId::Idle1, AnimationId::Run } );

	health_= game_resources_->monsters_description[ monster_id_ ].life;
}

Monster::~Monster()
{}

void Monster::Tick( Map& map, const Time current_time, const Time last_tick_delta )
{
	const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster_id_ ];
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	PC_ASSERT( current_animation_ < model.animations.size() );
	PC_ASSERT( model.animations[ current_animation_ ].frame_count > 0u );

	const float time_delta_s= ( current_time - current_animation_start_time_ ).ToSeconds();
	const float frame= time_delta_s * GameConstants::animations_frames_per_second;

	const unsigned int animation_frame_unwrapped= static_cast<unsigned int>( std::round(frame) );
	const unsigned int frame_count= model.animations[ current_animation_ ].frame_count;

	// Update target position if target moves.
	const PlayerConstPtr target= target_.lock();
	if( target != nullptr )
		target_position_= target->Position();

	switch( state_ )
	{
	case State::Idle:
		current_animation_frame_= animation_frame_unwrapped % frame_count;
		break;

	case State::MoveToTarget:
	{
		if( ( pos_.xy() - target_position_.xy() ).SquareLength() <= description.attack_radius * description.attack_radius )
		{
			state_= State::MeleeAttack;
			current_animation_= GetAnyAnimation( { AnimationId::MeleeAttackLeftHand, AnimationId::MeleeAttackRightHand, AnimationId::MeleeAttackHead } );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
		{	if( current_time >= target_change_time_ )
			{
				if( description.rock >= 0 && target != nullptr &&
					map.CanSee( pos_ + g_see_point_delta, target->Position() + g_see_point_delta ) )
				{
					state_= State::RemoteAttack;
					current_animation_= GetAnimation( AnimationId::RemoteAttack );
					current_animation_start_time_= current_time;
					current_animation_frame_= 0u;
					attack_was_done_= false;
				}
				else
					SelectTarget( map, current_time );
			}

			if( state_ == State::MoveToTarget )
			{
				MoveToTarget( map, last_tick_delta.ToSeconds() );
				current_animation_frame_= animation_frame_unwrapped % frame_count;
			}
		}
	}
		break;

	case State::PainShock:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::MeleeAttack:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::RemoteAttack:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
		{
			if( animation_frame_unwrapped >= frame_count / 2u &&
				!attack_was_done_ &&
				target != nullptr )
			{
				const m_Vec3 shoot_pos= pos_ + g_shoot_point_delta;
				m_Vec3 dir= target->Position() + g_see_point_delta - shoot_pos;
				dir.Normalize();

				PC_ASSERT( description.rock >= 0 );
				map.Shoot( description.rock, shoot_pos, dir, current_time );

				attack_was_done_= true;
			}

			current_animation_frame_= animation_frame_unwrapped;
		}
		break;

	case State::DeathAnimation:
		if( animation_frame_unwrapped >= frame_count )
			state_= State::Dead;
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::Dead:
		current_animation_frame_= frame_count - 1u; // Last frame of death animation
		break;
	};
}

void Monster::Hit( const int damage, const Time current_time )
{
	if( state_ != State::DeathAnimation && state_ != State::Dead )
	{
		health_-= damage;

		if( health_ > 0 )
		{
			if( state_ != State::PainShock &&
				state_ != State::MeleeAttack )
			{
				const int animation= GetAnyAnimation( { AnimationId::Pain0, AnimationId::Pain1 } );
				if( animation >= 0 )
				{
					state_= State::PainShock;
					current_animation_= static_cast<unsigned int>(animation);
					current_animation_start_time_= current_time;
				}
				else
				{}// No pain - no gain
			}
		}
		else
		{
			state_= State::DeathAnimation;
			current_animation_start_time_= current_time;

			const int animation= GetAnyAnimation( { AnimationId::Death0, AnimationId::Death1, AnimationId::Death2, AnimationId::Death3 } );
			PC_ASSERT( animation >= 0 );
			current_animation_= static_cast<unsigned int>(animation);
		}
	}
}

void Monster::MoveToTarget( const Map& map, const float time_delta_s )
{
	const m_Vec2 vec_to_target= target_position_.xy() - pos_.xy();
	const float vec_to_target_length= vec_to_target.Length();

	// Nothing to do, we are on target
	if( vec_to_target_length == 0.0f )
		return;

	const GameResources::MonsterDescription& monster_description= game_resources_->monsters_description[ monster_id_ ];

	const float distance_delta= time_delta_s * float( monster_description.speed ) / 10.0f;

	if( distance_delta >= vec_to_target_length )
	{
		pos_.x= target_position_.x;
		pos_.y= target_position_.y;
	}
	else
	{
		pos_.x+= std::cos(angle_) * distance_delta;
		pos_.y+= std::sin(angle_) * distance_delta;
	}

	const float target_angle= NormalizeAngle( std::atan2( vec_to_target.y, vec_to_target.x ) );
	float target_angle_delta= target_angle - angle_;
	if( target_angle_delta > +Constants::pi )
		target_angle_delta-= Constants::two_pi;
	if( target_angle_delta < -Constants::pi )
		target_angle_delta+= Constants::two_pi;

	if( target_angle_delta != 0.0f )
	{
		const float abs_target_angle_delta= std::abs(target_angle_delta);
		const float angle_delta= time_delta_s * float(monster_description.rotation_speed);

		if( angle_delta >= abs_target_angle_delta )
			angle_= target_angle;
		else
		{
			const float turn_direction= target_angle_delta > 0.0f ? 1.0f : -1.0f;
			angle_= NormalizeAngle( angle_ + turn_direction * angle_delta );
		}
	}

	const float height= GameConstants::player_height; // TODO - select height
	bool on_floor;
	pos_=
		map.CollideWithMap(
			pos_, height, monster_description.w_radius,
			on_floor );
}

void Monster::SelectTarget( const Map& map, const Time current_time )
{
	/*
	const float c_half_view_angle= Constants::pi * 0.25f;
	const float c_half_view_angle_cos= std::cos( c_half_view_angle );
	const m_Vec2 view_dir( std::cos(angle_), std::sin(angle_) );
	*/

	float nearest_player_distance= Constants::max_float;
	PlayerConstPtr nearest_player;

	const Map::PlayersContainer& players= map.GetPlayers();
	for( const Map::PlayersContainer::value_type& player_value : players )
	{
		PC_ASSERT( player_value.second != nullptr );
		const Player& player= *player_value.second;

		const m_Vec2 dir_to_player= player.Position().xy() - Position().xy();
		const float distance_to_player= dir_to_player.Length();
		if( distance_to_player == 0.0f )
			continue;

		if( distance_to_player >= nearest_player_distance )
			continue;

		// TODO - add angle check, if needed
		/*
		const float angle_cos= ( dir_to_player * view_dir ) / distance_to_player;
		if( angle_cos < c_half_view_angle_cos )
			continue;
		*/

		if( map.CanSee(
				Position() + g_see_point_delta,
				player.Position() + g_see_point_delta ) )
		{
			nearest_player_distance= distance_to_player;
			nearest_player= player_value.second;
		}
	}

	if( nearest_player != nullptr )
	{
		const float target_change_interval_s= 0.8f;

		target_= nearest_player;
		target_position_= nearest_player->Position();
		target_change_time_= current_time + Time::FromSeconds(target_change_interval_s);
	}
	else
	{
		const float direction= random_generator_->RandAngle();
		const float distance= random_generator_->RandValue( 2.0f, 5.0f );
		const float target_change_interval_s= random_generator_->RandValue( 0.5f, 2.0f );

		target_= PlayerConstPtr();
		target_position_= pos_ + distance * m_Vec3( std::cos(direction), std::sin(direction), 0.0f );
		target_change_time_= current_time + Time::FromSeconds(target_change_interval_s);
	}
}

} // namespace PanzerChasm
