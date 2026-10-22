-- The network is simulated on the engine's fixed tick.
if not energy.has(1) then energy.add_producer(1, 12) end
if not energy.has(2) then energy.add_consumer(2, 8, 10) end
if not energy.has(3) then energy.add_storage(3, 100, mod.state.energy_storage or 0) end
events.on("on_save", function()
    mod.state.energy_storage = energy.stored(3)
end)
events.on("on_tick", function(tick)
    if tick == "1" then
        log.info("energy network online; initial demand " .. tostring(energy.allocated(2)))
    end
end)
