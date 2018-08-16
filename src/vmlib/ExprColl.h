#ifndef GC_EXPRCOLL_H
#define GC_EXPRCOLL_H

#include "Object.h"
#include "ExprExpressionList.h"

namespace lake
{
enum class IndexType : uint8_t
{
    Parameterized,
    Append,
    Insert
};

/**
 * Creates a new unordered map on the stack
 */
class ExprCollPushMap : public Object
{
public:
    ExprCollPushMap(size_t initialSize=0) : Object(TokenType::TypeOperation), initialSize(initialSize)
    {
    }

    virtual Object *eval() override
    {
        auto map = new std::unordered_map<Object*, Object*>();
        if (initialSize > 1)
            map->reserve((size_t) initialSize);

        // Maps are not literals and thus not pinned
        auto val = Object::create(map);
        vm().push(val);

        return val;
    }

private:
    size_t initialSize;
};

/**
 * Creates an array on the stack
 */
class ExprCollPushArray : public Object
{
public:
    ExprCollPushArray(size_t initialSize=0) : Object(TokenType::TypeOperation), initialSize(initialSize)
    {
    }

    virtual Object *eval() override
    {
        auto arr = new std::vector<Object*>();
        if (initialSize > 1)
            arr->reserve((size_t) initialSize);

        // Maps are not literals and thus not pinned
        auto val = Object::create(arr);
        vm().push(val);

        return val;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_PUSH << " " << TOK_TYPEARRAY << " " << initialSize << "\n";
    }

private:
    size_t initialSize;
};

/**
 * Implements coll-project(coll, start, end) where start/end are zero-based indices from start
 * and beginning of a collection.
 */
class ExprCollProjection : public Object
{
public:

    ExprCollProjection() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        // The collection we're projecting
        Object* coll = vm().pop();
        if (coll->otype != TokenType::TypeArray)
            throw std::runtime_error("coll projection only supports arrays for now");

        // Zero-based start
        Object* start = vm().pop();

        // Zero-based end. 0=the end, 1=except the last one, etc
        Object* end = vm().pop();

        ProjectionData* projdata = new ProjectionData();
        projdata->collection = coll;
        projdata->start = start->asLong();
        projdata->end = end->asLong();

        Object* projection = Object::create(projdata);

        vm().push(lake::track(projection));

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_COLLPROJECTION << "\n";
    }
};

class ExprCollContains : public Object
{
public:

    ExprCollContains() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        Object* arr = vm().pop();
        Object* val = vm().pop();

        if (arr->otype == TokenType::TypePair)
        {
            if (std::equal_to<Object*>()(arr->pair->first, val))
                vm().push(&Object::trueObject());
            else
                vm().push(&Object::falseObject());
        }
        else if (arr->otype == TokenType::TypeUnorderedMap)
        {
            if (arr->umap->find(val) != arr->umap->end())
                vm().push(&Object::trueObject());
            else
                vm().push(&Object::falseObject());
        }
        else if (arr->otype == TokenType::TypeUnorderedSet)
        {
            if (arr->uset->find(val) != arr->uset->end())
                vm().push(&Object::trueObject());
            else
                vm().push(&Object::falseObject());
        }
        else if (arr->otype == TokenType::TypeArray)
        {
            for (const Object* obj : *arr->array)
            {
                if (std::equal_to<Object*>()(obj, val))
                {
                    vm().push(&Object::trueObject());
                    return nullptr;
                }
            }

            vm().push(&Object::falseObject());
        }
        else if (arr->otype == TokenType::TypeString)
        {
            for (char ch : *arr->str_value)
            {
                if (ch == val->char_value)
                {
                    vm().push(&Object::trueObject());
                    return nullptr;
                }
            }

            vm().push(&Object::falseObject());
        }
        else throw std::runtime_error("contains expected a collection type on the stack");

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_COLLCONTAINS << "\n";
    }
};

class ExprCollPut : public Object
{
public:
    ExprCollPut(IndexType it/* = IndexType::Parameterized*/) : Object(TokenType::TypeOperation), indexType(it)
    { }

