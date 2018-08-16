#ifndef LAKE_STACK_H
#define LAKE_STACK_H

#include <vector>
#include "Object.h"
#include "VMTypes.h"

namespace lake {

/**
 * Defines the function stack. An instance is created at VM startup for the
 * root function, and new ones are created as needed when calling dup on
 * stacked functions.
 */
class Stack : public Object
{
    std::vector<ssize_t> commits = {0};

public:

    Stack() : Object(TokenType::TypeOperation) {}

    /**
     * Items on this stack, referencing objects
     */
    std::vector<Object*> items;

    void mark() override;

    /**
     * Commit makes a note of the current size of the stack. Any data pushed after this
     * point can be effectively removed by calling revert. This can be used to create
     * a scratch area on the current stack.
     *
     * If you want a result to survive from a scratch computation, push a placeholder
     * and call "store commit" before calling "revert"
     */
    inline void commit()
    {
        this->commits.push_back((ssize_t)items.size());
    }

    /**
     * Returns the stack index of the top item on the last commit. If -1, the stack was empty.
     *
     * @return >= -1
     */
    inline ssize_t commitIndex() const { return commits.back()-1; }

    /**
     * Reverts the size to the last committed size. If commit() has not been called, the
     * entire stack is cleaned since there's an initial implicit commit whenever a stack
     * is created (and thus empty).
     *
     * If the stack size is smaller than it was at the last commit, the current size is kept.
     */
    inline void revert()
    {
        if (items.size() > commits.back())
            resize(commits.back());

        commits.pop_back();
    }

    /**
     * As functions are called and returned from, this maintains the base of the
     * stack frame for the benefit of relative stack addressing.
     *
     * The stack base for the root stack is -1, making relative addressing work
     * in top-level code as well (see ExprLoad for computation details)
     */
    std::vector<int64_t> stackBase {-1};

    inline int64_t getStackBase()
    {
        return stackBase.back();
    }

    inline Object*& at(size_t idx)
    {
        return items.at(idx);
    }

    inline auto size()
    {
        return items.size();
    }

    inline void clear()
    {
        items.clear();
    }

    inline void clearFrame()
    {
        items.resize(stackBase.back()+1);
    }

    inline void resize(size_t n)
    {
        items.resize(n);
    }

    inline auto back()
    {
        if (items.size() == 0)
            throw std::runtime_error("Empty stack");

        return items.back();
    }

    /**
     * Swap top two items on stack
     */
    inline void swap()
    {
        std::iter_swap(items.end()-1, items.end()-2);
    }

    inline void push_back(Object* obj)
    {
        items.push_back(obj);
    }

    inline void pop_back()
    {
        items.pop_back();
    }
};

}//ns

#endif //LAKE_STACK_H
