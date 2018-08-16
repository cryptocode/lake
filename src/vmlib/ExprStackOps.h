#ifndef LAKE_STACKOPS_H
#define LAKE_STACKOPS_H

#include "Object.h"
#include "Process.h"
#include "VM.h"
#include "Stack.h"

namespace lake {

/**
 * Save N arguments to the current function args vector. The number of arguments to save
 * must be pushed on the stack.
 */
class ExprSaveArgs : public Object
{
public:
    ExprSaveArgs() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        // Pop number of arguments to save
        auto count = vm().pop()->asInt();

        // Where to save
        auto& args = vm().current->fndata->args;
        args.clear();

        // Save all the arguments in order (by convention, the first argument should be availble first)
        Stack* stack = vm().stacks.back();
        for (auto i=0; i < count; i++)
        {
            auto idx = stack->getStackBase() - i;

            args.push_back((Object*)stack->at((size_t)idx));
        }

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SAVEARGS << std::endl;
    }
};

/**
 * Saves the current stack size; used to initiate scratch-usage of the stack.
 * Restore the stack size by calling ExprRevertStack.
 */
class ExprCommitStack : public Object
{
public:
    ExprCommitStack() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        vm().stacks.back()->commit();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COMMIT << std::endl;
    }
};

/**
 * Pushes the stack index of the top item on the last commit, -1 if empty or no commit yet
 */
class ExprGetCommitIndex : public Object
{
public:
    ExprGetCommitIndex() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        vm().push(lake::track(Object::create((int64_t)vm().stacks.back()->commitIndex())));
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COMMIT_INDEX << std::endl;
    }
};

class ExprRevertStack : public Object
{
public:
    ExprRevertStack() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        vm().stacks.back()->revert();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_REVERT << std::endl;
    }
};

class ExprReserve : public Object
{
public:

    ExprReserve(size_t count) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        vm().reserve(count);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        if (count > 0)
        {
            str << std::string(indentation, ' ') << TOK_RESERVE << " " << count;
            str << std::endl;
        }
    }

private:
    size_t count = 0;
};

class ExprSetCreator : public Object
{
public:
    ExprSetCreator() : Object(TokenType::TypeOperation) {}

    Object *eval() override
    {
        vm().peek()->fndata->creator = vm().current;
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SETCREATOR << std::endl;
    }
};


/**
 * Pushes the current function on the stack
 */
class ExprCurrentFunction : public Object
{
public:

    ExprCurrentFunction() : Object(TokenType::TypeOperation) {}

    Object *eval() override
    {
        vm().push(vm().current);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_CURRENT << std::endl;
    }
};

/**
 * Pushes a parent function on the stack
 */
class ExprParentFunction : public Object
{
    size_t parentIndex;

public:

    /**
     * C'tor
     *
     * @param parentIndex 0=direct parent, 1=grand parent, etc
     */
    ExprParentFunction(size_t parentIndex=0) : Object(TokenType::TypeOperation), parentIndex(parentIndex)
    {}

    Object *eval() override
    {
        Object* cur = vm().current;

        for (int i=0; i < parentIndex; i++)
        {
            cur = cur->fndata->creator;

            if (cur == nullptr)
                throw std::runtime_error("Invalid parent function reference");
        }

        vm().push(cur);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_PARENT << " " << parentIndex << std::endl;
    }
};

/**
 * Pushes the root function on the stack
 */
class ExprRootFunction : public Object
{
public:

    ExprRootFunction() : Object(TokenType::TypeOperation) {}

    Object *eval() override
    {
        vm().push(vm().root);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_ROOT << std::endl;
    }
};

class ExprDefaultEpsilon : public Object
{
public:
    ExprDefaultEpsilon(Object* epsilon) : Object(TokenType::TypeOperation), epsilon(epsilon)
    {
    }

    Object *eval() override
    {
        mpf_set(vm().epsilon, this->epsilon->mpf);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DEFAULTEPSILON << " ";
        epsilon->externalize(str, indentation);
        str << std::endl;
    }

    void mark() override
    {
        setFlag(FLAG_GC_REACHABLE);
        epsilon->mark();
    }

private:
    Object* epsilon;
};

class ExprDefaultPrecision : public Object
{
public:
    ExprDefaultPrecision(long defaultPrecision) : Object(TokenType::TypeOperation), defaultPrecision(defaultPrecision)
    {
    }

    Object *eval() override
    {
        Object::setDefaultPrecision(defaultPrecision == -1 ? 128 : defaultPrecision);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DEFAULTPRECISION << " " << defaultPrecision << std::endl;
    }

private:
    long defaultPrecision;
};

class ExprHalt : public Object
{
public:

    ExprHalt() : Object(TokenType::TypeOperation), exitCode(0)
    {
        popExitCode = true;
    }

    ExprHalt(int exitCode) : Object(TokenType::TypeOperation), exitCode(exitCode)
    {}

    virtual Object* eval() override
    {
        if (popExitCode)
            exitCode = (int)vm().pop()->asLong();

        trace_debug("HALTING PROCESS");
        exit(exitCode);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_HALT;

        if (!popExitCode)
            str << " " << exitCode;

        str << std::endl;
    }

private:
    bool popExitCode = false;
    int exitCode;
};

/**
 * Force a GC cycle
 */
