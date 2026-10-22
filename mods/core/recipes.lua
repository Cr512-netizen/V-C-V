-- Recipes are loaded from data/recipes.json and executed by the fixed-tick machine system.
registry.register("nuc.example.recipe", "crush_ore")
local crusher
if mod.state.crusher and ecs.alive(mod.state.crusher) then
    crusher = mod.state.crusher
else
    crusher = machine.create("crush_ore")
    if crusher ~= 0 then
        machine.insert_input(crusher, 1, 6)
        mod.state.crusher = crusher
    end
end
if crusher ~= 0 then
    log.info("recipe-driven crusher ready")
end
