#include <inttypes.h>
#include <iostream>
#include "Object.h"
#include "Stack.h"
#include "ExprExpressionList.h"
#include "AsmParser.h"
#include "ExprFFI.h"

namespace lake {

// Move to stdlib
extern "C" Object* lake_double(Object* obj)
{
    return obj->mul(obj, lake::track(Object::create((uint64_t)2)));
}

void ViewType::fromObjectAndViewType(Object &o, TokenType t)
{
    if (t >= TokenType::TypeViewUchar && t <= TokenType::TypeViewSint64)
    {
        bool sgn = (mpz_sgn(o.mpz) < 0);
        bool zero = mpz_cmp_ui(o.mpz, 0) == 0;

        // We need a larger type since mpz_export returns absolute values, which cannot represent the
        // smallest number (short is –32,768 to 32,767 for instance)
        // TODO: This means that we cannot convert to minlong!
        uint64_t s = 0;

        // mpz_export doesn't work, by design, if the value is zero.
        if (!zero)
            mpz_export(&s, NULL, 1, sizeof(s), 0, 0, o.mpz);

        if (t == TokenType::TypeViewSchar)
            _schar = sgn ? -s : s;
        else if (t == TokenType::TypeViewUchar)
            _uchar = sgn ? -s : s;
        else if (t == TokenType::TypeViewSshort)
            _sshort = sgn ? -s : s;
        else if (t == TokenType::TypeViewUshort)
            _ushort = sgn ? -s : s;
        else if (t == TokenType::TypeViewSint)
            _sint = sgn ? -s : s;
        else if (t == TokenType::TypeViewUint)
            _uint = sgn ? -s : s;
        else if (t == TokenType::TypeViewSlong)
            _slong = sgn ? -s : s;
        else if (t == TokenType::TypeViewUlong)
            _ulong = sgn ? -s : s;
        else if (t == TokenType::TypeViewSint8)
            _sint8 = sgn ? -s : s;
        else if (t == TokenType::TypeViewUint8)
            _uint8 = sgn ? -s : s;
        else if (t == TokenType::TypeViewSint16)
            _sint16 = sgn ? -s : s;
        else if (t == TokenType::TypeViewUint16)
            _uint16 = sgn ? -s : s;
        else if (t == TokenType::TypeViewSint32)
            _sint32 = sgn ? -s : s;
        else if (t == TokenType::TypeViewUint32)
            _uint32 = sgn ? -s : s;
        else if (t == TokenType::TypeViewSint64)
            _sint64 = sgn ? -s : s;
        else if (t == TokenType::TypeViewUint64)
            _uint64 = sgn ? -s : s;
    }
    else if (t == TokenType::TypeViewFloat && t == TokenType::TypeViewDouble)
    {
        double d = mpf_get_d(o.mpf);

        if (t == TokenType::TypeViewDouble)
            _double = d;
        else
            _float = (float) d;
    }
    else if (t == TokenType::TypeViewPointer)
    {
        // Doesn't matter if the data is in the utf8 field, as long as it's a pointer member
        _ptr = o.ptr_value;
    }
    else if (t == TokenType::TypeObject)
    {
        _obj = &o;
    }
    else
        throw std::runtime_error("Invalid view type");

}

FunctionData::FunctionData(Stack* stack, std::string name) : stack(stack), name(name)
{
}

/**
 * Copy construction in response to a "dup" evaluation. This should *only* be called by dup/copy.
 *
 * Note that a new stack is created for stacked functions.
 */
FunctionData::FunctionData(const FunctionData& copy)
{
    this->name = copy.name;
    this->body = copy.body;
    this->withStack = copy.withStack;

    this->locals = copy.locals;
    this->args = copy.args;
    this->creator = copy.creator;

    if (copy.withStack)
    {
        this->stack = (Stack*) lake::track(new (vm().stackpool.malloc()) Stack());
    }
    else
    {
        this->stack = nullptr;
    }
}

Object* FunctionData::evaluateBody(Object* functionObject)
{
    // The function can be executed with its own stack (typically the case
    // with a closure), so temporarily switch stacks if necessary.

    // Here we do a) frameing for local framed functions, and b) stack switch for stacked functions

    if (withStack && stack != nullptr)
        vm().stacks.push_back(stack);
    else
        vm().markStackBase();

    Object* oldFunction = vm().current;
    vm().current = functionObject;

    // Prevent GC while we're evaluating
    functionObject->setFlag(FLAG_GC_PINNED);
    body->setFlag(FLAG_GC_PINNED);

    Object* res = body->eval();

    functionObject->clearFlag(FLAG_GC_PINNED);
    body->clearFlag(FLAG_GC_PINNED);

    vm().current = oldFunction;

    // Restore stack
    if (withStack && stack != nullptr)
        vm().stacks.pop_back();
    else
        vm().restoreStackBase();

    return res;
}

void FunctionData::mark()
{
    if (creator)
        creator->mark();

    for (Object* obj : locals)
    {
        obj->mark();
    }

    for (Object* obj : args)
        obj->mark();

    // Mark expression list items
    body->mark();

    if (stack)
        stack->mark();
}

void ProjectionData::mark()
{
    collection->mark();
}

Object::Object(const Object& obj) : Object(obj.otype, FLAG_GC_PINNED)
{
    if (isInteger())
        mpz_set(mpz, obj.mpz);
    else if (isFloat())
        mpf_set(mpf, obj.mpf);
    else if (otype == TokenType::TypeString || otype == TokenType::TypeSymbol)
        str_value = new std::string(*obj.str_value);
    else if (otype == TokenType::TypeBool)
        bool_value = obj.bool_value;
    else if (otype == TokenType::TypeFunction)
        fndata = vm().fnpool.construct(*obj.fndata);
    else if (otype == TokenType::TypeArray)
        array = new std::vector<Object*>(*obj.array);

    // A bit subtle: dup/copy of a projection creates a real array of the projection
    else if(otype == TokenType::TypeProjection)
    {
        array = new std::vector<Object*>(obj.projection->collection->array->begin()+obj.projection->start,
                                         obj.projection->collection->array->end()-obj.projection->end);

        // array or set
        otype = obj.projection->collection->otype;
    }
    else if (otype == TokenType::TypeFFIStruct)
        structdata = new StructData(*obj.structdata);
    else if (otype == TokenType::TypeViewPointer)
        ptr_value = obj.ptr_value;
    else
        throw std::runtime_error("Invalid copy construction");

#ifdef VM_DEBUG
    debug = nullptr;
#endif
}

Object::Object(TokenType otype, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = otype;

    if (isInteger())
        mpz_init(mpz);
    else if (isFloat())
        mpf_init(mpf); // default is 53 bit precision
}


Object::Object(std::pair<Object*,Object*>* value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypePair;
    pair = value;
}

Object::Object(std::vector<Object*>* value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeArray;
    array = value;
}

Object::Object(std::unordered_map<Object*,Object*>* value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeUnorderedMap;
    umap = value;
}

Object::Object(std::unordered_set<Object*>* value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeUnorderedSet;
    uset = value;
}

Object::Object(int64_t value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeInt;
    mpz_init(mpz);
    mpz_set_sx(mpz, value);
}

Object::Object(uint64_t value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeInt;
    mpz_init(mpz);
    mpz_set_ux(mpz, value);
}

Object::Object(mpz_t value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeInt;
    mpz_init(mpz);
    mpz_set(mpz, value);
}

Object::Object(mpf_t value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeFloat;
    mpf_init(mpf);
    mpf_set(mpf, value);
}

Object::Object(FunctionData* fndata, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeFunction;
    this->fndata = fndata;
}

Object::Object(SymbolData* symdata, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeFFISymbol;
    this->symdata = symdata;
}

Object::Object(StructData* structdata, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeFFIStruct;
    this->structdata = structdata;
}

Object::Object(ProjectionData* projdata, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeProjection;
    this->projection = projdata;
}

Object::Object(double value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeFloat;
    mpf_init(mpf);
    mpf_set_d(mpf, value);
}

Object::Object(char* value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeString;
    this->str_value = new std::string((const char*)value);

    setFlag(FLAG_FREESTORE);
}

Object::Object(char value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeChar;
    this->char_value = value;
}

Object::Object(bool value, uint8_t flags) : flags(flags|FLAG_GC_PINNED)
{
    this->otype = TokenType::TypeBool;
    this->bool_value = value;
}

void Object::track()
{
    if (hasFlag(FLAG_GC_TRACKED))
        throw std::runtime_error("BUG: Object already tracked");

    setFlag(FLAG_GC_TRACKED);

    // All objects are constructed pinned, track removes it
    clearFlag(FLAG_GC_PINNED);

    vm().numObjects++;

    // Put ourself at the head of the heap gc chain
    prev = nullptr;
    next = vm().heapHead;
    if (next != nullptr)
        next->prev = this;
    vm().heapHead = this;
}

void Object::mark()
{
    if (hasFlag(FLAG_GC_REACHABLE) || !hasFlag(FLAG_GC_TRACKED))
        return;

    setFlag(FLAG_GC_REACHABLE);

    if (otype == TokenType::TypeFunction)
    {
        fndata->mark();
    }
    if (otype == TokenType::TypeProjection)
    {
        projection->mark();
    }
    else if (otype == TokenType::TypePair && pair != nullptr)
    {
        if (pair->first)
            pair->first->mark();
        if (pair->second)
            pair->second->mark();
    }
    else if (otype == TokenType::TypeArray && array != nullptr)
    {
        for (const auto& obj : *array)
        {
            obj->mark();
        }
    }
    else if (otype == TokenType::TypeUnorderedMap && umap != nullptr)
    {
        for (const auto& obj : *umap)
        {
            obj.first->mark();
            obj.second->mark();
        }
    }
    else if (otype == TokenType::TypeUnorderedSet && uset != nullptr)
    {
        for (const auto& obj : *uset)
        {
            obj->mark();
        }
    }
}

Object* Object::destruct()
{
    if (otype == TokenType::InvalidCollected)
        throw std::runtime_error("Destructing already collected object");

    if (hasFlag(FLAG_GC_PINNED))
        throw std::runtime_error("BUG:Trying to destruct a pinned object");

    if (!hasFlag(FLAG_GC_TRACKED))
        throw std::runtime_error("BUG:Trying to destruct an untracked object");

    if (isInteger())
        mpz_clear(mpz);
    else if (isFloat())
        mpf_clear(mpf);
    else if (otype == TokenType::TypeFunction && fndata != nullptr)
        vm().fnpool.free(fndata);
    else if (otype == TokenType::TypeString || otype == TokenType::TypeSymbol)
        delete str_value;
    else if (otype == TokenType::TypePair)
        delete pair;
    else if (otype == TokenType::TypeArray && !hasFlag(FLAG_FOREIGN))
        delete array;
    else if (otype == TokenType::TypeUnorderedMap && !hasFlag(FLAG_FOREIGN))
        delete umap;
    else if (otype == TokenType::TypeUnorderedSet && !hasFlag(FLAG_FOREIGN))
        delete uset;
    else if (otype == TokenType::TypeFFIStruct)
        delete structdata;
    else if (otype == TokenType::TypeFFISymbol)
        delete symdata;
    else if (otype == TokenType::TypeProjection)
        delete projection;

    otype = TokenType::InvalidCollected;
    ptr_value = nullptr;

    // Remove ourselves from the sweeplist
    if (vm().heapHead == this)
        vm().heapHead = this->next;
    else
    {
        if (prev != nullptr)
            prev->next = next;
        if (next != nullptr)
            next->prev = prev;
    }

    Object* res = next;

    vm().pool.free(this);

    return res;
}

// This should only be called from ExprCast, which makes a copy of the object being casted.
// The copy is changed in-place by this method.
void Object::castTo(TokenType target)
{
    if (otype == target)
        return;

    if (hasFlag(FLAG_CONST))
        throw std::runtime_error("Trying to mutate a const object");

    // Int to float
    if (otype == TokenType::TypeInt && target == TokenType::TypeFloat)
    {
        // mpf/mpz are in a union, so convert using a tmp
        mpf_t tmp_mpf;
        mpf_init(tmp_mpf);
        mpf_set_z(tmp_mpf, mpz);

        mpz_clear(mpz);
        mpf_init_set(mpf, tmp_mpf);
        mpf_clear(tmp_mpf);
    }
    else if (otype == TokenType::TypeFloat && target == TokenType::TypeInt)
    {
        // mpf/mpz are in a union, so convert using a tmp
        mpz_t tmp_mpz;
        mpz_init(tmp_mpz);
        mpz_set_f(tmp_mpz, mpf);

        mpf_clear(mpf);
        mpz_init_set(mpz, tmp_mpz);
        mpz_clear(tmp_mpz);
    }
    else if (isNumeric() && (target == TokenType::TypeString|| target == TokenType::TypeSymbol))
    {
        str_value = new std::string(toString());

        if (otype == TokenType::TypeInt)
            mpz_clear(mpz);
        else if (otype == TokenType::TypeFloat)
            mpf_clear(mpf);

        setFlag(FLAG_FREESTORE);
    }
    else if (otype == TokenType::TypeString && target == TokenType::TypeInt)
    {
        mpz_t tmp_mpz;
        mpz_init(tmp_mpz);
        mpz_set_str(tmp_mpz, str_value->data(), 0 /* use leading char to determine radix */);

        if (hasFlag (FLAG_FREESTORE))
            delete str_value;

        mpz_init_set(mpz, tmp_mpz);
        mpz_clear(tmp_mpz);
    }
    else if (otype == TokenType::TypeString && target == TokenType::TypeFloat)
    {
        mpf_t tmp_mpf;
        mpf_init(tmp_mpf);
        mpf_set_str(tmp_mpf, str_value->data(), 0 /* use leading char to determine radix */);

        if (hasFlag (FLAG_FREESTORE))
            delete str_value;

        mpf_init_set(mpf, tmp_mpf);
        mpf_clear(tmp_mpf);
    }
    else if (otype == TokenType::TypeString && target == TokenType::TypeBool)
    {
        if (str_value != nullptr && str_value->compare("true")==0)
            bool_value = true;
        else if (str_value != nullptr && str_value->compare("false")==0)
            bool_value = false;
        else
            throw std::runtime_error("Invalid type conversion: string to bool accepts \"true\" and \"false\" only");
    }
    else if (otype == TokenType::TypeInt && target == TokenType::TypeBool)
    {
        // Note that we check the length+1 to catch input errors like "true1"
        if (equals(this, &Object::oneObject()) == &Object::trueObject())
            bool_value = true;
        else if (equals(this, &Object::zeroObject()) == &Object::trueObject())
            bool_value = false;
        else
            throw std::runtime_error("Invalid type conversion: int to bool accepts 1 and 0 only");
    }
    else if (otype == TokenType::TypeBool && target == TokenType::TypeInt)
    {
        if (bool_value)
            mpz_init_set_sx(mpz, (int64_t)1);
        else
            mpz_init_set_sx(mpz, (int64_t)0);
    }
    else if (otype == TokenType::TypeBool && target == TokenType::TypeString)
    {
        static char trueString[] = "true";
        static char falseString[] = "false";

        str_value = new std::string(bool_value ? trueString : falseString);

        setFlag(FLAG_FREESTORE);
    }
    else if (otype == TokenType::TypeViewPointer && target == TokenType::TypeString)
    {
        // Make a copy
        str_value = new std::string((const char*)ptr_value);
        setFlag(FLAG_FREESTORE);
    }
    else if (otype == TokenType::TypeFFIStruct && target == TokenType::TypeArray)
    {
        // This conversion pops two inputs (StructData and ptr object), and we're pushing an entirely new array object
        // containing the struct values.

        StructData* sd = this->structdata;
        Object* obj = vm().pop();
        uint8_t* ptr = (uint8_t*) obj->ptr_value;

        ExprFFICall ffi;
        array = ffi.structToArray(sd, ptr);
    }
    else if (otype == TokenType::TypeString && target == TokenType::TypeFunction)
    {
        // This cast allows assembly source to be parsed into a function. Nice for building
        // macros and generally parsing snippets of assembly on demand.

        ExprExpressionList* el = new ExprExpressionList();
        lake::track(el);

        std::istringstream input(*str_value);

        AsmParser p(vm());
        p.parse(input, "macro", el);

        fndata = vm().fnpool.construct((Stack*)nullptr);
        fndata->body = el;
    }
    else
        throw std::runtime_error("Invalid type conversion");

    otype = target;
}

Object* Object::eval()
{
    return this;
}

std::string Object::dump() const
{
    std::string res;
    char buff[256];
    snprintf(buff, 256, "obj@%p (%ld bytes): type: %" PRIu8 ", val: ",
             this, sizeof(*this), static_cast<uint8_t >(this->otype));
    res = buff;
    res += toString();

#ifdef VM_DEBUG
    if (debug != nullptr)
    {
        res += " <";
        res += debug;
        res += ">";
    }
#endif

    return res;
}

std::string Object::typestring() const
{
    return typestring(this->otype);
}

std::string Object::typestring(TokenType otype) const
{
    switch (otype)
    {
        case TokenType::TypeInt:
            return "int";
        case TokenType::TypeFloat:
            return "float";
        case TokenType::TypeBool:
            return "bool";
        case TokenType::TypeFunction:
            return "function";
        case TokenType::TypeString:
            return "string";
        case TokenType::TypeSymbol:
            return "symbol";
        case TokenType::TypeArray:
            return "array";
        case TokenType::TypeUnorderedMap:
            return "map";
        case TokenType::TypePair:
            return "pair";
        case TokenType::TypeUnorderedSet:
            return "set";
        case TokenType::TypeObject:
            return "object";
        case TokenType::TypeChar:
            return "char";
        case TokenType::TypeViewPointer:
            return "ptr";
        case TokenType::TypeExprListObject:
            return "exprlist";
        default:
            return "invalid-type";
    }

    return std::string();
}

void Object::externalize(std::ostream& str, int indentation) const
{
    if (hasFlag(FLAG_ISNULL) || this == &Object::nullObject())
    {
        str << "null";
    }
    else if (isInteger())
    {
        void(*freefunc)(void *, size_t);
        mp_get_memory_functions(NULL, NULL, &freefunc);

        // NOTE: mpz_get_str is implemented recursively and may
        // blow up on small stacks
        char* tmp = mpz_get_str(nullptr, 10 /*radix*/, mpz);
        str << std::string(tmp);

        freefunc(tmp, strlen(tmp) + 1);
    }
    else if (isFloat())
    {
        void(*freefunc)(void *, size_t);
        mp_get_memory_functions(NULL, NULL, &freefunc);

        char* tmp = nullptr;
        size_t len = gmp_asprintf(&tmp, "%.Ff", mpf);
        str << std::string(tmp);

        freefunc(tmp, len + 1);
    }
    else if (otype == TokenType::TypeString || otype == TokenType::TypeSymbol)
    {
        if (str_value == nullptr)
            // Should be covered by isnull check above
            std::runtime_error("Null string");
        else
            str << '"' << *this->str_value << '"';
    }
    else if (otype == TokenType::TypeBool)
    {
        str << ((char *) (bool_value ? "true" : "false")) << std::endl;
    }
    else if (otype == TokenType::TypeFunction)
    {
        if (fndata->withStack)
            str << "withstack ";

        // Prefix function names with underscore to ensure it is parsed as an identifier, even if it
        // is called the same as special characters, such as "-"
        str << '_' << fndata->name << std::endl << std::string(indentation, ' ') << "{" << std::endl;
        fndata->body->externalize(str, indentation + 4);
        str << std::string(indentation, ' ') << "}";
    }
    else if(otype == TokenType::TypeChar)
    {
        str << "'" << (char)char_value << "'";
    }
    else if(otype == TokenType::TypeViewPointer)
    {
        str << "0x" << std::hex << (ptrdiff_t) ptr_value << std::dec;
    }
    else if(otype == TokenType::TypeArray)
    {
        str << array->capacity();
    }
    else if(otype == TokenType::TypeUnorderedMap)
    {
        // map/set lacks capacity() members
        str << "10";
    }
    else if(otype == TokenType::TypeUnorderedSet)
    {
        str << "10";
    }
    else if(otype == TokenType::TypePair)
    {
        // NOOP, type has no arguments
    }
    else if (this == &Object::repeatObject())
    {
        str << std::string(indentation, ' ') << TOK_REPEAT << std::endl;
    }
    else
    {
        str << "ERROR: No external representation, type: " << (int)otype << std::endl;
    }
}

std::string Object::toString() const
{
    std::string res("");
    
    if (isInteger())
    {
        void(*freefunc)(void *, size_t);
        mp_get_memory_functions(NULL, NULL, &freefunc);

        // NOTE: mpz_get_str is implemented recursively and may
        // blow up on small stacks or huge input. There's a bug report.
        char* tmp = mpz_get_str(nullptr, 10 /*radix*/, mpz);
        res += tmp;

        freefunc(tmp, strlen(tmp) + 1);
    }
    else if (isFloat())
    {
        void(*freefunc)(void *, size_t);
        mp_get_memory_functions(NULL, NULL, &freefunc);

        char* tmp = nullptr;
        size_t len = gmp_asprintf(&tmp, "%.Ff", mpf);
        res += tmp;

        freefunc(tmp, len + 1);
    }
    else
    {
        if (hasFlag(FLAG_ISNULL))
        {
            res = "null";
        }
        else if (otype == TokenType::TypeString || otype == TokenType::TypeSymbol)
        {
            res = this->str_value ? *this->str_value : "null";
        }
        else if (otype == TokenType::TypeBool)
        {
            res = (char *) (bool_value ? "true" : "false");
        }
        else if (otype == TokenType::TypeFunction)
        {
            res = fndata->name.c_str();
            res += (char *) " [function]";
        }
        else if(otype == TokenType::TypeChar)
        {
            res += (char)char_value;
        }
        else if (otype == TokenType::TypeViewPointer)
        {
            char ptr[64];
            snprintf(ptr,64,"%p", ptr_value);
            res += ptr;
        }
        else if (otype == TokenType::TypeOperation)
        {
            res = (char *) "Operation";
        }
        else if (otype == TokenType::TypePair)
        {
            res += "pair[";
            res += pair->first == nullptr ? "null" : pair->first->toString();
            res += ", ";
            res += pair->second == nullptr ? "null" : pair->second->toString();
            res += "]";
        }
        else if (otype == TokenType::TypeArray)
        {
            res += "arr[";
            size_t count = 0;
            for (const auto& obj : *array)
            {
                if (count > 0)
                    res += ",";

                res += obj->toString();
                count++;
            }
            res += "]";
        }
        else if (otype == TokenType::TypeUnorderedMap)
        {
            res += "map[";
            size_t count = 0;
            for (const auto& obj : *umap)
            {
                if (count > 0)
                    res += ",";

                res += obj.first->toString();
                res += "=>";
                res += obj.second->toString();

                count++;
            }
            res += "]";
        }
        else if (otype == TokenType::TypeUnorderedSet)
        {
            res += "set[";
            size_t count = 0;
            for (const auto& obj : *uset)
            {
                if (count > 0)
                    res += ",";

                res += obj->toString();

                count++;
            }
            res += "]";
        }
        else if (otype == TokenType::TypeFFISymbol)
        {
            res += "Symbol: ";
            res += symdata->name;
        }
        else if (otype == TokenType::Invalid)
        {
            res = (char *) "Invalid type";
        }
        else
        {
            res = (char *) "N/A";
        }
    }

    return res;
}

}//ns
