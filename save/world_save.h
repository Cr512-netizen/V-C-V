#pragma once
#include "sim/items.h"
#include "world/chunk.h"
#include <nlohmann/json.hpp>
#include "core/ecs/world.h"
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace nuc::save {
struct VoxelDelta {std::uint32_t index{},material{};};
struct SavedChunk {std::int32_t x{},z{};std::vector<VoxelDelta> delta;};
// Sparse world-space edit; material 0 explicitly records air over generated terrain.
struct SavedVoxelEdit {int x{},y{},z{};std::uint32_t material{};friend bool operator==(const SavedVoxelEdit&,const SavedVoxelEdit&)=default;};
struct SavedEntity {std::uint64_t id{};std::string kind;int x{},y{},z{};std::vector<sim::ItemStack> inventory;std::string recipe;std::uint32_t timer{};nlohmann::json extra=nlohmann::json::object();};
struct SavedCivilization {std::uint64_t id{},population{};std::map<sim::ItemId,std::int64_t> resources,production_per_second;std::map<std::uint64_t,int> relations;unsigned ticks_into_second{};};
// Canonical persistent state. Version 1 chunks are legacy materialized snapshots;
// version 2 stores new changes in voxel_edits while retaining optional legacy chunks.
struct WorldSaveData {std::uint64_t seed{};std::vector<SavedChunk> chunks;std::vector<SavedVoxelEdit> voxel_edits;std::vector<SavedEntity> entities;std::vector<SavedCivilization> civilizations;std::map<std::string,std::string> mod_state;std::uint32_t format_version{2};std::uint64_t hash() const;};
// Versioned, validated JSON persistence with temp-file replacement and legacy v1 reads.
class WorldSave {public:static std::vector<SavedChunk> encode_chunks(const std::vector<world::Chunk>&);static std::vector<world::Chunk> decode_chunks(const std::vector<SavedChunk>&);static void save(const std::filesystem::path&,const WorldSaveData&);static WorldSaveData load(const std::filesystem::path&);static std::vector<SavedEntity> capture_entities(ecs::World&);static void restore_entities(ecs::World&,const std::vector<SavedEntity>&);};
}