class ExprGC : public Object
{
public:

    ExprGC() : Object(TokenType::TypeOperation) {}

    virtual Object* eval() override
    {
        vm().gc();

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_GC << std::endl;
    }
};

/**
 * Duplicate (copy) the top item, and keep the original reference on the stack
 */
class ExprDuplicate : public Object
{
public:

    ExprDuplicate() : Object(TokenType::TypeOperation) {}

    virtual Object* eval() override
    {
        Object* top = vm().peek();
        if (top == nullptr)
            throw std::runtime_error("duplicate failed: empty stack");

        Object* result = lake::track(Object::create((const Object&)*(top)));

#ifdef VM_DEBUG
        result->setDebug("duplicate");
#endif

        vm().push(result);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DUPLICATE << std::endl;
    }
};

/**
 * Duplicate (copy) the top item, but pop off the original reference
 */
class ExprCopy : public Object
{
public:

    ExprCopy() : Object(TokenType::TypeOperation) {}

    virtual Object* eval() override
    {
        Object* top = vm().pop();
        Object* result = lake::track(Object::create((const Object&)*(top)));

#ifdef VM_DEBUG
        result->setDebug("copy");
#endif

        vm().push(result);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COPY << std::endl;
    }
};

class ExprLoadStack : public Object
{
public:
    ExprLoadStack() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        Object* obj = vm().pop();

        if (obj->otype == TokenType::TypeFunction && obj->fndata->stack != nullptr)
        {
            vm().stacks.push_back(obj->fndata->stack);
        }
        else
        {
            throw std::runtime_error("No function on stack");
        }

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_LOADSTACK << std::endl;
    }
};

class ExprUnloadStack : public Object
{
public:
    ExprUnloadStack() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        vm().stacks.pop_back();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_UNLOADSTACK << std::endl;
    }
};

class ExprStackClear : public Object
{
public:
    ExprStackClear(bool frameOnly) : Object(TokenType::TypeOperation), frameOnly(frameOnly)  {}

    virtual Object *eval() override
    {
        if (frameOnly)
            vm().stacks.back()->clearFrame();
        else
            vm().stacks.back()->clear();

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_CLEAR;
        if (frameOnly)
            str << " "  << TOK_FRAME;
        str << std::endl;
    }
private:
    bool frameOnly;
};

class ExprStackSize : public Object
{
public:
    ExprStackSize() : Object(TokenType::TypeOperation)  {}

    virtual Object *eval() override
    {
        vm().push(lake::track(Object::create((int64_t)vm().stacks.back()->size())));
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SIZE << std::endl;
    }
};

class ExprCast : public Object
{
public:
    ExprCast(TokenType target) : Object(TokenType::TypeOperation), target(target) { }

    virtual Object* eval() override
    {
        // We can't in-place convert pinned objects; these are generally constants.
        // More importantly, we can't change the original since multiple references may exist.
        Object* val = vm().pop();

        val = lake::track(Object::create(*val));

        val->castTo(target);

        vm().push(val);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_CAST << " " << typestring(target);
        str << std::endl;
    }

private:
    TokenType target = TokenType::Invalid;
};

class ExprLift : public Object
{
public:

    ExprLift(size_t count) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        // If the count is zero, it means that we're asked to pop
        // the number of objects to lift
        if (count == 0)
        {
            Object* val = vm().pop();
            count = (size_t) val->asLong();
        }

        vm().lift(count);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_LIFT << " " << count;
        str << std::endl;
    }

private:
    size_t count = 0;
};


class ExprSink : public Object
{
public:
    ExprSink(size_t count) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        vm().sink(count);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SINK << " " << count;
        str << std::endl;
    }

private:
    size_t count;
};

/**
 * Pops N items from the stack, but keeps the top one
 */
class ExprSquash : public Object
{
public:
    ExprSquash(size_t count) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        vm().squash(count);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SQUASH << " " << count << std::endl;
    }

private:
    size_t count;
};

class ExprSwap : public Object
{
public:
    ExprSwap() : Object(TokenType::TypeOperation) { }

    virtual Object *eval() override
    {
        vm().swap();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_SWAP << std::endl;
    }
};

class ExprPush : public Object
{
    Object* operand = nullptr;

public:

    ExprPush(Object* operand) : Object(TokenType::TypeOperation), operand(operand) { }


    virtual Object* eval() override
    {
        vm().push(operand);

        return operand;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_PUSH << " "; str << operand->typestring() << " ";
        operand->externalize(str, indentation);
        str << std::endl;
    }

    void mark() override
    {
        if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
            return;

        setFlag(FLAG_GC_REACHABLE);
        if (operand)
            operand->mark();
    }
};

class ExprPop : public Object
{
public:
    ExprPop(size_t count=1) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        return vm().pop(count);
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_POP << " " << count;
        str << std::endl;
    }

private:
    size_t count;
};

/**
 * Pops N items from the stack
 */
class ExprRemove : public Object
{
public:
    ExprRemove(size_t count=1) : Object(TokenType::TypeOperation), count(count) { }

    virtual Object *eval() override
    {
        vm().remove(count);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_REMOVE << " " << count;
        str << std::endl;
    }

private:
    size_t count;
};

}//ns

#endif //LAKE_STACKOPS_H
