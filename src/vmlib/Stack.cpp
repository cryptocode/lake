#include "Stack.h"
#include "Object.h"

namespace lake
{
    void Stack::mark()
    {
        if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
            return;

        setFlag(FLAG_GC_REACHABLE);

        for (auto& obj : items)
        {
            obj->mark();
        }
    }
}
