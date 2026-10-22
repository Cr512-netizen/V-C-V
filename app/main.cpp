#include "app/loop.h"
#include "app/edit_mode.h"
#include "app/edit_queue.h"
#include "core/config.h"
#include "core/ecs/world.h"
#include "core/ecs/view.h"
#include "core/jobs.h"
#include "core/log.h"
#include "platform/input.h"
#include "platform/camera.h"
#include "platform/window.h"
#include "render/gl/gl_renderer.h"
#include "render/gl/mesh_upload.h"
#include "render/null_renderer.h"
#include "render/chunk_mesh.h"
#include "render/renderer.h"
#include "script/lua_state.h"
#include "script/mod_loader.h"
#include "script/events.h"
#include "sim/npc.h"
#include "sim/machine.h"
#include "sim/energy.h"
#include "sim/recipes.h"
#include "save/world_save.h"
#include "physics/destruction.h"
#include "physics/system.h"
#include "world/generator.h"
#include "world/chunk_manager.h"
#include "world/gen_resources.h"
#include "world/registry.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
namespace {
std::filesystem::path data_path(const std::filesystem::path& relative) {
 if(std::filesystem::exists(relative)) return relative;
#ifdef NUC_SOURCE_DIR
 const auto source=std::filesystem::path(NUC_SOURCE_DIR)/relative;
 if(std::filesystem::exists(source)) return source;
#endif
 return relative;
}
std::int32_t chunk_coord(int value) {
 auto q=value/static_cast<int>(nuc::world::Chunk::extent);
 if(value%static_cast<int>(nuc::world::Chunk::extent)<0) --q;
 return q;
}
}
int main() {
 try {
  auto config=nuc::Config::load(data_path("config.toml").string());
  if (config.renderer == "vulkan") { nuc::log::error("Vulkan renderer is not implemented yet"); return 2; }
  if (config.renderer != "gl_mesh" && config.renderer != "null" && config.renderer != "opengl_stub") {
   nuc::log::error("Unknown renderer: " + config.renderer); return 2;
  }
  if (config.renderer == "opengl_stub") config.renderer = "gl_mesh";
  nuc::log::info("Starting Nuc-lVuc-l");
  nuc::Jobs jobs(config.threads);
  nuc::save::WorldSaveData save_data;
  save_data.seed=1337;
  if(std::filesystem::exists("world.save")) {
   try { save_data=nuc::save::WorldSave::load("world.save"); }
   catch(const std::exception& e) { nuc::log::warn(std::string("Could not load world.save: ")+e.what(),"save"); }
  }

  nuc::world::Registry registry;
  registry.load(data_path("data/materials.json"),data_path("data/blocks.json"));
  nuc::world::ResourceGenerator resource_generator;
  resource_generator.load(data_path("data/biomes.json"));
  for(const auto& biome:resource_generator.biomes()) for(const auto& rule:biome.resources) {
   if(!registry.material_by_id(rule.id)||!registry.material_by_id(rule.source_material))
    throw std::runtime_error("resource rule in biome '"+biome.id+"' references an undefined material");
  }
  std::map<std::pair<std::int32_t,std::int32_t>,nuc::world::Chunk> chunks;
  for(auto& chunk:nuc::save::WorldSave::decode_chunks(save_data.chunks))
   chunks.emplace(std::pair{chunk.chunk_x(),chunk.chunk_z()},std::move(chunk));
  const auto seed=static_cast<std::int32_t>(save_data.seed&0x7fffffffULL);
  nuc::world::ChunkGenerator generator({seed,0.035F,12.0F,8.0F,10});
  std::function<void(std::int32_t,std::int32_t)> chunk_loaded_callback;
  auto ensure_chunk=[&](std::int32_t cx,std::int32_t cz)->nuc::world::Chunk& {
   const auto key=std::pair{cx,cz};
   auto it=chunks.find(key);
   if(it==chunks.end()) {
    auto generated=generator.generate(cx,cz);
    resource_generator.apply(generated,save_data.seed);
    it=chunks.emplace(key,std::move(generated)).first;
    if(chunk_loaded_callback) chunk_loaded_callback(cx,cz);
   }
   return it->second;
  };
  for(std::int32_t z=-1;z<=1;++z) for(std::int32_t x=-1;x<=1;++x) (void)ensure_chunk(x,z);
  for(const auto& edit:save_data.voxel_edits) (void)ensure_chunk(chunk_coord(edit.x),chunk_coord(edit.z));
  nuc::world::ChunkManager chunk_manager(chunks);
  for(const auto& edit:save_data.voxel_edits) {
   const nuc::world::VoxelCoord position{edit.x,edit.y,edit.z};
   if(!chunk_manager.set_voxel(position,edit.material)&&chunk_manager.get_voxel(position)!=edit.material)
    throw std::runtime_error("could not restore saved voxel edit");
  }

  nuc::LuaState boot_script;
  boot_script.boot(data_path("scripts/boot.lua").string());
  nuc::ecs::World ecs;
  nuc::sim::RecipeBook recipes;
  recipes.load(data_path("data/recipes.json"));
  nuc::sim::EnergyNetwork energy_network;
  std::vector<nuc::sim::Civilization> civilizations;
  civilizations.reserve(save_data.civilizations.size());
  for(const auto& saved:save_data.civilizations) {
   nuc::sim::Civilization civ; civ.id=saved.id; civ.population=saved.population;
   civ.resources=saved.resources; civ.production_per_second=saved.production_per_second;
   civ.relations.insert(saved.relations.begin(),saved.relations.end()); civ.ticks_into_second=saved.ticks_into_second;
   civilizations.push_back(std::move(civ));
  }
  nuc::save::WorldSave::restore_entities(ecs,save_data.entities);
  nuc::script::EventBus event_bus;
  nuc::script::ModLoader mod_loader(ecs);
  event_bus.subscribe(nuc::script::EventType::OnTick,[&](const nuc::script::Event& event){
   mod_loader.emit_event(nuc::script::EventBus::name(event.type),std::to_string(event.tick));
  });
  event_bus.subscribe(nuc::script::EventType::OnChunkLoad,[&](const nuc::script::Event& event){
   mod_loader.emit_event(nuc::script::EventBus::name(event.type),
    std::to_string(event.chunk_x)+","+std::to_string(event.chunk_z));
  });
  auto voxel_event=[&](const nuc::script::Event& event) {
   mod_loader.emit_event(nuc::script::EventBus::name(event.type),
    std::to_string(event.x)+","+std::to_string(event.y)+","+std::to_string(event.z)+","+std::to_string(event.material));
  };
  event_bus.subscribe(nuc::script::EventType::OnVoxelBreak,voxel_event);
  event_bus.subscribe(nuc::script::EventType::OnVoxelPlace,voxel_event);
  event_bus.subscribe(nuc::script::EventType::OnEntitySpawn,[&](const nuc::script::Event& event) {
   mod_loader.emit_event(nuc::script::EventBus::name(event.type),std::to_string(event.entity));
  });
  auto lifecycle_event=[&](const nuc::script::Event& event) {
   mod_loader.emit_event(nuc::script::EventBus::name(event.type),event.data);
  };
  event_bus.subscribe(nuc::script::EventType::OnLoad,lifecycle_event);
  event_bus.subscribe(nuc::script::EventType::OnSave,lifecycle_event);
  chunk_loaded_callback=[&](std::int32_t cx,std::int32_t cz) {
   nuc::script::Event event; event.type=nuc::script::EventType::OnChunkLoad;
   event.chunk_x=cx; event.chunk_z=cz; event_bus.emit(event);
  };
  auto shared=mod_loader.shared_state();
  bool mods_ready=false;
  shared->recipes=&recipes;
  shared->energy_network=&energy_network;
  shared->entity_spawned=[&](nuc::ecs::Entity entity) {
   if(!mods_ready) return;
   nuc::script::Event event; event.type=nuc::script::EventType::OnEntitySpawn; event.entity=entity; event_bus.emit(event);
  };
  shared->voxel_reader=[&](int x,int y,int z)->std::uint32_t {
   if(y<0||y>=static_cast<int>(nuc::world::Chunk::extent)) return 0;
   const auto cx=chunk_coord(x),cz=chunk_coord(z);
   const auto lx=x-cx*static_cast<int>(nuc::world::Chunk::extent);
   const auto lz=z-cz*static_cast<int>(nuc::world::Chunk::extent);
   return ensure_chunk(cx,cz).at(static_cast<std::size_t>(lx),static_cast<std::size_t>(y),static_cast<std::size_t>(lz));
  };
  shared->voxel_writer=[&](int x,int y,int z,std::uint32_t material) {
   if(y<0||y>=static_cast<int>(nuc::world::Chunk::extent)) return;
   if(material!=0&&!registry.material_by_id(material)) throw std::invalid_argument("Lua voxel API references an undefined material ID");
   const auto cx=chunk_coord(x),cz=chunk_coord(z);
   const auto lx=x-cx*static_cast<int>(nuc::world::Chunk::extent);
   const auto lz=z-cz*static_cast<int>(nuc::world::Chunk::extent);
   ensure_chunk(cx,cz).set(static_cast<std::size_t>(lx),static_cast<std::size_t>(y),static_cast<std::size_t>(lz),material);
  };
  shared->voxel_changed=[&](int x,int y,int z,std::uint32_t material) {
   nuc::script::Event event;
   event.type=material==0?nuc::script::EventType::OnVoxelBreak:nuc::script::EventType::OnVoxelPlace;
   event.x=x; event.y=y; event.z=z; event.material=material; event_bus.emit(event);
  };
  mod_loader.restore_mod_states(save_data.mod_state);
  for(const auto& error:mod_loader.discover_and_load(data_path("mods").string())) nuc::log::error(error,"mods");
  mods_ready=true;
  nuc::ecs::view<nuc::sim::NpcTag,nuc::sim::NpcPosition>(ecs).each([&](nuc::ecs::Entity entity,nuc::sim::NpcTag&,nuc::sim::NpcPosition&) {
   nuc::script::Event event; event.type=nuc::script::EventType::OnEntitySpawn; event.entity=entity; event_bus.emit(event);
  });
  nuc::ecs::view<nuc::sim::MachineInput,nuc::sim::MachineOutput,nuc::sim::MachineRecipe,nuc::sim::MachineTimer>(ecs).each([&](nuc::ecs::Entity entity,nuc::sim::MachineInput&,nuc::sim::MachineOutput&,nuc::sim::MachineRecipe&,nuc::sim::MachineTimer&) {
   nuc::script::Event event; event.type=nuc::script::EventType::OnEntitySpawn; event.entity=entity; event_bus.emit(event);
  });
  for(const auto& [coord,chunk]:chunks) {
   (void)chunk;
   nuc::script::Event event; event.type=nuc::script::EventType::OnChunkLoad;
   event.chunk_x=coord.first; event.chunk_z=coord.second; event_bus.emit(event);
  }
  nuc::script::Event on_load; on_load.type=nuc::script::EventType::OnLoad;
  on_load.data=std::to_string(save_data.seed); event_bus.emit(on_load);
  std::uint64_t logic_tick=0;
  nuc::Window window(config.width,config.height,"Nuc-lVuc-l",config.vsync);
  auto render_chunks=chunk_manager.materialized_snapshot();
  std::vector<nuc::render::ChunkMesh> chunk_meshes;
  chunk_meshes.reserve(render_chunks.size());
  for (const auto& [coord, chunk] : render_chunks) {
   nuc::render::ChunkNeighbors neighbors;
   const auto west = render_chunks.find({coord.first - 1, coord.second});
   const auto east = render_chunks.find({coord.first + 1, coord.second});
   const auto north = render_chunks.find({coord.first, coord.second - 1});
   const auto south = render_chunks.find({coord.first, coord.second + 1});
   if (west != render_chunks.end()) neighbors.west = &west->second;
   if (east != render_chunks.end()) neighbors.east = &east->second;
   if (north != render_chunks.end()) neighbors.north = &north->second;
   if (south != render_chunks.end()) neighbors.south = &south->second;
   chunk_meshes.push_back(nuc::render::build_chunk_mesh(chunk, neighbors));
  }
  std::unique_ptr<nuc::IRenderer> renderer;
  if (config.renderer == "null") renderer = std::make_unique<nuc::NullRenderer>();
  else renderer = std::make_unique<nuc::GlRenderer>();
  auto* gl_renderer = dynamic_cast<nuc::GlRenderer*>(renderer.get());
  nuc::Camera camera({16.0F, 18.0F, 40.0F},
   static_cast<float>(window.width()) / static_cast<float>(std::max(window.height(),1)));
  camera.set_rotation(-90.0F,-30.0F);
  if (!renderer->init()) { nuc::log::error("Renderer initialization failed"); return 1; }
  renderer->resize(window.width(),window.height());
  nuc::EditMode edit_mode(chunk_manager,3);
  nuc::physics::DebrisPool debris_pool;
  std::vector<nuc::physics::RigidBody> restored_bodies;
  nuc::ecs::view<nuc::physics::RigidBody>(ecs).each([&](nuc::ecs::Entity,nuc::physics::RigidBody& body){restored_bodies.push_back(body);});
  if(debris_pool.reserve(restored_bodies).rejected)
   throw std::runtime_error("saved physics bodies exceed the per-chunk debris cap");
  edit_mode.set_break_callback([&](nuc::world::VoxelCoord position,std::uint32_t material){
   const auto seed=nuc::physics::fracture_seed(save_data.seed,logic_tick,position.x,position.y,position.z);
   auto fracture=nuc::physics::fracture_block({position.x,position.y,position.z},material,seed,camera.forward()*2.0F+glm::vec3(0.0F,1.0F,0.0F));
   const auto reserved=debris_pool.reserve(fracture.boundary_bodies);
   for(const auto index:reserved.accepted_indices){
    const auto entity=ecs.create();
    ecs.emplace<nuc::physics::RigidBody>(entity,std::move(fracture.boundary_bodies[index]));
    if(shared->entity_spawned)shared->entity_spawned(entity);
   }
   if(reserved.rejected)nuc::log::warn("Debris pool cap reached; excess fracture bodies collapsed","physics");
  });
  std::vector<nuc::render::gl::MeshUpload> gpu_meshes;
  nuc::render::gl::MeshUpload debris_mesh;
  std::vector<nuc::BodyDraw> body_draws;
  std::vector<nuc::ChunkDraw> chunk_draws;
  std::map<std::pair<int,int>,std::size_t> gpu_mesh_indices;
  if (gl_renderer) {
   gpu_meshes.reserve(chunk_meshes.size());
   for (const auto& mesh : chunk_meshes) {
    nuc::render::gl::MeshUpload upload;
    if (!upload.upload(mesh)) { renderer->shutdown(); nuc::log::error("Chunk GPU upload failed"); return 1; }
    gpu_meshes.push_back(std::move(upload));
   }
   chunk_draws.reserve(chunks.size());
   std::size_t mesh_index = 0;
   for (const auto& [coord, chunk] : chunks) {
    (void)chunk;
    gpu_mesh_indices[coord]=mesh_index;
    chunk_draws.push_back({&gpu_meshes[mesh_index++],
     static_cast<float>(coord.first * static_cast<std::int32_t>(nuc::world::Chunk::extent)),
     static_cast<float>(coord.second * static_cast<std::int32_t>(nuc::world::Chunk::extent))});
   }
   gl_renderer->set_world(&chunk_draws);
   nuc::world::Chunk unit_cube_chunk;
   unit_cube_chunk.set(0,0,0,1);
   auto unit_cube_cpu=nuc::render::build_chunk_mesh(unit_cube_chunk);
   for(auto& vertex:unit_cube_cpu.vertices)vertex.material=0;
   if(!debris_mesh.upload(unit_cube_cpu)){renderer->shutdown();nuc::log::error("Debris cube GPU upload failed");return 1;}
   gl_renderer->set_bodies(&body_draws,&debris_mesh);
   gl_renderer->set_view_projection(camera.view_projection());
  }
  nuc::EditRequestQueue edit_queue;
  auto last=std::chrono::steady_clock::now(); auto last_render=last; unsigned frames=0;
  nuc::Loop::run(
   [&](double tick_dt){
    ++logic_tick;
    // World edits are applied inside the tick, never from a frame: the number of ticks
    // no longer depends on the frame rate, which is what deterministic replay needs.
    for(const auto action:edit_queue.drain())
     edit_mode.apply(action,camera.position(),camera.forward(),8.0F);
    energy_network.tick();
    nuc::sim::MachineSystem::tick(ecs,recipes,0);
    nuc::sim::NpcSystem::tick(ecs);
    nuc::sim::CivilizationSystem::tick(civilizations,static_cast<unsigned>(config.tick_rate));
    nuc::sim::NpcSystem::update_tiers(ecs,0,0,0,32);
    nuc::physics::PhysicsSystem::tick(ecs,tick_dt,[&](int x,int y,int z){
     return chunk_manager.get_voxel({x,y,z})!=0;
    });
    nuc::script::Event tick; tick.type=nuc::script::EventType::OnTick; tick.tick=logic_tick; event_bus.emit(tick);
    const auto input=nuc::Input::capture(window.native());
    if(input.escape) window.close();
    chunk_manager.rebuild_dirty_at_tick_end();
   },
   [&](double render_alpha){
    const auto now=std::chrono::steady_clock::now();
    const double frame_dt=std::chrono::duration<double>(now-last_render).count();
    last_render=now;
    if (gl_renderer) {
     camera.set_aspect(static_cast<float>(window.width()) / static_cast<float>(std::max(window.height(),1)));
     camera.update(window.native(),frame_dt);
     gl_renderer->set_view_projection(camera.view_projection());
     const auto input=nuc::Input::capture(window.native());
     edit_queue.observe(input.mouse_left,input.mouse_right);
     body_draws.clear();
     nuc::ecs::view<nuc::physics::RigidBody>(ecs).each([&](nuc::ecs::Entity,nuc::physics::RigidBody& body){
      body_draws.push_back({nuc::physics::interpolated_position(body,static_cast<float>(render_alpha)),body.half_extents,body.material});
     });
     for(auto& update:chunk_manager.drain_upload_queue_on_render_thread()) {
      const auto index=gpu_mesh_indices.find(update.coord);
      if(index!=gpu_mesh_indices.end()&&!gpu_meshes[index->second].upload(update.mesh)) {
       nuc::log::error("Chunk GPU upload failed; retaining previous mesh","render");
       chunk_manager.mark_chunk_dirty(update.coord.first,update.coord.second);
      }
     }
    }
    renderer->resize(window.width(),window.height());
    renderer->begin_frame(); renderer->end_frame(); window.swap(); ++frames;
    const double elapsed=std::chrono::duration<double>(now-last).count();
    if(elapsed>=1.0){nuc::log::info("FPS: "+std::to_string(frames/elapsed),"render");frames=0;last=now;}
   },
   [&]{window.poll();return !window.should_close();},config.tick_rate);
  nuc::script::Event on_save; on_save.type=nuc::script::EventType::OnSave;
  on_save.data=std::to_string(save_data.seed); event_bus.emit(on_save);
  renderer->shutdown();
  for (auto& mesh : gpu_meshes) mesh.reset();
  debris_mesh.reset();
  save_data.format_version=2;
  save_data.voxel_edits.clear();
  for(const auto& [position,material]:chunk_manager.delta().entries())
   save_data.voxel_edits.push_back({position.x,position.y,position.z,material});
  auto captured=nuc::save::WorldSave::capture_entities(ecs);
  for(const auto& entity:save_data.entities) if(entity.kind!="npc"&&entity.kind!="machine"&&entity.kind!="physics_body") captured.push_back(entity);
  save_data.entities=std::move(captured);
  save_data.mod_state=mod_loader.capture_mod_states();
  save_data.civilizations.clear();
  for(const auto& civ:civilizations) {
   nuc::save::SavedCivilization saved; saved.id=civ.id; saved.population=civ.population;
   saved.resources=civ.resources; saved.production_per_second=civ.production_per_second;
   saved.relations.insert(civ.relations.begin(),civ.relations.end()); saved.ticks_into_second=civ.ticks_into_second;
   save_data.civilizations.push_back(std::move(saved));
  }
  nuc::save::WorldSave::save("world.save",save_data);
  nuc::log::info("Shutdown complete"); return 0;
 } catch(const std::exception& e) { nuc::log::error(e.what()); return 1; }
}
