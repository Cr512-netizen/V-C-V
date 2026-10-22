#pragma once
#include <cstdint>
#include <vector>
namespace nuc::sim {
enum class EnergyRole { Producer, Consumer, Storage };
struct EnergyNode {std::uint32_t id{};EnergyRole role{EnergyRole::Consumer};std::int64_t production{},demand{},capacity{},stored{},allocated{};int priority{};};
class EnergyNetwork {public:void add(EnergyNode);void tick();const EnergyNode* find(std::uint32_t) const noexcept;std::int64_t produced_last_tick() const noexcept{return produced_;}std::int64_t consumed_last_tick() const noexcept{return consumed_;}private:std::vector<EnergyNode> nodes_;std::int64_t produced_{},consumed_{};};
}
