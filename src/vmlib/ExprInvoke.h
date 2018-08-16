#ifndef LAKE_EXPRINVOKE_H
#define LAKE_EXPRINVOKE_H

#include "Object.h"
#include "VM.h"

namespace lake {

/**
 * Evaluates the object on the stack. If the object is a function object,
 * the body is evaluated.
 */
class ExprInvoke : public Object
{
public:

    ExprInvoke(bool tail = false) : Object(TokenType::TypeOperation), tail(tail) { }

    virtual Object* eval() override
    {
        //trace_debug("INVOKE::eval:stack=");
        //trace_stack();

        // Pop the object we're going to evaluate
        Object* evalObject = vm().pop();
        Object* res = nullptr;

        if (evalObject->otype == TokenType::TypeFunction)
        {

            // Pass a tailcall sentinel down to the function (we may be inside
            // nested expression lists, nested cond for instance). This is how we
            // we *don't* blow up the native interpreter stack.
            if (tail)
            {
                vm().tailcallRequest = evalObject;
                res = &Object::tailcallRequestObject();
            }
            else
            {
                // This may return a sentinel as well (raise)
                res =  evalObject->fndata->evaluateBody(evalObject);
            }
        }
        else
            res = evalObject->eval();

        return res;

        //trace_debug("INVOKE completed. Stack=");
        //trace_stack();
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_INVOKE;
        if (tail) str << " " << TOK_TAIL;
        str << std::endl;
    }

private:
    bool tail = false;
};

}//ns

#endif //LAKE_EXPRINVOKE_H
