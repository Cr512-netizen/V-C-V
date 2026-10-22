#include "script/events.h"
#include <algorithm>
#include <stdexcept>
namespace nuc::script {
std::uint64_t EventBus::subscribe(EventType type,Handler handler) {
 if(!handler) throw std::invalid_argument("event handler cannot be empty");
 const auto token=next_token_++;
 subscriptions_.push_back({token,type,std::move(handler)});
 return token;
}
bool EventBus::unsubscribe(std::uint64_t token) noexcept {
 const auto before=subscriptions_.size();
 subscriptions_.erase(std::remove_if(subscriptions_.begin(),subscriptions_.end(),
  [token](const auto& item){return item.token==token;}),subscriptions_.end());
 return subscriptions_.size()!=before;
}
void EventBus::emit(const Event& event) const {
 const auto snapshot=subscriptions_;
 for(const auto& sub:snapshot) if(sub.type==event.type) sub.handler(event);
}
const char* EventBus::name(EventType type) noexcept {
 switch(type) {
  case EventType::OnTick:return "on_tick";
  case EventType::OnVoxelBreak:return "on_voxel_break";
  case EventType::OnVoxelPlace:return "on_voxel_place";
  case EventType::OnChunkLoad:return "on_chunk_load";
  case EventType::OnEntitySpawn:return "on_entity_spawn";
  case EventType::OnSave:return "on_save";
  case EventType::OnLoad:return "on_load";
 }
 return "unknown";
}
}
