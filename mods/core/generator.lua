-- Resource and biome rules are loaded from data/biomes.json by ResourceGenerator.
registry.register("nuc.generator.resource_api", "1")
events.on("on_chunk_load", function(payload)
    log.info("chunk-load event: " .. payload)
end)
