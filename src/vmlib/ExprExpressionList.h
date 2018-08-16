#ifndef LAKE_BODY_H
#define LAKE_BODY_H

#include <vector>

#include "Object.h"
#include "Exceptions.h"
#include "ExprInvoke.h"
#include "DebugInfo.h"


namespace lake {

/**
 * A list of expressions. When evaluated, all expressions in the list
 * are evaluated in order.
 */
class ExprExpressionList : public Object {
public:

    ssize_t errorLabelIndex = -1;
    ssize_t prependCount = 0;

    ExprExpressionList(Object* _owner = nullptr) : Object(TokenType::TypeOperation), owner(_owner)
    {
    }

    void mark() override
    {
        if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
            return;

        setFlag(FLAG_GC_REACHABLE);

        for (auto& expr : expressions)
        {
            expr->mark();
        }

        if (owner != nullptr)
            owner->mark();
    }

    // TODO: need to be able to externalize expressionlist objects,
    // like "expr { push int 2; push int 3; add }", which can be evaluated as an object

    void externalize(std::ostream& str, int indentation=0) const override
    {
        for (auto& expr : expressions)
        {
            expr->externalize(str, indentation);
        }
    }

    inline void clear()
    {
        expressions.clear();
        errorLabelIndex = -1;
        prependCount = 0;
    }

    void addExpression(Object* obj)
    {
        addExpression(obj, DebugInfo(Location(0,0,0)));
    }

    void addExpression(Object* obj, DebugInfo di)
    {
        if (obj == &Object::errorLabelObject())
            errorLabelIndex = expressions.size();

        expressions.push_back(obj);

        if (Process::instance().debugInfo)
        {
            Process::instance().debugMap.emplace(
                    StableExprListReference {(ptrdiff_t) &expressions, (ssize_t)expressions.size() - prependCount - 1},
                    di);
        }
    }

    void prependExpression(Object* obj)
    {
        prependExpression(obj, DebugInfo(Location(0,0,0)));
    }

    void prependExpression(Object* obj, DebugInfo di)
    {
        if (obj == &Object::errorLabelObject())
            errorLabelIndex = 0;

        expressions.insert(expressions.begin()+prependCount, obj);

        if (Process::instance().debugInfo)
        {
            Process::instance().debugMap.emplace(
                    StableExprListReference {(ptrdiff_t) &expressions, (-1)-prependCount},
                    di);
        }

        prependCount++;
    }

    inline bool isEmpty()
    {
        return expressions.empty();
    }

    /**
     * Evaluates each expression in the list
     */
    virtual Object* eval() override
    {
        Object* res = nullptr;

        std::vector<Object*>* exprlist = &expressions;

    restart:

        for (size_t idx = 0; idx < exprlist->size ();)
        {
            try
            {
                auto& expr = exprlist->at(idx);
                res = expr->eval();

                if (res == &Object::tailcallRequestObject())
                {
                    if (owner != nullptr && owner->otype != TokenType::TypeFunction)
                        return res;

                    // Grab the expression list from the target tail function and start over
                    exprlist = &vm().tailcallRequest->fndata->body->expressions;
                    vm().tailcallRequest = nullptr;
                    goto restart;
                }
                else if (res == &Object::raiseRequestObject())
                {
                    // Do we have an error label? If not, just return res and
                    // let the parent expression list deal with it.
                    if (errorLabelIndex >= 0)
                    {
                        idx = errorLabelIndex;
                        continue;
                    }
                    else
                        break;
                }
                else if (res == &Object::errorLabelObject())
                {
                    // Just trace and skip it
                    trace_debug("Resuming at error label");
                    idx++;
                    continue;
                }
                else if (res == &Object::exitRequestObject())
                {
                    return res;
                }
            }
            // If we don't catch and rethrow EvalException here, we get the equivalent of a stack-trace since
            // function calls are nested expressions lists here.
            catch (EvalException& evalEx)
            {
                throw evalEx;
            }
            catch (const std::exception& e)
            {
                if (idx < prependCount)
                    idx = -idx;
                else
                    idx = idx - prependCount;

                const auto& match = Process::instance().debugMap.find(
                        StableExprListReference {(ptrdiff_t) &expressions, (ssize_t)(idx)});

                if (match != Process::instance().debugMap.end())
                {
                    const auto& di = match->second;

                    // Rethrow as exception with debug info.
                    // This is the only place where an EvalException is created.
                    throw EvalException(e.what(), di);
                }

                throw;
            }

            if (res == &Object::exitScopeObject())
                break;

            idx++;
        }

        // Test with VM#heapCountTriggerGC(0) regularly to stress GC logic
        vm().gcIfNeeded();

        // We return the last evaluated expression in the list.
        // This allows expressions lists to contain sentinels, such as
        // exitScopeObject, repeatObject, etc
        return res;
    }

protected:
    std::vector<Object*> expressions;
    Object* owner;
};

class ExprExpressionListObject : public Object
{
public:

    ExprExpressionListObject() : Object(TokenType::TypeExprListObject)
    {
    }

    void mark() override
    {
        if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
            return;

        setFlag(FLAG_GC_REACHABLE);

        expressionList.mark();
    }

    Object* eval() override
    {
        return expressionList.eval();
    }

    // TODO: must parse this
    void externalize(std::ostream& str, int indentation=0) const override
    {
        str << std::string(indentation, ' ') << "{ " << std::endl;
        expressionList.externalize(str, indentation+4);
        str << std::string(indentation, ' ') << "}" << std::endl;
    }

    auto getExpressions() {
        return &expressionList;
    }

    ExprExpressionList expressionList;
};

}//ns

#endif //LAKE_BODY_H
