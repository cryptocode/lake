#ifndef LAKE_EXPRASSERTTRUE_H
#define LAKE_EXPRASSERTTRUE_H


#include "Object.h"
#include "VM.h"
#include "Process.h"

namespace lake {

/**
 * Pop value from stack and check if it's true. If not, throw a runtime error.
 */
class ExprAssertTrue : public Object
{
public:

    ExprAssertTrue(const std::string err) : Object(TokenType::TypeOperation), err(err) { }

    virtual Object* eval() override
    {
        Object* val = vm().pop();

        if (val == nullptr || val->otype != TokenType::TypeBool || !val->bool_value)
            throw std::runtime_error(err);

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_ASSERTTRUE << " \"" << err << '"' << "\n";
    }

private:
    std::string err;
};

}//ns

#endif //LAKE_EXPRASSERTTRUE_H
