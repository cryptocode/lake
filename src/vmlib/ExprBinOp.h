#ifndef LAKE_EXPRBINOP_H
#define LAKE_EXPRBINOP_H

#include <sstream>
#include <numeric>
#include "Object.h"
#include "Process.h"
#include "VM.h"
#include "ExprInvoke.h"
#include "ExprColl.h"

namespace lake {

    inline void recursiveIterator(Object* val, std::function<void(Object*)> fn)
    {
        if (val->isArray())
        {
            for (Object* elem : *val->array)
            {
                recursiveIterator(elem, fn);
            }
        }
        else if (val->isUnorderedSet())
        {
            for (Object* elem : *val->uset)
            {
                recursiveIterator(elem, fn);
            }
        }
        else if (val->isProjection())
        {
            auto from = val->projection->collection->array->begin();
            auto to = val->projection->collection->array->end();

            from += val->projection->start;
            to -= val->projection->end;

            for (auto elem=from; elem != to; ++elem)
            {
                recursiveIterator(*elem, fn);
            }
        }
        else
        {
            fn(val);
        }
    }

class ExprAccumulate : public Object
{
public:

    ExprAccumulate() : Object(TokenType::TypeOperation)
    {
    }

    virtual Object* eval() override
    {
        static ExprInvoke invoke(false);

        // First argument is an accumulation function, such as a function doing "mul"
        Object* func = vm().pop();

        // Next is initial value
        Object* initial = vm().pop();

        // Next argument is the count of objects to accumulate. These objects may be
        // arrays (in which case each element is pushed)
        long count = vm().pop()->asLong();

        // This flattens the input, accumulating deeply. Beware of cycles...
        // Make flat-or-not a stack argument?
        std::vector<Object*> input;
        for (long i=0; i < count; i++)
        {
            recursiveIterator(vm().pop(),
                              [&input](Object* obj) { input.push_back(obj); });
        }

        auto res = std::accumulate(input.begin(), input.end(), initial, [&] (Object* res,  Object* elem)
        {
            // The order here is important for non-commutative ops like subtraction.
            vm().push(elem);
            vm().push(res);
            vm().push(func);
            invoke.eval();

            return vm().pop();
        });

        vm().push(res);

        return res;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_ACCUMULATE << "\n";
    }
};

class ExprBinOp : public Object
{
public:

    ExprBinOp(std::function<Object*(Object*, Object*, Object*)> op, TokenType tok, const char* token, bool evaluate=true) : Object(tok), op(op), token(token), evaluate(evaluate)
    {
    }

    virtual Object* eval() override
    {
        // Pop in reverse order. This way, clients can push the first operand first, then the second

        Object* first = vm().pop();
        Object* second = vm().pop();

        // We evaluate after popping both operands, since eval() may push values
        // In some cases, we're asked not to evaluate operands, such as when comparing operand *types*
        if (evaluate)
        {
            second = second->eval();
            first = first->eval();
        }

        // Allow different types only for ptr- and type equality checks
        if (second->otype != first->otype)
        {
            bool _is = *op.target<decltype(&Object::is)>() == &Object::is;
            bool _same = *op.target<decltype(&Object::same)>() == &Object::same;

            if (!_is && !_same)
            {
                if ((first->otype == TokenType::TypeInt || second->otype == TokenType::TypeInt) &&
                    (first->otype == TokenType::TypeInt || second->otype == TokenType::TypeInt))
                {
                    throw std::runtime_error("Missing cast: Attempt to mix int and float.");
                }
                else
                {
                    std::ostringstream s;
                    s << "Binary operations require equal types on stack (use cast) Types: ";
                    s << first->toString() << ", " << second->toString();
                    throw std::runtime_error(s.str().c_str());
                }
            }
        }

        // Turn op into a callable with bind
        auto fun = std::bind(op, first, first, second);
        Object* result = fun();
        vm().push(result);

        return result;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << token << "\n";
    }

private:

    bool evaluate;
    // For externalization
    const char* token = nullptr;
    const std::function<Object*(Object* /*this ptr*/, Object*, Object*)> op;
};

}//ns

#endif //LAKE_EXPRBINOP_H
