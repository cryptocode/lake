#ifndef LAKE_EXPRDUMP_H
#define LAKE_EXPRDUMP_H

#include "Object.h"
#include "Process.h"
#include "VM.h"

namespace lake {

/**
 * Dumps the operand. If the operand is nullptr, the top of the stack is dumped (but not popped)
 */
class ExprDump : public Object {

public:
    ExprDump() : Object(TokenType::TypeOperation) { }
    ExprDump(Object* operand) : Object(TokenType::TypeOperation), operand(operand) { }

    Object* operand = nullptr;

    virtual Object *eval() override
    {
        Object* val = operand;

        if (val == nullptr)
            val = vm().peek();

        if (Process::instance().traceLevel >= Process::DEBUG)
        {trace_debugf("%s", val == nullptr ? "<null>" : val->dump().c_str());}
        else
        {trace_infof("%s", val == nullptr ? "<null>" : val->toString().c_str());}

        return this;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DUMP;
        if (operand)
        {
            str << " " << operand->typestring() << " ";
            operand->externalize(str, 0);
        }
        str << "\n";
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

class ExprDumpStack : public Object
{
public:
    ExprDumpStack() : Object(TokenType::TypeOperation) { }

    virtual Object *eval() override
    {
        vm().dumpStack();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DUMP << " " TOK_STACK << "\n";
    }
};

class ExprDumpStackHierarchy : public Object
{
public:
    ExprDumpStackHierarchy() : Object(TokenType::TypeOperation) { }

    virtual Object *eval() override
    {
        vm().dumpStackHierarchy(vm().stacks.back(), 0);
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_DUMP << " " << TOK_STACKHIERARCHY << "\n";
    }
};

class ExprDumpSweepList : public Object
{
public:
    ExprDumpSweepList() : Object(TokenType::TypeOperation) { }

    virtual Object *eval() override
    {
        vm().dumpSweepList();
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') <<  TOK_DUMP << " " << TOK_SWEEPLIST << "\n";
    }
};

class ExprDumpFreeList : public Object
{
public:
    ExprDumpFreeList() : Object(TokenType::TypeOperation) { }

    virtual Object *eval() override
    {
        /* no-op */
        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') <<  TOK_DUMP << " " << TOK_FREELIST << "\n";
    }
};

}//ns

#endif //LAKE_EXPRDUMP_H
