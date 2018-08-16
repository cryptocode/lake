#ifndef LAKE_EXPRUNARYOP_H
#define LAKE_EXPRUNARYOP_H

#include "Object.h"
#include "Process.h"
#include "VM.h"
#include <functional>

namespace lake {

class ExprUnaryOp : public Object
{
public:

    ExprUnaryOp(std::function<Object&(Object*)> op, TokenType tok, const char* token, uint8_t flags=0) : Object(tok), op(op), token(token)
    {
        Object::flags = flags;
    }

    virtual Object* eval() override
    {
        Object* first = vm().pop();

        // Don't touch the original as unary methods on Object updates in-place
        Object* result = lake::track(Object::create(*first));

        // Turn op into a callable
        auto unaryop = std::bind(op, result);
        unaryop();

        //result->setDebug("unary result");

        vm().push(result);

        return result;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << token << std::endl;
    }

private:

    const char* token = nullptr;
    const std::function<Object&(Object*)> op;
};

}//ns

#endif //LAKE_EXPRUNARYOP_H
