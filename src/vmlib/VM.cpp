#include "VM.h"
#include "Process.h"
#include "ExprExpressionList.h"
#include "Stack.h"
#include <algorithm>
#include <iterator>

namespace lake {

VM::VM() : heapHead(nullptr), current(nullptr),
           stacks(0),
           heapCountTriggerGC(1024*1024*128), // For stress testing deallocation logic, set to 0
           numObjects(0)
{
    Process::instance().vm = this;

    // The root stacked function
    root = lake::track(
            new Object(
                    new (vm().fnpool.malloc()) FunctionData((Stack*)lake::track(
                            new (vm().stackpool.malloc()) Stack()), "___root")));

    // Let the GC know root has a stack
    root->fndata->withStack = true;
    root->fndata->body = (ExprExpressionList *) lake::track(new ExprExpressionList(root));
    mpf_init2(epsilon, 512);
    mpf_set_d(epsilon, DBL_EPSILON);
}

VM::~VM()
{
}

Object* VM::eval()
{
    current = root;
    return root->fndata->evaluateBody(root);
}

void VM::swap()
{
    stacks.back()->swap();
}

void VM::lift(size_t count)
{
    if (stacks.size() < 2)
        throw std::runtime_error("ERROR: lift instruction requires at least two stacks");

    Stack* prev = *(stacks.end()-2);
    Stack* source = stacks.back();

    //std::cout << "Stack ("<< stacks.size() << " count) sizes: prev: " << prev->size() << ", now: " << now->size() << std::endl;

    auto last = source->size();
    source->resize(last+count);

    for (size_t i=0; i < count; i++)
    {
        Object* obj = prev->back();
        prev->pop_back();

        // This preserves the order
        source->at(last+(count-i-1)) = obj;
    }
}

void VM::sink(size_t count)
{
    if (stacks.size() < 2)
        throw std::runtime_error("ERROR: sink instruction requires at least two stacks");

    // Same as "lift" but with roles reversed
    Stack* target = *(stacks.end()-2);
    Stack* source = stacks.back();

    //std::cout << "Stack ("<< stacks.size() << " count) sizes: prev: " << prev->size() << ", now: " << now->size() << std::endl;

    auto last = target->size();
    target->resize(last+count);

    for (size_t i=0; i < count; i++)
    {
        Object* obj = source->back();
        source->pop_back();

        // This preserves the order
        target->at(last+(count-i-1)) = obj;
    }
}

void VM::reserve(size_t count)
{
    for (size_t i=0; i < count; i++)
    {
        stacks.back()->push_back(&Object::nullObject());
    }
}

void VM::squash(ssize_t count)
{
    Stack* source = stacks.back();
    auto last = source->back();

    if (count == -1)
        source->resize(0);
    else
        // -1 because we want to remove the last item as well...
        source->resize(source->size() - count - 1);

    //... since we push it back here
    source->push_back(last);
}

void VM::remove(size_t count)
{
    Stack* source = stacks.back();

    if (count > source->size())
        throw std::runtime_error("Remove count larger than stack");

    // Call destructors
    for (size_t i=0; i < count; i++)
    {
        Object *v = pop();
        if (v->hasFlag(FLAG_GC_TRACKED))
            v->destruct();
    }
}

void VM::mark()
{
    root->mark();

    // Visit all live stacks
    for (auto& stack : stacks)
        stack->mark();
}

// Should be able to do this in a thread. Only head access needs to be sync'ed
void VM::sweep()
{
    trace_debug("Sweeping...");

    Object* object = heapHead;

    while (object != nullptr)
    {
        Object* curr = object;

        // Free unreachables. Note that we skip pinned objects.
        // A pinned object which is also tracked is typically a function being eval'ed (ExprInvoke pops
        // it from the stack, but it's still alive, of course.)
        if (!curr->hasFlag(FLAG_GC_REACHABLE) && !curr->hasFlag(FLAG_GC_PINNED))
        {
            if (!curr->hasFlag(FLAG_GC_TRACKED))
            {
                throw std::runtime_error("Untracked object found on the sweep list");
            }
            else
            {
                //trace_debugf("GC:COLLECTING: %s", curr->dump().c_str());

                // Deallocates content, puts itself on the free list. If free list full, deallocates itself.
                object = curr->destruct();

                numObjects--;
            }
        }
        else
        {
            trace_debugf("GC:STILL ALIVE: %p", curr);// curr->dump().c_str());

            // Reachable, so unmark in preparation for the next GC cycle
            curr->clearFlag(FLAG_GC_REACHABLE);
            object = object->next;
        }
    }
}

void VM::gc(bool explicitGC)
{
    if (gcActive)
    {
        if (!explicitGC)
            trace_debug("GC TRIGGERED BY LOW SWEEPLIST CAPACITY");

        int numObjectsNow = numObjects;

        mark();
        sweep();

        trace_debugf("Collected %zd objects, %zd remaining.\n", ssize_t(numObjectsNow - numObjects), ssize_t(numObjects));
    }
}

void VM::dumpStack()
{
    std::cout << "==========STACK (" << stacks.size() <<  ") ================================" << std::endl;
    int i=0;
    for (auto& obj : stacks.back()->items)
    {
        if (i == stacks.back()->stackBase.back())
            std::cout << ">   ";
        else
            std::cout << "    ";

        if (Process::instance().traceLevel >= Process::DEBUG)
            std::cout << i << ": " << ((obj) ? obj->dump() : "<null>") << std::endl;
        else
            std::cout << i << ": " << ((obj) ? obj->toString() : "<null>") << std::endl;
        i++;
    }
    std::cout << "-----------------------------------------------" << std::endl;

}

void VM::dumpStackHierarchy(Stack* stack, int indent)
{
    std::cout << std::string(indent, ' ') << "=== STACK (" << stacks.size() <<  ") ===" << std::endl;
    int i=0;
    for (auto& obj : stack->items)
    {
        std::cout << std::string(indent, ' ');

        if (i == stack->stackBase.back())
            std::cout << ">   ";
        else
            std::cout << "    ";

        if (Process::instance().traceLevel >= Process::DEBUG)
            std::cout << i << ": " << ((obj) ? obj->dump() : "<null>") << std::endl;
        else
            std::cout << i << ": " << ((obj) ? obj->toString() : "<null>") << std::endl;
        i++;
    }
    //std::cout << std::string(indent, ' ') << "-----------------------------------------------" << std::endl;
}

void VM::dumpSweepList()
{
    Object* object = heapHead;

    std::cout << "SWEEP LIST: " << std::endl;

    while (object != nullptr)
    {
        std::cout << "> " << object->dump() << std::endl;
        object = object->next;
    }
}

void VM::externalize(std::ostream& str, int indentation)
{
    for (auto& def : defines)
    {
        str << TOK_DEFINE << " " << def.first << " " << def.second->typestring() << " ";
        def.second->externalize(str, 0);
        str << std::endl;
    }

    str << std::endl;

    root->fndata->body->externalize(str, indentation);
}

Object *VM::peek()
{
    Stack* stack = stacks.back();

    if (stack->size() > 0)
        return stack->back();
    else
        return nullptr;
}

Object *VM::pop(size_t count)
{
    Stack* source = stacks.back();

    // The current top object is the result, even if multiple items are popped
    Object* res = source->back();
    ssize_t size = source->size() - count;
    if (size < 0)
        throw std::runtime_error("Invalid stack size");

    source->resize(size);

    return res;
}

void VM::push(Object* value)
{
    stacks.back()->push_back(value);
}

void VM::markStackBase()
{
    Stack* stack = stacks.back();

    stack->stackBase.push_back((int64_t)stack->size()-1);
}

int64_t VM::getStackBase() const
{
    return stacks.back()->getStackBase();//stackBase.back();
}

void VM::restoreStackBase()
{
    stacks.back()->stackBase.pop_back();
}

void VM::setGCActive(bool active)
{
    this->gcActive = active;
}

}//ns