    virtual Object* eval() override
    {
        Object* arr = vm().pop();
        Object* val = vm().pop();

        if (arr->otype == TokenType::TypePair)
        {
            Object* first = vm().pop();
            arr->pair->first = first;
            arr->pair->second = val;
        }
        else if (arr->otype == TokenType::TypeArray)
        {
            long idx = indexType == IndexType::Append ? -1 : 0;
            if (indexType == IndexType::Parameterized)
                idx = vm().pop()->asLong();

            if (indexType == IndexType::Insert)
                arr->array->insert(arr->array->begin(), val);
            else if (idx != -1)
            {
                if (idx >= arr->array->size())
                    throw std::runtime_error("put offset is out of range");

                arr->array->at(idx) = val;
            }
            else
                arr->array->push_back(val);
        }
        else if (arr->otype == TokenType::TypeString)
        {
            long idx = indexType == IndexType::Append ? -1 : 0;
            if (indexType == IndexType::Parameterized)
                idx = vm().pop()->asLong();

            // Insert at beginning
            if (indexType == IndexType::Insert)
            {
                if (val->otype == TokenType::TypeChar)
                    arr->str_value->insert(arr->str_value->cbegin(), val->char_value);
                else if (val->otype == TokenType::TypeString)
                    arr->str_value->insert(0, *val->str_value);
                else
                    throw std::runtime_error("can only insert string and char values to strings");
            }
            else if (idx != -1)
            {
                if (val->otype == TokenType::TypeChar)
                    arr->str_value->at(idx) = val->char_value;
                else if (val->otype == TokenType::TypeString)
                    arr->str_value->insert(idx, *val->str_value);
                else
                    throw std::runtime_error("can only insert string and char values to strings");
            }
            else
            {
                if (val->otype == TokenType::TypeChar)
                    arr->str_value->append(1, val->char_value);
                else if (val->otype == TokenType::TypeString)
                    arr->str_value->append(*val->str_value);
                else
                    throw std::runtime_error("can only append string and char values to strings");

            }
        }
        else if (arr->otype == TokenType::TypeUnorderedMap)
        {
            Object* key = vm().pop();

            (*arr->umap)[key] = val;
        }
        else if (arr->otype == TokenType::TypeUnorderedSet)
        {
            arr->uset->insert(val);
        }
        else throw std::runtime_error("put expected a collection type on the stack");

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " ";

        if (indexType == IndexType::Parameterized)
            str << TOK_COLLPUT;
        else if (indexType == IndexType::Append)
            str << TOK_COLLAPPEND;
        else if (indexType == IndexType::Insert)
            str << TOK_COLLINSERT;
        else
            throw std::runtime_error("Invalid index type");

        str << std::endl;
    }

private:
    IndexType indexType;
};

/**
 * Get ptr-to-item in collection. For arrays and strings, an index is expected on the stack. If
 * the index is -1, the last element is returned.
 *
 * For maps and sets, the key is expected on the stack.
 */
class ExprCollGet : public Object
{
public:

    ExprCollGet(IndexType it=IndexType::Parameterized) : Object(TokenType::TypeOperation), indexType(it)
    { }

    virtual Object* eval() override
    {
        Object* arr = vm().pop();
        get(arr);
        return nullptr;
    }

    void get(Object* arr, ProjectionData* projection=nullptr)
    {
        if (arr->otype == TokenType::TypePair)
        {
            long idx = indexType == IndexType::Append ? 1 : 0;
            if (indexType == IndexType::Parameterized)
                idx = vm().pop()->asLong();

            if (idx == 0)
                vm().push(arr->pair->first);
            else
                vm().push(arr->pair->second);
        }
        else if (arr->otype == TokenType::TypeArray)
        {
            long idx = indexType == IndexType::Append ? -1 : 0;
            if (indexType == IndexType::Parameterized)
                idx = vm().pop()->asLong();

            if (idx != -1)
            {
                if (projection)
                    idx += projection->start;

                vm().push(arr->array->at(idx));
            }
            else
                vm().push(arr->array->back());
        }
        else if (arr->otype == TokenType::TypeProjection)
        {
            get(arr->projection->collection, arr->projection);
        }
        else if (arr->otype == TokenType::TypeString)
        {
            char ch = 0;

            long idx = indexType == IndexType::Append ? -1 : 0;
            if (indexType == IndexType::Parameterized)
                idx = vm().pop()->asLong();

            if (idx != -1)
                ch = arr->str_value->at(idx);
            else
                ch = arr->str_value->back();

            vm().push(lake::track(Object::create(ch)));
        }
        else if (arr->otype == TokenType::TypeUnorderedMap)
        {
            Object* key = vm().pop();

            const auto& val = arr->umap->find(key);
            if (val == arr->umap->end())
                vm().push(&Object::nullObject<TokenType::TypeObject>());
            else
                vm().push(val->second);
        }
        else if (arr->otype == TokenType::TypeUnorderedSet)
        {
            Object* key = vm().pop();
            const auto& itr = arr->uset->find(key);
            if (itr == arr->uset->end())
                vm().push(&Object::nullObject<TokenType::TypeObject>());
            else
                vm().push(*itr);
        }
        else throw std::runtime_error("get expected a collection type on the stack");
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_COLLGET << std::endl;
    }

private:
    IndexType indexType;
};

