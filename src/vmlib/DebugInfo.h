#ifndef LAKE_DEBUGINFO_H
#define LAKE_DEBUGINFO_H

#include "StdUtil.h"

namespace lake {

/**
 * Debug map key. This provides a stable reference into
 * expression lists (vectors which may reallocate the backing array,
 * so we can't reference these by address)
 */
struct StableExprListReference
{
    ptrdiff_t vec;
    ssize_t offset;

    // For unordered_set equality testing
    bool operator==(const StableExprListReference &other) const
    {
        return vec == other.vec && offset == other.offset;
    }
};

/**
 * For now, just Location.
 */
struct DebugInfo
{
    DebugInfo() : loc(Location(0,0,0)) {}
    DebugInfo(Location loc) : loc(loc) {}
    
    Location loc;
};

}//ns

// Specialize hash in the std namespace for StableExprListReference; we
// need this for unordered_set usage.
namespace std
{
    template <> struct hash<lake::StableExprListReference>
    {
        size_t operator()(const lake::StableExprListReference& k) const
        {
            std::size_t seed = 0;
            hash_combine(seed, k.vec);
            hash_combine(seed, k.offset);
            return seed;
        }
    };
}

#endif //LAKE_DEBUGINFO_H
