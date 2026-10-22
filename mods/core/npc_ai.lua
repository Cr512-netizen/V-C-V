local worker
if mod.state.worker and ecs.alive(mod.state.worker) then
    worker = mod.state.worker
else
    worker = npc.spawn(0, 0, 0)
    mod.state.worker = worker
    npc.move_to(worker, 4, 0, 0)
    npc.mine(worker, 1, 3, 2)
end
events.on("on_tick", function(tick)
    if tick == "1" then log.info("NPC worker started") end
end)
