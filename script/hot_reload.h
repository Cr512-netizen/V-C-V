#pragma once
#include "script/mod_loader.h"
#include <string>
namespace nuc::script {
class HotReload {
public:
 bool reload(ModLoader& loader,const std::string& mod_id,std::string* error=nullptr) const;
};
}
