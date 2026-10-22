#include "core/ecs/world.h"
#include <limits>
namespace nuc::ecs {
Entity World::create() {
 std::uint32_t i;
 if (!free_indices_.empty()) {
  i = free_indices_.back();
  free_indices_.pop_back();
 } else {
  if (generations_.size() >= std::numeric_limits<std::uint32_t>::max())
   throw std::length_error("entity capacity exhausted");
  i = static_cast<std::uint32_t>(generations_.size());
  generations_.push_back(1);
  try {
   alive_.push_back(false);
  } catch (...) {
   generations_.pop_back();
   throw;
  }
 }
 alive_[i] = true;
 ++alive_count_;
 return make_entity(i, generations_[i]);
}
Entity World::create_at(Entity requested) {
 if(requested==null_entity||entity_generation(requested)==0)
  throw std::invalid_argument("restored entity must have a nonzero index/generation identity");
 const auto index=entity_index(requested);
 if(index<alive_.size()&&alive_[index]) throw std::invalid_argument("restored entity index is already alive");
 const auto target_size=static_cast<std::size_t>(index)+1;
 if(target_size>std::numeric_limits<std::uint32_t>::max())
  throw std::length_error("restored entity index exceeds entity capacity");
 if(target_size>generations_.size()) {
  const auto old_size=generations_.size();
  free_indices_.reserve(free_indices_.size()+static_cast<std::size_t>(index)-old_size);
  generations_.resize(target_size,1);
  try { alive_.resize(target_size,false); }
  catch(...) { generations_.resize(old_size); throw; }
  for(std::size_t slot=old_size;slot<index;++slot) free_indices_.push_back(static_cast<std::uint32_t>(slot));
 } else {
  free_indices_.erase(std::remove(free_indices_.begin(),free_indices_.end(),index),free_indices_.end());
 }
 generations_[index]=entity_generation(requested);
 alive_[index]=true;
 ++alive_count_;
 return requested;
}
void World::destroy(Entity e) {
 if (!alive(e)) return;
 const auto i = entity_index(e);
 // Allocate the free-list entry before mutating the entity so allocation
 // failure leaves the world unchanged.
 free_indices_.push_back(i);
 for (auto& entry : pools_) entry.second->erase(i);
 alive_[i] = false;
 if (++generations_[i] == 0) ++generations_[i];
 --alive_count_;
}
bool World::alive(Entity e) const noexcept {
 const auto i = entity_index(e);
 return e != null_entity && i < alive_.size() && alive_[i] && generations_[i] == entity_generation(e);
}
Entity World::entity_at(std::uint32_t i) const noexcept {
 return i < alive_.size() && alive_[i] ? make_entity(i, generations_[i]) : null_entity;
}
}