class ExprCollDel : public Object
{
public:
    ExprCollDel() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        Object* coll = vm().pop();

        if (coll->otype == TokenType::TypePair)
        {
            long idx = vm().pop()->asLong();
            if (idx == 0)
                coll->pair->first = nullptr;
            else
                coll->pair->second = nullptr;
        }
        else if (coll->otype == TokenType::TypeArray)
        {
            long idx = vm().pop()->asLong();
            if (idx != -1)
                coll->array->erase(coll->array->begin() + idx);
            else
                coll->array->pop_back();
        }
        else if (coll->otype == TokenType::TypeString)
        {
            long idx = vm().pop()->asLong();
            if (idx != -1)
                coll->str_value->erase((size_t)idx, 1);
            else if (coll->str_value->length() > 0)
                coll->str_value->erase(coll->str_value->length()-1, 1);
        }
        else if (coll->otype == TokenType::TypeUnorderedMap)
        {
            Object* key = vm().pop();

            const auto& val = coll->umap->find(key);
            if (val != coll->umap->end())
            {
                coll->umap->erase(val);
            }
        }
        else if (coll->otype == TokenType::TypeUnorderedSet)
        {
            Object* key = vm().pop();

            const auto& val = coll->uset->find(key);
            if (val != coll->uset->end())
            {
                coll->uset->erase(val);
            }
        }
        else throw std::runtime_error("del expected a collection type on the stack");

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_COLLDEL << std::endl;
    }
};
#include <iterator>
class ExprCollReverse: public Object
{
public:
    ExprCollReverse() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        Object* coll = vm().pop();
        if (coll->otype == TokenType::TypeArray)
        {
            std::reverse(coll->array->begin(), coll->array->end());
        }
        else if (coll->otype == TokenType::TypeString)
        {
            std::reverse(coll->str_value->begin(), coll->str_value->end());
        }
        else 
        {
            throw std::runtime_error("reverse expected array type on stack");
        }

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_COLLREVERSE << std::endl;
    }
};

class ExprCollForeach : public Object
{
public:
    ExprCollForeach() : Object(TokenType::TypeOperation)
    { }

    void mark() override
    {
        setFlag(FLAG_GC_REACHABLE);
        exprlist->mark();
    }

    virtual Object* eval() override
    {
        Object* coll = vm().pop();
        iterate(coll);
        return nullptr;
    }

    void iterate(Object* coll, ssize_t start=0, ssize_t end=-1)
    {
        if (coll->otype == TokenType::TypeUnorderedMap)
        {
            for (const auto& entry : *coll->umap)
            {
                vm().push(entry.first);
                vm().push(entry.second);

                exprlist->eval();
            }
        }
        else if (coll->otype == TokenType::TypeUnorderedSet)
        {
            for (const auto& entry : *coll->uset)
            {
                vm().push(entry);

                exprlist->eval();
            }
        }
        else if (coll->otype == TokenType::TypePair)
        {
            vm().push(coll->pair->first);
            exprlist->eval();
            vm().push(coll->pair->second);
            exprlist->eval();
        }
        else if (coll->otype == TokenType::TypeArray)
        {
            auto from = coll->array->begin();
            auto to = coll->array->end();

            if (start != 0)
                from += start;
            if (end != -1)
                to -= end;

            for (auto entry=from; entry != to; ++entry)
            {
                vm().push(*entry);
                exprlist->eval();
            }
        }
        else if (coll->otype == TokenType::TypeProjection)
        {
            iterate(coll->projection->collection, coll->projection->start, coll->projection->end);
        }
        else if (coll->otype == TokenType::TypeString)
        {
            for (char ch : *coll->str_value)
            {
                vm().push(lake::track(Object::create(ch)));

                exprlist->eval();
            }
        }
        else throw std::runtime_error("foreach expected collection type on stack");
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLLFOREACH << std::endl;
        str << std::string(indentation, ' ') << "{" << std::endl;
        exprlist->externalize(str, indentation+4);
        str << std::string(indentation, ' ') << "}" << std::endl;
    }
};

