# TODOs

The net context has an implementation that allows each NetConnection (a domain) to distribute components of the same types (the callback components) to a single entity. 
1. This needs to be tested to see if it actaully works (destroys, everything)
2. Maybe provide an abstraction for this? i see a use case, maybe each ChunkDomain distributes components as well, or enforce a design where the WorldDomain will distribute components and itll just pass in the chunkdomain accordingly. Think on this
