#ifndef LAKE_EXPRREADLOCAL_H
#define LAKE_EXPRREADLOCAL_H

#include "Object.h"
#include "Process.h"
#include "VM.h"
#include "Stack.h"

namespace lake {

/**
 * Read local variable at the given index from the start of the stack/locals-vector,
 * and push it to top of stack.
 */
class ExprLoad : public Object
{
public:

    ExprLoad(int64_t index, TokenType addressingMode=TokenType::Abs, int64_t parentIndex=0)
            : Object(TokenType::TypeOperation),
            index(index), addressingMode(addressingMode), parentIndex(parentIndex) {}

    virtual Object *eval() override
    {
        int64_t idx = index;

        Stack* stack = vm().stacks.back();

        // Relative indexing for framed functions (used for parameters, which are sited
        // before and at the stack base established at invoke-time)
        if (addressingMode == TokenType::Rel)
        {
            // We +1 to make the instruction consistent with absolute addressing. That is, the first
            // local variable is 0 in *assembly*, but is actually 1 (first real param at 0, but -1 in assembly)
            idx = stack->getStackBase() + index + 1;
        }
        else if (addressingMode == TokenType::AbsRoot)
        {
            stack = vm().root->fndata->stack;
        }
        else if (addressingMode == TokenType::Local)
        {
            Object* function = vm().pop();
            vm().push(function->fndata->locals.at(index));
            return nullptr;
        }
        else if (addressingMode == TokenType::Arg)
        {
            Object* function = vm().pop();
            vm().push(function->fndata->args.at(index));
            return nullptr;
        }
        else if (addressingMode == TokenType::Commit)
        {
            idx = stack->commitIndex();
        }
        else if (addressingMode == TokenType::IntegerLiteral)
        {
            if (index > 0)
            {
                throw std::runtime_error("Top-relative load index must be <= 0");
            }

            idx = stack->size()-1 + index;
        }
        // Abs or AbsParent
        else
        {
            // end()-1 is back, which is the result if parentIndex is 0
            stack = *(vm().stacks.end() - (parentIndex+1));
        }

        //trace_debugf("Read stack obj: stack size: %" PRIi64 ", stackIndex: %" PRIi64 ", idx: [%" PRIi64 "]%" PRIi64,
        //        (int64_t)stack->size(), stackIndex, vm().getStackBase(), idx);

        if (idx < 0 || idx >= stack->size())
        {
            throw std::runtime_error("Attempt to read outside stack");
        }

        Object* res = stack->at((size_t)idx);

        //trace_debugf("Read stack obj res: %s", res->toString().c_str());

        vm().push(res);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_LOAD << " ";

        if (addressingMode == TokenType::Abs)
            str << TOK_ABS << " " << index;
        else if (addressingMode == TokenType::AbsRoot)
            str << TOK_ROOT  << " " << index;
        else if (addressingMode == TokenType::Parent)
            str << TOK_PARENT  << " " << index << " " << parentIndex;
        else if (addressingMode == TokenType::Rel)
            str << TOK_REL  << " " << index;
        else if (addressingMode == TokenType::Commit)
            str << TOK_COMMIT;
        else if (addressingMode == TokenType::Local)
            str << TOK_LOCAL  << " " << index;
        else if (addressingMode == TokenType::Arg)
            str << TOK_ARG  << " " << index;
        else if (addressingMode == TokenType::IntegerLiteral)
            str << index;

        str << std::endl;
    }

private:
    int64_t index = 0;
    int64_t parentIndex = 0;
    TokenType addressingMode;
};

}//ns

#endif //LAKE_EXPRREADLOCAL_H
