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