class ExprCollSize : public Object
{
public:
    ExprCollSize() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        Object* coll = vm().pop();
        pushSize(coll);
        return nullptr;
    }

    void pushSize(Object* coll)
    {
        int64_t size = 0;

        if (coll->otype == TokenType::TypeArray)
            size = (int64_t) coll->array->size();
        else if (coll->otype == TokenType::TypeProjection)
        {
            pushSize(coll->projection->collection);
            return;
        }
        else if (coll->otype == TokenType::TypePair)
            size = 2;
        else if (coll->otype == TokenType::TypeUnorderedMap)
            size = (int64_t) coll->umap->size();
        else if (coll->otype == TokenType::TypeUnorderedSet)
            size = (int64_t) coll->uset->size();
        else if (coll->otype == TokenType::TypeString)
            size = (int64_t) coll->str_value->size();
        else throw std::runtime_error("size expected collection type on stack");

        vm().push(lake::track(Object::create(size)));
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_SIZE << std::endl;
    }
};

class ExprCollClear : public Object
{
public:
    ExprCollClear() : Object(TokenType::TypeOperation)
    { }

    virtual Object* eval() override
    {
        Object* coll = vm().pop();

        if (coll->otype == TokenType::TypeArray)
            coll->array->clear();
        else if (coll->otype == TokenType::TypePair)
        {   coll->pair->first = nullptr; coll->pair->second = nullptr; }
        else if (coll->otype == TokenType::TypeUnorderedMap)
            coll->umap->clear();
        else if (coll->otype == TokenType::TypeUnorderedSet)
            coll->uset->clear();
        else if (coll->otype == TokenType::TypeString)
            coll->str_value->clear();
        else throw std::runtime_error("clear expected collection type on stack");

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << TOK_CLEAR << std::endl;
    }
};

/**
 * Pops a collection off the stack, pushes all the elements in order, or reverse order
 */
class ExprCollSpread : public Object
{
public:

    ExprCollSpread(bool reverse) : Object(TokenType::TypeOperation), reverse(reverse)
    { }

    virtual Object* eval() override
    {
        Object* arr = vm().pop();

        if (arr->otype == TokenType::TypeArray)
        {
            if (reverse)
            {
                for (auto it = arr->array->rbegin(); it != arr->array->rend(); ++it)
                {
                    vm().push(*it);
                }
            }
            else
            {
                for (auto& v : *arr->array)
                {
                    vm().push(v);
                }
            }
        }
        else if (arr->otype == TokenType::TypeString)
        {
            for (char& ch : *arr->str_value)
                vm().push(lake::track(Object::create(ch)));
        }
        else if (arr->otype == TokenType::TypeUnorderedMap)
        {
            // Since umap is unordered, reversing makes no sense, so ignore the flag
            for (auto& v : *arr->umap)
            {
                vm().push(v.first);
                vm().push(v.second);
            }
        }
        else if (arr->otype == TokenType::TypeUnorderedSet)
        {
            // Since uset is unordered, reversing makes no sense, so ignore the flag
            for (auto& v : *arr->uset)
            {
                vm().push(v);
            }
        }
        else if (arr->otype == TokenType::TypePair)
        {
            if (reverse)
            {
                vm().push(arr->pair->second);
                vm().push(arr->pair->first);
            }
            else
            {
                vm().push(arr->pair->first);
                vm().push(arr->pair->second);
            }
        }
        else
            throw std::runtime_error("expand expected a collection type on the stack");

        return nullptr;
    }

    void externalize(std::ostream &str, int indentation) const override
    {
        str << std::string(indentation, ' ') << TOK_COLL << " " << (reverse ? TOK_COLLRSPREAD : TOK_COLLSPREAD) << " " << std::endl;
    }

private:
    bool reverse;
};

}//ns

#endif //GC_EXPRCOLL_H
