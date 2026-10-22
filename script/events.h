#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace nuc::script {
enum class EventType { OnTick, OnVoxelBreak, OnVoxelPlace, OnChunkLoad, OnEntitySpawn, OnSave, OnLoad };
struct Event {
 EventType type{EventType::OnTick};
 std::uint64_t tick{};
 int x{},y{},z{},chunk_x{},chunk_z{};
 std::uint32_t material{};
 std::uint64_t entity{};
 std::string data;
};
class EventBus {
public:
 using Handler=std::function<void(const Event&)>;
 std::uint64_t subscribe(EventType,Handler);
 bool unsubscribe(std::uint64_t token) noexcept;
 void emit(const Event&) const;
 static const char* name(EventType) noexcept;
private:
 struct Subscription { std::uint64_t token; EventType type; Handler handler; };
 std::vector<Subscription> subscriptions_;
 std::uint64_t next_token_{1};
};
}
