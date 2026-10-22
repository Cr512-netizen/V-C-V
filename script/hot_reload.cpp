#include "script/hot_reload.h"
namespace nuc::script {
bool HotReload::reload(ModLoader& loader,const std::string& mod_id,std::string* error) const {
 std::string message;
 const bool ok=loader.reload(mod_id,message);
 if(!ok&&message.empty()) message="hot reload failed without a diagnostic for mod '"+mod_id+"'";
 if(error) *error=message;
 return ok;
}
}
