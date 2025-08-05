keep ck

# Instructions and guide

Guys despite this being a file I actually did NOT vibe code most of this, just dirty boilerplate work :D

## ECS/Domain system

The ECS in this codebase is not used as a traditional ECS. Here are the components you should be aware of:

Engine: The host of an ECS world that creates and stores domains and contexts
Context: A core system that creates & attaches and operates on engine components
Component: A class consisting of some optional data and callbacks that a context calls
Domain: A collection of components, but can also become a context and create components or be a single component and act
as a singleton

The part that really stands out is the Domain, which can actually be all three: an entity, component, and a system. Some
might even argue that at that point it just turns into an object-oriented design, and they’d be right! That’s part of
what makes it powerful and easy to use in practice. And due to the way the engine operates, it still gives us the
benefits of ECS by treating it as a context or component.

A domain is meant to be thought of as a collection of core systems that can be processed independently by default, or
reach into other domains for extended functionality.

## Domains

The destructor of Domains may not access any other Domains/Contexts. To do so, attach a custom on_destroy function to
the engine, which may safely access all Domains/Contexts.

Domains are accessed via the Engine's registry via type pointer (e.g. registry.view<DomainName*>().

# Code Style (from README.md)

## Classes

```c++
class Class {
    // The public interface is the thing people are
    // likely looking at the class definition for
    public:
        // Associated functions are snake_case
        i32 get_some_value();
        // ...
    protected:
        // Members are postfixed with an underscore
        u32 inherited_val_;
        // ...
    private:
        i32 some_val_;
        // ...
};
```

Doxygen comments should be written with triple slashes (///), without trailing periods.  
Your goal is to change as LITTLE as possible, only doing what is requested. This is true for all instances **UNLESS**
you are refactoring systems or doing anything that requires change of usage in other places, then you must seek out and
ensure the entire codebase remains consistent with the new usage.

# Concurrency

The engine is designed with concurrency in mind.  
It is possible to acquire a read or write lock for the engine's domain registry, so read operations can be
done in parallel. However, since Domains are stored as pointers, it is possible to call non-const methods
on queried domains while only having a read lock for the registry.

Because of this, two usages emerge:

1. Acquire write locks when non-const methods need to be called on Domains, and read locks when
   only const methods are needed (the intended use).
2. Always acquire read locks for a particular Domain that is derived from ConcurrentDomain,
   then manually acquire and manage a read or write lock from each ConcurrentDomain
   (fine-grained locking).

It is up to the programmer to manage this (accidental) flexibility, which is now a core feature.  