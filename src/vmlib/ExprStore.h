#ifndef LAKE_EXPRWRITELOCAL_H
#define LAKE_EXPRWRITELOCAL_H

#include "Object.h"
#include "Process.h"
#include "VM.h"
#include <inttypes.h>

namespace lake {

/**
 * Pops a stack value and writes it to the local variable at the
 * given stack location.
 */
class ExprStore : public Object
{
public:

    ExprStore(int64_t index, TokenType addressingMode=TokenType::Abs, int64_t parentIndex=0)
            : Object(TokenType::TypeOperation), addressingMode(addressingMode), index(index), parentIndex(parentIndex) {}

    virtual Object *eval() override
    {
        int64_t idx = index;

        Stack* stack = vm().stacks.back();

        if (addressingMode == TokenType::Rel)
        {
            // Relative indexing for framed functions (used for parameters)
            // +1 because the assembly input is 0 based in terms of locals (which in frame-relative
            // addressing, the actual index is 1; the last param is at zero, but -1 in assembly to
            // make it mirror the absolute addressing mode)
            idx = stack->getStackBase() + index + 1;
        }
        else if (addressingMode == TokenType::AbsRoot)
        {
            stack = vm().root->fndata->stack;
        }
        else if (addressingMode == TokenType::Local)
        {
            Object* function = vm().pop();
            Object* value = vm().pop();

            if (function->fndata->locals.size() <= index)
                function->fndata->locals.resize(index+1);
            function->fndata->locals[index] = value;
            return nullptr;
        }
        else if (addressingMode == TokenType::Arg)
        {
            Object* function = vm().pop();
            Object* value = vm().pop();

            if (function->fndata->args.size() <= index)
                function->fndata->args.resize(index+1);
            function->fndata->args[index] = value;
            return nullptr;
        }
        else if (addressingMode == TokenType::IntegerLiteral)
        {
            if (index > 0)
                throw std::runtime_error("Top-relative store index must be <= 0");

            // -2 because we have to account for the upcoming pop, and because indexing is 0-based
            // That is, the store instruction pops what is needs to store *before* computing the top-relative address.
            idx = stack->size()-2 + index;
        }
        else if (addressingMode == TokenType::Commit)
        {
            idx = stack->commitIndex();
        }
        // Abs or AbsParent
        else
        {
            // end()-1 is back, which is the result if parentIndex is 0
            // Hence, to navigate to parent, use 1
            stack = *(vm().stacks.end() - (parentIndex+1));
        }

        //trace_debugf("Write stack obj: stack size: %" PRIi64 ", stackIndex: %" PRIi64 ", idx: %" PRIi64,
        //             (int64_t)stack->size(), stackIndex, idx);

        if (idx < 0 || idx >= stack->size())
        {
            throw std::runtime_error("Attempt to write outside stack");
        }

        stack->at((size_t)idx) = vm().pop();

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_STORE << " ";

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
    int64_t index;
    int64_t parentIndex = 0;
    TokenType addressingMode;
};

}//ns

#endif //LAKE_EXPRWRITELOCAL_H
