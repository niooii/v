# TODOs

### Net

The net context has an implementation that allows each NetConnection (a domain) to distribute components of the same types (the callback components) to a single entity. 
1. This needs to be tested to see if it actaully works (destroys, everything)
2. Maybe provide an abstraction for this? i see a use case, maybe each ChunkDomain distributes components as well, or enforce a design where the WorldDomain will distribute components and itll just pass in the chunkdomain accordingly. Think on this

### Async
Plans:
```cpp
Future<T> future = engine.get_ctx<AsyncContext>().task([]{ return T(); });
// Do some work
// ...

// This will be immediately queued to the engine's thread if the future has completed, or 
// it will be queued whenever the future completes.
future.then([](T result) { LOG_INFO("{}", T); });

// Same w this
future.or_else([](std::exception_ptr e) { ... });

// This means that the functions passed into ::then() must 
// somehow be queued when the future completes (i dont want a polling approach)
// or it would be queued instantly.
```
