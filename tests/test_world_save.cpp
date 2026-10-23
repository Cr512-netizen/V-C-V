#include <catch2/catch_test_macros.hpp>
#include "save/world_save.h"
#include "world/chunk_manager.h"
#include "world/gen_resources.h"
#include "sim/npc.h"
#include "physics/body.h"
#include "test_scratch.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
TEST_CASE("world save and load preserve deterministic state hash") {
 nuc::world::Chunk chunk(2,-1);chunk.set(1,2,3,7);chunk.set(31,31,31,4);nuc::save::WorldSaveData data;data.format_version=1;data.seed=987;data.chunks=nuc::save::WorldSave::encode_chunks({chunk});data.entities.push_back({42,"npc",1,2,3,{{5,8,64}},"crush",2});nuc::save::SavedCivilization civ;civ.id=7;civ.population=30;civ.resources[5]=120;civ.production_per_second[5]=3;civ.relations[9]=-3;civ.ticks_into_second=7;data.civilizations.push_back(civ);data.mod_state["core"]="{\"counter\":42}";
 auto path=nuc::test::scratch_file("world_save_test.json");nuc::save::WorldSave::save(path,data);auto loaded=nuc::save::WorldSave::load(path);REQUIRE(loaded.hash()==data.hash());auto chunks=nuc::save::WorldSave::decode_chunks(loaded.chunks);REQUIRE(chunks.size()==1);REQUIRE(chunks[0].voxels()==chunk.voxels());std::filesystem::remove(path);
}
TEST_CASE("world save delta encoding handles ten thousand voxels") {
 nuc::world::Chunk chunk(-3,4);for(std::uint32_t i=0;i<10000;++i){const auto x=i%32;const auto y=(i/32)%32;const auto z=i/(32*32);chunk.set(x,y,z,(i%6)+1);}auto delta=nuc::save::WorldSave::encode_chunks({chunk});REQUIRE(delta[0].delta.size()==10000);auto restored=nuc::save::WorldSave::decode_chunks(delta);REQUIRE(restored[0].voxels()==chunk.voxels());
}
TEST_CASE("resource generation is deterministic and follows biome rules") {
 nuc::world::ResourceGenerator gen;gen.load(std::filesystem::path(NUC_SOURCE_DIR)/"data/biomes.json");nuc::world::Chunk a(0,0),b(0,0);for(std::size_t z=0;z<32;++z)for(std::size_t y=0;y<32;++y)for(std::size_t x=0;x<32;++x){a.set(x,y,z,1);b.set(x,y,z,1);}gen.apply(a,1234);gen.apply(b,1234);REQUIRE(a.voxels()==b.voxels());REQUIRE(a.voxels()!=nuc::world::Chunk(0,0).voxels());
}

TEST_CASE("NPC inventory and work state survive ECS save restore") {
 nuc::ecs::World source;auto recycled=source.create();source.destroy(recycled);auto id=nuc::sim::NpcSystem::spawn(source,3,2,1);
 auto& goal=source.get<nuc::sim::NpcGoal>(id);goal.target_x=9;goal.work_item=6;goal.work_count=4;goal.work_ticks=5;goal.remaining_ticks=2;
 source.get<nuc::sim::NpcInventory>(id).inventory.insert({6,3,64});
 auto entities=nuc::save::WorldSave::capture_entities(source);nuc::ecs::World restored;nuc::save::WorldSave::restore_entities(restored,entities);
 REQUIRE(restored.alive(id));REQUIRE(restored.get<nuc::sim::NpcPosition>(id).x==3);REQUIRE(restored.get<nuc::sim::NpcGoal>(id).target_x==9);REQUIRE(restored.get<nuc::sim::NpcGoal>(id).remaining_ticks==2);REQUIRE(restored.get<nuc::sim::NpcInventory>(id).inventory.count(6)==3);
}

TEST_CASE("version 2 preserves ten thousand sparse voxel edits and canonical hash") {
    nuc::save::WorldSaveData data;
    data.seed=123456;
    for(int i=0;i<10000;++i)
        data.voxel_edits.push_back({i-5000,i%32,(i/32)-156,static_cast<std::uint32_t>(i%5)});
    const auto expected=data.voxel_edits;
    const auto path=nuc::test::scratch_file("world_save_v2_test.json");
    nuc::save::WorldSave::save(path,data);
    const auto loaded=nuc::save::WorldSave::load(path);
    REQUIRE(loaded.format_version==2);
    REQUIRE(loaded.hash()==data.hash());
    REQUIRE(loaded.voxel_edits.size()==10000);
    auto sorted_expected=expected;
    std::sort(sorted_expected.begin(),sorted_expected.end(),[](const auto&a,const auto&b){if(a.x!=b.x)return a.x<b.x;if(a.y!=b.y)return a.y<b.y;return a.z<b.z;});
    REQUIRE(loaded.voxel_edits==sorted_expected);
    std::filesystem::remove(path);
}
TEST_CASE("version 2 save rejects duplicate and out of range edits") {
    nuc::save::WorldSaveData data;
    data.voxel_edits={{1,1,1,0},{1,1,1,4}};
    const auto path=nuc::test::scratch_file("world_save_duplicate_test.json");
    REQUIRE_THROWS(nuc::save::WorldSave::save(path,data));
    data.voxel_edits={{1,32,1,4}};
    REQUIRE_THROWS(nuc::save::WorldSave::save(path,data));
    std::filesystem::remove(path);
}
TEST_CASE("version 1 chunk snapshot remains readable") {
    nuc::world::Chunk chunk(-2,3);
    chunk.set(4,5,6,9);
    nuc::save::WorldSaveData legacy;
    legacy.format_version=1;
    legacy.seed=77;
    legacy.chunks=nuc::save::WorldSave::encode_chunks({chunk});
    const auto path=nuc::test::scratch_file("world_save_v1_test.json");
    nuc::save::WorldSave::save(path,legacy);
    const auto loaded=nuc::save::WorldSave::load(path);
    REQUIRE(loaded.format_version==1);
    REQUIRE(loaded.hash()==legacy.hash());
    REQUIRE(nuc::save::WorldSave::decode_chunks(loaded.chunks).front().voxels()==chunk.voxels());
    std::filesystem::remove(path);
}

