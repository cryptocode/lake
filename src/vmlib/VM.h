#ifndef LAKE_VM_H
#define LAKE_VM_H

#include <vector>
#include <stack>
#include <mutex>
#include <map>
#include <assert.h>
#include <mpir.h>
#include <boost/pool/object_pool.hpp>

namespace lake {

class Stack;
class Object;
class FunctionData;
class ExprFunction;

/**
 * A VM is a lightweight object representing a thread of execution, with a stack and a
 * garbage collector. Multiple VM objects may be created, each running in isolation from
 * each other in a separate OS thread. VM's can only communicate via messages.
 */
class VM
{
public:

    VM();

    VM(VM&) = delete;
    VM& operator=(const VM&) = delete;

    virtual ~VM();

    /**
     * The epsilon for comparing floats can be changed per VM
     */
    mpf_t epsilon;

    /**
     * Currently executing function; used by ExprCurrent.
     */
    Object* current = nullptr;
    
    /**
     * Starts evaluation of the root closure
     */
    virtual Object* eval();
    virtual void externalize(std::ostream& str, int indentation=0);

    std::map<std::string, Object*> defines;

    // Stack of stacks. A stack is created at VM creation. The stack-of-stacks is needed when
    // calling closure constructor functions (functions with their own stack) so that they can
    // capture objects on parent stack (parameters, parent-locals, parent-parent-locals, etc)
    std::vector<Stack*> stacks;

    // The root function for this VM. The is the one and only GC root. The GC follows the
    // stack objects on this function.
    Object* root = nullptr;

    // First Object on GC mark-list. This are all objects that are reachable and may be
    // unreachable. Objects deemed unreachable are moved to the freelist for reuse.
    Object* heapHead = nullptr;

    // When objects are garbage collected, they're put on the free list. If the free list
    // if full (given a configurable size using a vm instruction), the object is actually
    // deallocated. Note that a destructor is still called, even if merely placed on the
    // free list.
    //
    // The freelist improves allocation speed and counters heap fragmentation (since we'll
    // request heap allocations with increasing rarity.)
    //
    // freeHead points to the first object on freelist. The next/prev pointers of a
    // deallocated object are repurposed to do freelist node chaining.
    //
    // When grabbing objects from the free-list, placement-new must be used
    // (what about FunctionData? keep the emptied stack around!)

    /* The total number of currently allocated objects. */
    int64_t numObjects;

    /* The number of objects required to trigger a GC. */
    int64_t heapCountTriggerGC;

    bool gcActive = true;

    /**
     * When this is set, an "invoke tail" is requested, at which point currently
     * evaluated expression lists return with a tailcall sentinel, all the way down
     * to the function expression list, which now switches lists and starts evaluating
     * the the body of tailcallRequest. This means no interpreter stack is need for
     * tail calls, allowing the implementation of loop-as-recursion, co-routines, CPS,
     * and so on.
     */
    Object* tailcallRequest = nullptr;

    // Set to stack-top when invoking so that params can be read. Because params
    // may be read after invocation of other functions (possible recursively), we
    // need to maintain a stack of stack-bases (which is pop'ed after the invoke)
    void markStackBase();

    int64_t getStackBase() const;

    void restoreStackBase();

    inline void gcIfNeeded()
    {
        if (numObjects+1 >= heapCountTriggerGC)
        {
            gc(false);
        }
    }

    void mark();
    void sweep();
    void gc(bool explicitGC=true);
    void setGCActive(bool active);

    /**
     * Push object to the current stack
     */
    void push(Object* value);

    /**
     * Pops one or more objects off the stack, without destructing the objects.
     */
    Object* pop(size_t count=1);

    /**
     * Returns the top of stack without popping it. If the stack is empty,
     * nullptr is returned.
     */
    Object* peek();

    /**
     * Swap top two items
     */
    void swap();

    void lift(size_t count);
    void sink(size_t count);

    /**
     * Remove the N items just from before the last item (that is,
     * the top item is retained, N items below it is removed)
     *
     * If N is -1, the entire stack is squashed, except the top item
     */
    void squash(ssize_t count);

    /**
     * Removed top N items
     */
    void remove(size_t count);

    /**
     * Reserve N items on the stack. These will contain nullObject's
     * @param count Number of items to reserve
     */
    void reserve(size_t count);


    void dumpStack();
    void dumpStackHierarchy(Stack* stack, int indentation);
    void dumpSweepList();

    boost::object_pool<Object> pool;
    boost::object_pool<FunctionData> fnpool;
    boost::object_pool<Stack> stackpool;
};

}//ns

#endif //LAKE_VM_H
