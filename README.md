# thing

yay

# Code Style

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

Doxygen comments should be written with triple slashes (///), without trailing periods except on multi-sentence
comments.

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