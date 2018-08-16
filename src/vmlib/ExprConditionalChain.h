#ifndef LAKE_EXPRCOND_H
#define LAKE_EXPRCOND_H

#include "ExprExpressionList.h"
#include "VM.h"
#include "Process.h"

namespace lake {

class ExprConditionalChain : public Object
{
public:

    ExprConditionalChain() : Object(TokenType::TypeOperation)
    {
    }

    virtual Object *eval() override
    {
        Object* res = &Object::trueObject();

        rep:

        // If the guard is missing, trueObject is used (such as an unconditional else branch)
        if (guard != nullptr)
        {
            guard->eval();
            res = vm().pop();
        }

        if (res == nullptr || res->otype != TokenType::TypeBool)
            throw std::runtime_error("Expected bool expression in condition");

        if (res->bool_value && body != nullptr)
        {
            // Eval body directly, no need to evaluateBody (which does stack stuff, not relevant here)
            Object* lastExpression = body->eval();

            if (lastExpression == &Object::tailcallRequestObject())
            {
                return lastExpression;
            }
            else if (lastExpression == &Object::repeatObject())
            {
                goto rep;
            }
            else if (lastExpression == &Object::repeatIfTrueObject())
            {
                Object* boolval = vm().pop();
                if (boolval->otype == TokenType::TypeBool && boolval->bool_value)
                    goto rep;
            }
            else if (lastExpression == &Object::repeatIfFalseObject())
            {
                Object* boolval = vm().pop();
                if (boolval->otype == TokenType::TypeBool && !boolval->bool_value)
                    goto rep;
            }
        }
        else if (nextChain != nullptr)
        {
            nextChain->eval();
        }
        else
        {
            trace_debug("cond: no branch");
        }

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        externalize(str, indentation, TOK_IF);
    }

    void externalize(std::ostream &str, int indentation, const char* guardToken) const
    {
        str << std::string(indentation, ' ') << guardToken << std::endl;
        str << std::string(indentation, ' ') << '(' << std::endl;
        if (guard)
            guard->externalize(str, indentation + 4);
        str << std::string(indentation, ' ') << ')' << std::endl;

        str << std::string(indentation, ' ') << '{' << std::endl;
        if (body)
            body->externalize(str, indentation + 4);
        str << std::string(indentation, ' ') << '}' << std::endl;

        if (nextChain)
            nextChain->externalize(str, indentation, TOK_ELSE);
    }

    void mark() override
    {
        if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
            return;

        setFlag(FLAG_GC_REACHABLE);

        if (guard)
            guard->mark();
        if (body)
            body->mark();
        if (nextChain)
            nextChain->mark();
    }

    /**
     * After evaluating this list, an boolean object is expected
     * to be left on the stack. If null, the branch is unconditional.
     */
    ExprExpressionList* guard = nullptr;

    /** If null, there's an empty body */
    ExprExpressionList* body = nullptr;

    ExprConditionalChain* nextChain = nullptr;
};

}//ns

#endif //LAKE_EXPRCOND_H