TEST_CASE("loaded sparse edits overlay generated base including explicit air") {
    nuc::world::Chunk base(0,0);
    base.set(1,1,1,3);
    nuc::save::WorldSaveData data;
    data.seed=42;
    data.voxel_edits={{1,1,1,0},{2,1,1,4}};
    const auto path=nuc::test::scratch_file("world_save_overlay_test.json");
    nuc::save::WorldSave::save(path,data);
    const auto loaded=nuc::save::WorldSave::load(path);
    nuc::world::ChunkManager::ChunkMap chunks;
    chunks.emplace(std::pair{0,0},base);
    nuc::world::ChunkManager manager(chunks);
    for(const auto& edit:loaded.voxel_edits)
        REQUIRE((manager.set_voxel({edit.x,edit.y,edit.z},edit.material) ||
                 manager.get_voxel({edit.x,edit.y,edit.z})==edit.material));
    const auto materialized=manager.materialized_snapshot();
    REQUIRE(materialized.at({0,0}).at(1,1,1)==0);
    REQUIRE(materialized.at({0,0}).at(2,1,1)==4);
    REQUIRE(base.at(1,1,1)==3);
    std::filesystem::remove(path);
}

TEST_CASE("version 2 loader rejects duplicate edits, invalid height and unknown version") {
    const auto path=nuc::test::scratch_file("world_save_invalid_test.json");
    const auto write=[&](const std::string& json){std::ofstream out(path,std::ios::trunc);out<<json;};
    const std::string suffix=R"("entities":[],"civilizations":[],"mod_state":{}})";
    write(R"({"version":2,"seed":1,"chunks":[],"voxel_edits":[{"x":1,"y":1,"z":1,"material":0},{"x":1,"y":1,"z":1,"material":4}],)" + suffix);
    REQUIRE_THROWS(nuc::save::WorldSave::load(path));
    write(R"({"version":2,"seed":1,"chunks":[],"voxel_edits":[{"x":1,"y":32,"z":1,"material":4}],)" + suffix);
    REQUIRE_THROWS(nuc::save::WorldSave::load(path));
    write(R"({"version":99,"seed":1,"chunks":[],"voxel_edits":[],"entities":[],"civilizations":[],"mod_state":{}})");
    REQUIRE_THROWS(nuc::save::WorldSave::load(path));
    std::filesystem::remove(path);
}
TEST_CASE("save hash is independent of sparse edit input ordering") {
    nuc::save::WorldSaveData first,second;
    first.seed=second.seed=91;
    first.voxel_edits={{8,2,1,0},{-1,3,7,4},{0,0,0,2}};
    second.voxel_edits={first.voxel_edits[2],first.voxel_edits[0],first.voxel_edits[1]};
    REQUIRE(first.hash()==second.hash());
    second.voxel_edits[0].material=9;
    REQUIRE(first.hash()!=second.hash());
}

TEST_CASE("save hash includes entity, civilization and mod state") {
    nuc::save::WorldSaveData base;
    base.seed=101;
    base.entities.push_back({7,"npc",1,2,3,{},"",0});
    const auto original=base.hash();
    auto changed=base;
    changed.entities[0].x=9;
    REQUIRE(changed.hash()!=original);
    changed=base;
    nuc::save::SavedCivilization civ;
    civ.id=3;
    civ.population=100;
    changed.civilizations.push_back(civ);
    REQUIRE(changed.hash()!=original);
    changed=base;
    changed.mod_state["core"]="counter=1";
    REQUIRE(changed.hash()!=original);
}

TEST_CASE("physics body ECS state survives world save and restore") {
    nuc::ecs::World source;
    const auto id=source.create();
    auto body=nuc::physics::make_body({2.25F,4.5F,-1.0F},{0.125F,0.125F,0.125F},0.5F,7);
    body.previous_position={2.2F,4.4F,-1.1F};
    body.velocity={1.0F,-2.0F,0.5F};
    body.sleep_time=1.25F;
    body.owner_chunk_x=-1;
    body.owner_chunk_z=0;
    body.force={99,99,99};
    source.emplace<nuc::physics::RigidBody>(id,body);
    nuc::save::WorldSaveData data;
    data.seed=123;
    data.entities=nuc::save::WorldSave::capture_entities(source);
    const auto path=nuc::test::scratch_file("physics_body_save_test.json");
    nuc::save::WorldSave::save(path,data);
    const auto loaded=nuc::save::WorldSave::load(path);
    nuc::ecs::World restored;
    nuc::save::WorldSave::restore_entities(restored,loaded.entities);
    REQUIRE(source.deterministic_hash()==restored.deterministic_hash());
    REQUIRE(glm::length(restored.get<nuc::physics::RigidBody>(id).force)<1e-6F);
    std::filesystem::remove(path);
}
