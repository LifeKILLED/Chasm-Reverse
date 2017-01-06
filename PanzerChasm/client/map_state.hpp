#pragma once
#include <unordered_map>

#include "../fwd.hpp"
#include "../messages.hpp"
#include "../rand.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

// Class for accumulating of map state, recieved from server.
class MapState final
{
public:
	struct DynamicWall
	{
		m_Vec2 vert_pos[2];
		unsigned char texture_id;
		float z; // wall bottom z
	};

	typedef std::vector<DynamicWall> DynamicWalls;

	struct StaticModel
	{
		m_Vec3 pos;
		float angle;
		unsigned int model_id;
		unsigned int animation_frame;
		bool visible;
	};

	typedef std::vector<StaticModel> StaticModels;

	struct Item
	{
		m_Vec3 pos;
		float angle;
		unsigned char item_id;
		bool picked_up;
		unsigned int animation_frame;
	};

	typedef std::vector<Item> Items;

	struct SpriteEffect
	{
		Time start_time= Time::FromSeconds(0);
		m_Vec3 pos;
		m_Vec3 speed;
		float frame;
		unsigned char effect_id;
	};

	typedef std::vector<SpriteEffect> SpriteEffects;

	struct MonsterBodyPart
	{
		m_Vec3 pos;
		float angle;
		unsigned char monster_type;
		unsigned char body_part_id;

		Time start_time= Time::FromSeconds(0);
		m_Vec3 speed;

		unsigned int animation;
		unsigned int animation_frame;
	};

	typedef std::vector<MonsterBodyPart> MonstersBodyParts;

	struct Monster
	{
		m_Vec3 pos;
		float angle;
		unsigned char monster_id;
		unsigned char body_parts_mask;
		unsigned int animation;
		unsigned int animation_frame;
	};

	typedef std::unordered_map< EntityId, Monster > MonstersContainer;

	struct Rocket
	{
		m_Vec3 start_pos;
		m_Vec3 pos;
		float angle[2]; // 0 - z, 1 - x
		unsigned char rocket_id;
		Time start_time= Time::FromSeconds(0);

		unsigned int frame;
	};

	typedef std::unordered_map< EntityId, Rocket > RocketsContainer;

	struct DynamicItem
	{
		m_Vec3 pos;
		Time birth_time= Time::FromSeconds(0); // TODO - does this need?
		unsigned int frame;
		unsigned char item_type_id;
	};

	typedef std::unordered_map< EntityId, DynamicItem > DynamicItemsContainer;

public:
	MapState(
		const MapDataConstPtr& map,
		const GameResourcesConstPtr& game_resources,
		Time map_start_time );
	~MapState();

	const DynamicWalls& GetDynamicWalls() const;
	const StaticModels& GetStaticModels() const;
	const Items& GetItems() const;
	const SpriteEffects& GetSpriteEffects() const;
	const MonstersBodyParts& GetMonstersBodyParts() const;
	const MonstersContainer& GetMonsters() const;
	const RocketsContainer& GetRockets() const;
	const DynamicItemsContainer& GetDynamicItems() const;

	float GetSpritesFrame() const;

	void Tick( Time current_time );

	void ProcessMessage( const Messages::MonsterState& message );
	void ProcessMessage( const Messages::WallPosition& message );
	void ProcessMessage( const Messages::ItemState& message );
	void ProcessMessage( const Messages::StaticModelState& message );
	void ProcessMessage( const Messages::SpriteEffectBirth& message );
	void ProcessMessage( const Messages::ParticleEffectBirth& message );
	void ProcessMessage( const Messages::MonsterPartBirth& message );
	void ProcessMessage( const Messages::MonsterBirth& message );
	void ProcessMessage( const Messages::MonsterDeath& message );
	void ProcessMessage( const Messages::RocketState& message );
	void ProcessMessage( const Messages::RocketBirth& message );
	void ProcessMessage( const Messages::RocketDeath& message );
	void ProcessMessage( const Messages::DynamicItemBirth& message );
	void ProcessMessage( const Messages::DynamicItemDeath& message );

private:
	const MapDataConstPtr map_data_;
	const GameResourcesConstPtr game_resources_;
	const Time map_start_time_;
	Time last_tick_time_;

	LongRand random_generator_;

	DynamicWalls dynamic_walls_;
	StaticModels static_models_;
	Items items_;
	SpriteEffects sprite_effects_;
	MonstersBodyParts monsters_body_parts_;
	MonstersContainer monsters_;
	RocketsContainer rockets_;
	DynamicItemsContainer dynamic_items_;
};

} // namespace PanzerChasm
