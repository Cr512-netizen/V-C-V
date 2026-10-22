#include "app/edit_mode.h"
#include "world/raycast.h"

namespace nuc {
bool EditMode::apply(EditAction action,glm::vec3 origin,glm::vec3 direction,float reach) {
    const auto reader=[this](int x,int y,int z) { return world_.get_voxel({x,y,z}); };
    const auto hit=world::raycast(reader,origin,direction,reach);
    if(!hit) return false;
    if(action==EditAction::break_block) {
        const auto material=world_.get_voxel(hit->voxel);
        if(material==0||!world_.set_voxel(hit->voxel,0)) return false;
        if(on_break_) on_break_(hit->voxel,material);
        return true;
    }
    if(!hit->has_previous_air || world_.get_voxel(hit->previous_air)!=0) return false;
    return world_.set_voxel(hit->previous_air,selected_material_);
}
}
