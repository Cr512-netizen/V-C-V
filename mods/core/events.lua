events.on("on_tick", function(payload)
    -- Tick payload is a stable string until structured Lua payloads arrive.
end)
events.on("on_voxel_break", function(payload)
    log.info("voxel break: " .. payload)
end)
