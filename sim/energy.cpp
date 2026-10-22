#include "sim/energy.h"
#include <algorithm>
#include <stdexcept>
namespace nuc::sim {
void EnergyNetwork::add(EnergyNode n){if(n.id==0||n.production<0||n.demand<0||n.capacity<0||n.stored<0||n.stored>n.capacity)throw std::invalid_argument("invalid energy node");if(find(n.id))throw std::invalid_argument("duplicate energy node id");nodes_.push_back(n);}
const EnergyNode* EnergyNetwork::find(std::uint32_t id) const noexcept{auto i=std::find_if(nodes_.begin(),nodes_.end(),[&](const auto& n){return n.id==id;});return i==nodes_.end()?nullptr:&*i;}
void EnergyNetwork::tick(){produced_=0;consumed_=0;std::int64_t available=0;for(auto& n:nodes_){n.allocated=0;if(n.role==EnergyRole::Producer){available+=n.production;produced_+=n.production;}}
 std::vector<EnergyNode*> consumers;for(auto& n:nodes_)if(n.role==EnergyRole::Consumer)consumers.push_back(&n);std::stable_sort(consumers.begin(),consumers.end(),[](auto a,auto b){return a->priority>b->priority;});
 for(auto* n:consumers){auto need=n->demand;if(available<need){for(auto& s:nodes_)if(s.role==EnergyRole::Storage&&s.stored>0&&available<need){auto draw=std::min(s.stored,need-available);s.stored-=draw;available+=draw;}}n->allocated=std::min(need,available);available-=n->allocated;consumed_+=n->allocated;}
 for(auto& s:nodes_)if(s.role==EnergyRole::Storage&&available>0){auto charge=std::min(s.capacity-s.stored,available);s.stored+=charge;available-=charge;}
}
}
