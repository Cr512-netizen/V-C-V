#pragma once
#include <cstdint>
#include <algorithm>
#include "core/sim/state_hash.h"
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
namespace nuc::ecs {
using Entity = std::uint64_t;
constexpr Entity null_entity = 0;
constexpr std::uint32_t entity_index(Entity e) noexcept { return static_cast<std::uint32_t>(e); }
constexpr std::uint32_t entity_generation(Entity e) noexcept { return static_cast<std::uint32_t>(e >> 32U); }
constexpr Entity make_entity(std::uint32_t i, std::uint32_t g) noexcept { return (static_cast<Entity>(g) << 32U) | i; }
template<class T> struct ComponentStateTraits {
 static constexpr bool registered = false;
 static constexpr std::uint64_t id = 0;
};
struct IComponentPool {
 virtual ~IComponentPool() = default;
 virtual void erase(std::uint32_t) = 0;
 virtual bool contains(std::uint32_t) const = 0;
 virtual const std::vector<std::uint32_t>& dense_indices() const = 0;
 virtual std::size_t size() const noexcept = 0;
 virtual std::uint64_t stable_type_id() const noexcept = 0;
 virtual void append_component_hash(sim::StateHash&, std::uint32_t) const = 0;
};
template<class T> class ComponentPool final : public IComponentPool {
public:
 template<class... A> T& emplace(std::uint32_t i, A&&... a) {
  if (contains(i)) throw std::logic_error("component already exists");
  if (sparse_.size() <= i) sparse_.resize(static_cast<std::size_t>(i) + 1, 0);
  dense_.emplace_back(std::forward<A>(a)...);
  try {
   indices_.push_back(i);
  } catch (...) {
   dense_.pop_back();
   throw;
  }
  sparse_[i] = static_cast<std::uint32_t>(dense_.size());
  return dense_.back();
 }
 void erase(std::uint32_t i) override {
  static_assert(std::is_nothrow_move_assignable_v<T>, "ECS components must be nothrow move-assignable for safe swap-and-pop removal");
  if (!contains(i)) return;
  const auto p = sparse_[i] - 1;
  const auto last = static_cast<std::uint32_t>(dense_.size() - 1);
  if (p != last) {
   dense_[p] = std::move(dense_[last]);
   indices_[p] = indices_[last];
   sparse_[indices_[p]] = p + 1;
  }
  dense_.pop_back();
  indices_.pop_back();
  sparse_[i] = 0;
 }
 bool contains(std::uint32_t i) const override { return i < sparse_.size() && sparse_[i] != 0; }
 T& get(std::uint32_t i) {
  if (!contains(i)) throw std::out_of_range("component missing");
  return dense_[sparse_[i] - 1];
 }
 const T& get(std::uint32_t i) const {
  if (!contains(i)) throw std::out_of_range("component missing");
  return dense_[sparse_[i] - 1];
 }
 const std::vector<std::uint32_t>& dense_indices() const override { return indices_; }
 std::size_t size() const noexcept override { return dense_.size(); }
 std::uint64_t stable_type_id() const noexcept override {
  if constexpr (ComponentStateTraits<T>::registered) return ComponentStateTraits<T>::id;
  else return 0;
 }
 void append_component_hash(sim::StateHash& hash, std::uint32_t i) const override {
  if constexpr (ComponentStateTraits<T>::registered) {
   ComponentStateTraits<T>::append(hash, get(i));
  } else {
   throw std::logic_error("component has no stable ECS state serializer");
  }
 }
private:
 std::vector<std::uint32_t> sparse_, indices_;
 std::vector<T> dense_;
};
class World {
public:
 Entity create();
 Entity create_at(Entity requested);
 void destroy(Entity);
 bool alive(Entity) const noexcept;
 Entity entity_at(std::uint32_t) const noexcept;
 template<class T, class... A> T& emplace(Entity e, A&&... a) {
  validate(e);
  return pool<T>().emplace(entity_index(e), std::forward<A>(a)...);
 }
 template<class T> bool has(Entity e) const {
  if (!alive(e)) return false;
  const auto* p = find_pool<T>();
  return p && p->contains(entity_index(e));
 }
 template<class T> T& get(Entity e) {
  validate(e);
  return pool<T>().get(entity_index(e));
 }
 template<class T> const T& get(Entity e) const {
  validate(e);
  const auto* p = find_pool<T>();
  if (!p) throw std::out_of_range("component pool missing");
  return p->get(entity_index(e));
 }
 template<class T> void remove(Entity e) {
  validate(e);
  if (auto* p = find_pool<T>()) p->erase(entity_index(e));
 }
 template<class T> ComponentPool<T>* try_pool() noexcept { return find_pool<T>(); }
 template<class T> const ComponentPool<T>* try_pool() const noexcept { return find_pool<T>(); }
 std::size_t entity_count() const noexcept { return alive_count_; }
 // Hashes live entities in index order and components in stable type-ID order.
 // Every component type present in the world must specialize ComponentStateTraits.
 std::uint64_t deterministic_hash() const {
  sim::StateHash hash;
  hash.text("nuc.ecs.world.v1");
  std::vector<const IComponentPool*> pools;
  pools.reserve(pools_.size());
  for (const auto& entry : pools_) if (entry.second->size() != 0) pools.push_back(entry.second.get());
  for (const auto* p : pools) {
   if (p->stable_type_id() == 0) throw std::logic_error("component type lacks a nonzero stable ECS type ID");
  }
  std::sort(pools.begin(), pools.end(), [](const auto* a, const auto* b) { return a->stable_type_id() < b->stable_type_id(); });
  for (std::size_t i = 1; i < pools.size(); ++i) {
   if (pools[i - 1]->stable_type_id() == pools[i]->stable_type_id()) throw std::logic_error("duplicate stable ECS component type ID");
  }
  // Allocator history affects future entity IDs, so it is part of simulation state.
  hash.integer<std::uint64_t>(generations_.size());
  for (std::uint32_t i = 0; i < generations_.size(); ++i) {
   hash.integer(i);
   hash.integer(generations_[i]);
   hash.integer<std::uint8_t>(alive_[i] ? 1 : 0);
  }
  hash.integer<std::uint64_t>(free_indices_.size());
  for (const auto i : free_indices_) hash.integer(i);
  hash.integer<std::uint64_t>(alive_count_);
  for (std::uint32_t i = 0; i < alive_.size(); ++i) {
   if (!alive_[i]) continue;
   hash.integer(i);
   hash.integer(generations_[i]);
   std::uint64_t count = 0;
   for (const auto* p : pools) if (p->contains(i)) ++count;
   hash.integer(count);
   for (const auto* p : pools) if (p->contains(i)) {
    hash.integer(p->stable_type_id());
    p->append_component_hash(hash, i);
   }
  }
  return hash.value();
 }
private:
 void validate(Entity e) const {
  if (!alive(e)) throw std::invalid_argument("invalid or dead entity");
 }
 template<class T> ComponentPool<T>& pool() {
  auto k = std::type_index(typeid(T));
  auto it = pools_.find(k);
  if (it == pools_.end()) it = pools_.emplace(k, std::make_unique<ComponentPool<T>>()).first;
  return *static_cast<ComponentPool<T>*>(it->second.get());
 }
 template<class T> ComponentPool<T>* find_pool() noexcept {
  auto it = pools_.find(std::type_index(typeid(T)));
  return it == pools_.end() ? nullptr : static_cast<ComponentPool<T>*>(it->second.get());
 }
 template<class T> const ComponentPool<T>* find_pool() const noexcept {
  auto it = pools_.find(std::type_index(typeid(T)));
  return it == pools_.end() ? nullptr : static_cast<const ComponentPool<T>*>(it->second.get());
 }
 std::vector<std::uint32_t> generations_, free_indices_;
 std::vector<bool> alive_;
 std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
 std::size_t alive_count_ = 0;
};
}
