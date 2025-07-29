# Instructions and guide

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
