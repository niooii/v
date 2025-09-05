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

# Debugging

If a bug is encountered and a fix is requested, you are to attempt to understand the issue fully. Do not make
assumptions and always test iteratively. You can build the application with `./v.sh build [vclient/vserver/vlib]`, and
run executables with `./v.sh run [vclient/vserver]`. Prefer to pipe the output to `| grep error` with some amount of surrounding
lines, as the output is quite large. If it's a runtime error, you probably want to set a timeout on it,
since it will keep running forever with any intervention. If it's a compilation error, you can test it with build, not
run. If you cannot find out the root of the issue, you may suggest workarounds and quick 'patch fixes' but do not
implement them until requested explicitly. Remove any debug code you wrote to track down the issue after it has been
fixed (eg. logs, prints, etc). Report the findings in a `bug-[name].md` file in the project root, containing what was
wrong, the symptoms, and how it was fixed. Note symptoms are different than the root cause - do not just treat the
symptoms, your goal is to find the root cause.  
