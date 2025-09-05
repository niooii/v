#include <defs.h>

namespace v {
    /// A Hashmap + Sparse Directed Acyclic Graph structure based on
    /// https://github.com/Phyronnaz/HashDAG
    /// Paper: https://onlinelibrary.wiley.com/doi/full/10.1111/cgf.13916


    template <typename T>
    struct HDAGNode {
        T val;
    };

    template <typename T>
    class HDAG {
    public:
        // rough scaffold for now
        void insert();

    private:
    };
} // namespace v
