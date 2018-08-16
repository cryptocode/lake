#ifndef LAKE_OBJECT_H
#define LAKE_OBJECT_H

#include <iosfwd>
#include <map>
#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>
#include <gmp.h>
#include <cstring>
#include <ffi.h>
#include <cfloat>
#include <sstream>
#include "VMTypes.h"
#include "Process.h"
#include "VM.h"

namespace lake {

class Object;
class ExprExpressionList;
class Stack;

#ifdef WIN32
	#define PACK_ATTR
	#pragma pack (push,1)
#else
	#define PACK_ATTR __attribute__ ((__packed__))
#endif

// http://www.chiark.greenend.org.uk/doc/libffi-dev/html/Primitive-Types.html#Primitive-Types
union ViewType
{
    Object* _obj;
    void* _ptr;
    int64_t _sint64;
    uint64_t _uint64;
    int32_t _sint32;
    uint32_t _uint32;
    int16_t _sint16;
    uint16_t _uint16;
    int8_t _sint8;
    uint8_t _uint8;
    unsigned char _uchar;
    signed char _schar;
    unsigned short _ushort;
    short _sshort;
    int _sint;
    unsigned int _uint;
    long _slong;
    unsigned long _ulong;
    float _float;
    double _double;

    // This is 80-bits, but probably never used...
    // We simply don't support it for now (when supported, use indirection to save space)
    //long double _longdouble;

    void fromObjectAndViewType(Object &o, TokenType t);
};

/**
 * Function object instance data
 */
struct PACK_ATTR FunctionData
{
    FunctionData(Stack* stack, std::string name="");

    /**
     * Copy construction. Note that a new stack is created for withstack-function.
     */
    FunctionData(const FunctionData& copy);

    /**
     * Evaluate the body of this function. We pass the owning function object, so
     * that we can store/restore the previous "current" function (available via ExprCurrentFunction)
     *
     * @param functionObject
     * @return
     */
    Object* evaluateBody(Object* functionObject);

    /**
     * GC marking
     */
    void mark();

    /**
     * The root function has the root stack. However, any function can
     * have its own stack (source-wise, by using the withstack assembly directive)
     */
    Stack* stack = nullptr;

    /**
     * Nested functions/lambas may need to reference free variables.
     * Every top-level function points to the root function.
     */
    Object* creator = nullptr;

    // Scratch area for locals. All functions have this.
    std::vector<Object*> locals;

    // For functions with free variables, this will contain references to parameter objects
    std::vector<Object*> args;

    bool withStack = false;
    ExprExpressionList* body = nullptr;
    std::string name = "";
};

/**
 * FFI symbol data
 */
struct PACK_ATTR SymbolData
{
    SymbolData(VMFFI_MOD_TYPE mod,VMFFI_PROC_TYPE sym,ffi_type* ffiRetType,std::vector<ffi_type*> ffiArgTypes,std::string name)
        : mod(mod), sym(sym), ffiRetType(ffiRetType), ffiArgTypes(ffiArgTypes), name(name){}

    VMFFI_MOD_TYPE mod;
    VMFFI_PROC_TYPE sym;
    ffi_type* ffiRetType;
    std::vector<ffi_type*> ffiArgTypes;
    std::string name;
};

/**
 * FFI struct data
 */
struct PACK_ATTR StructData
{
    StructData(std::string name) : name(name) {}
    StructData(std::string name,std::vector<ffi_type*> elementTypes) : name(name), elementTypes(elementTypes) {}
    StructData(const StructData& obj)
    {
        name = obj.name;
        elementTypes = obj.elementTypes;
        values = obj.values;
    }

    std::string name;
    std::vector<ffi_type*> elementTypes;
    std::vector<void*> values;
};

struct PACK_ATTR ProjectionData
{
    Object* collection=nullptr;
    ssize_t start=0;
    ssize_t end=0;

    void mark();
};

inline Object* track(Object* obj);

/**
 * A virtual machine Object is a variant type. All values pushed to
 * the stack is an Object.
 */
class PACK_ATTR Object
{
    // Make sure structs are standard layout, so we can do FFI with the object data.
    // http://en.cppreference.com/w/cpp/concept/StandardLayoutType

    static_assert(std::is_standard_layout<StructData>::value, "StructData must have standard layout");
    static_assert(std::is_standard_layout<SymbolData>::value, "SymbolData must have standard layout");

public:

    // Note that Object has a vtbl, so don't try treating an Object* as a union value despite the fact that
    // it's the first member (which is merely for cache locality)
    union
    {
        mpf_t mpf;
        mpz_t mpz;

        /* TypeString, a string containing utf8 encoded unicode characters.
         * or TypeSymbol, naming a unique interned lisp-like symbol */
        std::string* str_value;

        /* TypeChar, unicode code point */
        uint32_t char_value;

        /* TypeBool */
        bool bool_value;

        /* TypeArray; growable arrays */
        std::vector<Object*>* array;

        /* TypeUnorderedMap */
        std::unordered_map<Object*,Object*>* umap;

        /* TypeUnorderedMap */
        std::unordered_set<Object*>* uset;

        // TODO: ordered map/set. Must define good hashes/compare functions for each object type (including nested maps, etc)

        /* TypeFunction */
        FunctionData* fndata;

        /* TypeFFISymbol */
        SymbolData* symdata;

        /* TypeFFIStruct */
        StructData* structdata;

        /* TypeProjection */
        ProjectionData* projection;

        /* TypeOperation; since a few expressions need this, and they are Object's anyway,
         * we might as well put the union to use to save some memory (instead of having this
         * as an extra member in relevant subclasses */
        ExprExpressionList* exprlist;

        /**
         * TypePair.
         */
        std::pair<Object*,Object*>* pair;

        /* Arrays, buffers, native pointers, etc */
        void* ptr_value = nullptr;

        /* Various view types. Useful for native wrappers and FFI purposes */
        ViewType view;
    };

    TokenType otype = TokenType::Invalid;

    // By default, objects are pinned, e.g not eligible for gc. This flag is removed if track() is called
    uint8_t flags = FLAG_GC_PINNED;

#ifdef VM_DEBUG
    char* debug = nullptr;

    inline void setDebug(const char* str)
    {
        if (str)
        {
            if (debug) delete [] debug;
            debug = new char[strlen(str) + 1];
            strcpy(debug, str);
        }
    }
#else
    inline void setDebug(const char* str)
    {
    }
#endif

    // Sweep chaining
    Object* next = nullptr;
    Object* prev = nullptr;

    Object() = delete;

    Object(const Object& obj);
    Object(Object&& obj) = delete;
    Object& operator=(Object&& obj) = delete;

    /**
     * For storing as int, but representing another type, like bool
     */
    explicit Object(int64_t value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(uint64_t value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(double value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(char *value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(char value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(mpz_t value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(mpf_t value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(FunctionData* fndata, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(SymbolData* symdata, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(StructData* symdata, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(ProjectionData* projdata, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(std::pair<Object*,Object*>* value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(std::vector<Object*>* value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(std::unordered_map<Object*,Object*>* value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(std::unordered_set<Object*>* value, uint8_t flags = FLAG_GC_PINNED);
    explicit Object(bool value, uint8_t flags = FLAG_GC_PINNED);

    /**
     * For operations
     */
    explicit Object(TokenType otype, uint8_t flags = 0);

    /**
     * Object creation wrapper which allocates from the pool. This is the
     * preferred way to obtain new objects.
     *
     * Wrap calls to create with track(...) to make the returned object eligible
     * for garbage collection.
     */
    template<typename... Args>
    static inline Object* create(Args&&... args)
    {
        return new (vm().pool.malloc()) Object(std::forward<Args>(args)...);
    }

    /**
     * Destruct contained data. The GC cycle / remove instruction calls this and places
     * the object the freelist. The Object instance itself is only delete'ed when the
     * freelist is full.
     *
     * \returns The next object on the free list, or nullptr if no more object
     */
    Object* destruct();

    // Avoid conversion from string literals
    Object(const char* value) = delete;

    static Object& trueObject()
    {
        static Object OBJECT_TRUE(true);
        return OBJECT_TRUE;
    }

    static Object& falseObject()
    {
        static Object OBJECT_FALSE(false);
        return OBJECT_FALSE;
    }

    static Object& zeroObject()
    {
        static Object OBJECT_ZERO((int64_t)0);
        return OBJECT_ZERO;
    }

    static Object& oneObject()
    {
        static Object OBJECT_ONE((int64_t)1);
        return OBJECT_ONE;
    }

    /**
     * Set the default precision to be *at least* 'prec' bits.
     *
     * @param prec
     */
    static void setDefaultPrecision(int prec)
    {
        mpf_set_default_prec(prec);
    }

    static auto getDefaultPrecision()
    {
        return mpf_get_default_prec();
    }

    /**
     * A null singleton for a given type. Must not be mutated.
     *
     * @tparam OT
     * @return
     */
    template <TokenType OT=TokenType::Invalid>
    static Object& nullObject()
    {
        static Object OBJECT_NULL(OT, FLAG_ISNULL | FLAG_CONST | FLAG_GC_PINNED);
        return OBJECT_NULL;
    }

    /**
     * A new mutable null object. Nice for placeholders that will later be mutated.
     * @return A newly allocated object marked as null
     */
    /*static Object* newNullObject()
    {
        return new Object(TokenType::TypeObject, FLAG_ISNULL);
    }*/

    static Object& exitScopeObject()
    {
        static Object OBJECT_EXITSCOPE(TokenType::TypeObject);
        return OBJECT_EXITSCOPE;
    }

    static Object& repeatObject()
    {
        static Object OBJECT_REPEAT(TokenType::TypeObject);
        return OBJECT_REPEAT;
    }

    static Object& repeatIfTrueObject()
    {
        static Object OBJECT_REPEAT(TokenType::TypeObject);
        return OBJECT_REPEAT;
    }

    static Object& repeatIfFalseObject()
    {
        static Object OBJECT_REPEAT(TokenType::TypeObject);
        return OBJECT_REPEAT;
    }

    // Sentinel to tell the current expression list evaluator that there's
    // a tail call invoke.
    static Object& tailcallRequestObject()
    {
        static Object OBJECT_TCALL(TokenType::TypeObject);
        return OBJECT_TCALL;
    }

    /**
     * Exit sentinel
     */
    static Object& exitRequestObject()
    {
        static Object OBJECT_EXIT(TokenType::TypeObject);
        return OBJECT_EXIT;
    }

    static Object& raiseRequestObject()
    {
        static Object OBJECT_RAISE(TokenType::TypeObject);
        return OBJECT_RAISE;
    }

    static Object& errorLabelObject()
    {
        static Object OBJECT_ERROR(TokenType::TypeObject);
        return OBJECT_ERROR;
    }

    inline long asLong() const
    {
        if (isInteger())
            return mpz_get_si(mpz);
        else if (isFloat())
            return mpf_get_si(mpf);
        else
            throw std::runtime_error("Invalid conversion to long: Object is not an integer");
    }

    inline long asInt() const { return (int) asLong(); }

    inline unsigned long asULong() const
    {
        if (isInteger())
            return mpz_get_ux(mpz);
        else if (isFloat())
            return (unsigned long)mpf_get_ui(mpf);
        else
            throw std::runtime_error("Invalid conversion to long: Object is not an integer");
    }

    template <typename T>
    T fromInt()
    {
        if (isInteger())
        {
            T result = 0;

            // Believe it or not, mpz_export doesn't work if the value is zero
            bool zero = mpz_cmp_ui(mpz, 0) == 0;
            if (!zero)
                mpz_export(&result, 0, -1, sizeof result, 0, 0, mpz);

            return result;
        }
        else
            throw std::runtime_error("Invalid conversion: Object is not an integer");
    }

    /**
     * Implements negate (for numerics) and logical NOT for booleans
     *
     * This updates the object in-place, so make sure you have pushed a copy of what you're changing.
     */
    inline Object& inplace_negate()
    {
        if (otype == TokenType::TypeBool)
        {
            bool_value = !bool_value;
        }
        else if (isInteger())
        {
            mpz_neg(mpz,mpz);
        }
        else if (isFloat())
        {
            mpf_neg(mpf, mpf);
        }
        else
            throw std::runtime_error("Negate/Logical not can only be applied to numeric and boolean objects");

        return *this;
    }

    inline Object& inplace_decrement()
    {
        if (isInteger())
        {
            mpz_sub_ui(mpz, mpz, 1);
        }
        else if (isFloat())
        {
            mpf_sub_ui(mpf, mpf, 1);
        }
        else
            throw std::runtime_error("Decrement can only be applied to numeric objects");

        return *this;
    }

    inline Object& inplace_increment()
    {
        if (isInteger())
        {
            mpz_add_ui(mpz, mpz, 1);
        }
        else if (isFloat())
        {
            mpf_add_ui(mpf, mpf, 1);
        }
        else
            throw std::runtime_error("Increment can only be applied to numeric objects");

        return *this;
    }

    inline Object* mp_binop2(Object* lhs, Object* rhs,
                           std::function<void(mpz_t&,mpz_t&,mpz_t&)>& mpz_func,
                           std::function<void(mpf_t&,mpf_t&,mpf_t&)>& mpf_func)
    {
        Object* res = lake::track(create(lhs->otype));

        if (lhs->isInteger())
        {
            mpz_func(const_cast<mpz_t&>(res->mpz), const_cast<mpz_t&>(lhs->mpz), const_cast<mpz_t&>(rhs->mpz));
        }
        else if (lhs->isFloat())
        {
            mpf_func(const_cast<mpf_t&>(res->mpf), const_cast<mpf_t&>(lhs->mpf), const_cast<mpf_t&>(rhs->mpf));
        }
        else
        {
            throw std::runtime_error("Invalid binary operand types");
        }

        return res;
    }

    inline int mp_cmp_op2(Object* lhs, Object* rhs,
                  std::function<int(mpz_t&,mpz_t&)>& mpz_func,
                  std::function<int(mpf_t&,mpf_t&)>& mpf_func)
    {
        int res = 0;

        if (lhs->isInteger())
        {
            res = mpz_func(const_cast<mpz_t&>(lhs->mpz), const_cast<mpz_t&>(rhs->mpz));
        }
        else if (lhs->isFloat())
        {
            res = mpf_func(const_cast<mpf_t&>(lhs->mpf), const_cast<mpf_t&>(rhs->mpf));
        }
        else
        {
            throw std::runtime_error("Invalid comparison operand types");
        }

        return res;
    }

    inline Object* add(Object* lhs, Object* rhs)
    {
        std::function<void(mpz_t&,mpz_t&,mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b, mpz_t& c) {mpz_add(a,b,c);};
        std::function<void(mpf_t&,mpf_t&,mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b, mpf_t& c) {mpf_add(a,b,c);};

        return mp_binop2(lhs, rhs, mpz_op, mpf_op);
    }

    inline Object* sub(Object* lhs, Object* rhs)
    {
        std::function<void(mpz_t&,mpz_t&,mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b, mpz_t& c) {mpz_sub(a,b,c);};
        std::function<void(mpf_t&,mpf_t&,mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b, mpf_t& c) {mpf_sub(a,b,c);};

        return mp_binop2(lhs, rhs, mpz_op, mpf_op);
    }

    inline Object* mul(Object* lhs, Object* rhs)
    {
        std::function<void(mpz_t&,mpz_t&,mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b, mpz_t& c) {mpz_mul(a,b,c);};
        std::function<void(mpf_t&,mpf_t&,mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b, mpf_t& c) {mpf_mul(a,b,c);};

        return mp_binop2(lhs, rhs, mpz_op, mpf_op);
    }

    inline Object* div(Object* lhs, Object* rhs)
    {
        std::function<void(mpz_t&,mpz_t&,mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b, mpz_t& c) {mpz_div(a,b,c);};
        std::function<void(mpf_t&,mpf_t&,mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b, mpf_t& c) {mpf_div(a,b,c);};

        return mp_binop2(lhs, rhs, mpz_op, mpf_op);
    }

    inline Object* logicalOr(Object* lhs, Object* rhs)
    {
        if (lhs->otype == TokenType::TypeBool && rhs->otype == TokenType::TypeBool)
            return lhs->bool_value || rhs->bool_value ? &Object::trueObject() : &Object::falseObject();
        throw std::runtime_error("or only accept boolean operands");

    }

    inline Object* logicalAnd(Object* lhs, Object* rhs)
    {
        if (lhs->otype == TokenType::TypeBool && rhs->otype == TokenType::TypeBool)
            return lhs->bool_value && rhs->bool_value ? &Object::trueObject() : &Object::falseObject();
        throw std::runtime_error("and only accept boolean operands");

    }

    inline Object* same(Object* lhs, Object* rhs)
    {
        return lhs == rhs ? &Object::trueObject() : &Object::falseObject();
    }

    inline Object* is(Object* lhs, Object* rhs)
    {
        return lhs->otype == rhs->otype ? &Object::trueObject() : &Object::falseObject();
    }

    inline Object* equals(Object* lhs, Object* rhs)
    {
        if (lhs == nullptr || rhs == nullptr || (lhs->otype != rhs->otype))
            throw std::runtime_error("eq only accept equal non-null types: bool, string and numeric");

        if (lhs->otype == TokenType::TypeBool && rhs->otype == TokenType::TypeBool)
            return lhs->bool_value == rhs->bool_value ? &Object::trueObject() : &Object::falseObject();
        else if (lhs->isNumeric() && rhs->isNumeric())
        {
            std::function<int(mpz_t&, mpz_t&)> mpz_op = [&](mpz_t &a, mpz_t &b) { return mpz_cmp(a, b); };

            // https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
            //  - Basically: when comparing against zero, use absolute epsilon
            //  - Otherwise, use relative epsilon.
            // Does it hold for arbitrary precision math...?
            // Maybe allow optionally comparison with epsilon: (https://github.com/blynn/pbc/blob/master/ecc/hilbert.c), or mpf_reldiff
            std::function<int(mpf_t&, mpf_t&)> mpf_op = [&](mpf_t &a, mpf_t &b)
            {
                mpf_t reldiff;
                mpf_init(reldiff);
                mpf_reldiff(reldiff, a, b);

                // DEBUGGING
                /*char* tmp = nullptr;
                gmp_asprintf(&tmp, "%.Ff", reldiff);
                std::cout << "RELDIFF: " << tmp << std::endl;*/

                // Note: The epsilon can be set with "epsilon N"
                // It's probably ok to use mpf_cmp against an mpf_t epsilon, mpf_cmp_d only allows for 1.11e-16 epsilon at best
                auto res = mpf_cmp(reldiff, vm().epsilon);
                mpf_clear(reldiff);

                // If reldiff is less than epsilon, consider it equal.
                return res < 0 ? 0 : -1;
            };

            return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) == 0 ? &Object::trueObject() : &Object::falseObject();
        }
        else if (lhs->otype == TokenType::TypeString && rhs->otype == TokenType::TypeString)
            return (lhs->str_value->compare(*rhs->str_value) == 0) ? &Object::trueObject() : &Object::falseObject();
        else
            throw std::runtime_error("eq only accept bool, string and numeric operands");
    }

    inline Object* notEquals(Object* lhs, Object* rhs)
    {
        if (lhs == nullptr || rhs == nullptr || (lhs->otype != rhs->otype))
            throw std::runtime_error("ne only accept equal non-null types: bool, string and numeric");

        if (lhs->otype == TokenType::TypeBool && rhs->otype == TokenType::TypeBool)
            return lhs->bool_value != rhs->bool_value ? &Object::trueObject() : &Object::falseObject();
        else if (lhs->isNumeric() && rhs->isNumeric())
        {
            std::function<int(mpz_t&, mpz_t&)> mpz_op = [&](mpz_t &a, mpz_t &b) { return mpz_cmp(a, b); };
            std::function<int(mpf_t&, mpf_t&)> mpf_op = [&](mpf_t &a, mpf_t &b) { return mpf_cmp(a, b); };

            return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) != 0 ? &Object::trueObject() : &Object::falseObject();
        }
        else if (lhs->otype == TokenType::TypeString && rhs->otype == TokenType::TypeString)
            return (lhs->str_value->compare(*rhs->str_value) != 0) ? &Object::trueObject() : &Object::falseObject();
        else
            throw std::runtime_error("!= only accept bool and numeric operands");
    }

    inline Object* less(Object* lhs, Object* rhs)
    {
        std::function<int(mpz_t&, mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b) {return mpz_cmp(a,b);};
        std::function<int(mpf_t&, mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b) {return mpf_cmp(a,b);};

        return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) < 0 ? &Object::trueObject() : &Object::falseObject();
    }

    inline Object* lessEqual(Object* lhs, Object* rhs)
    {
        std::function<int(mpz_t&, mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b) {return mpz_cmp(a,b);};
        std::function<int(mpf_t&, mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b) {return mpf_cmp(a,b);};

        return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) <= 0 ? &Object::trueObject() : &Object::falseObject();
    }

    inline Object* greater(Object* lhs, Object* rhs)
    {
        std::function<int(mpz_t&, mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b) {return mpz_cmp(a,b);};
        std::function<int(mpf_t&, mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b) {return mpf_cmp(a,b);};

        return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) > 0 ? &Object::trueObject() : &Object::falseObject();
    }

    inline Object* greaterEqual(Object* lhs, Object* rhs)
    {
        std::function<int(mpz_t&, mpz_t&)> mpz_op = [&] (mpz_t& a, mpz_t& b) {return mpz_cmp(a,b);};
        std::function<int(mpf_t&, mpf_t&)> mpf_op = [&] (mpf_t& a, mpf_t& b) {return mpf_cmp(a,b);};

        return mp_cmp_op2(lhs, rhs, mpz_op, mpf_op) >= 0 ? &Object::trueObject() : &Object::falseObject();
    }

    inline void setFlag(int flag)    
    {
        flags |= flag;
    }

    inline void clearFlag(int flag)
    {
        flags &= ~(flag);
    }

    inline bool hasFlag(int flag) const
    {
        return (flags & flag) != 0;
    }

    /**
     * Evaluate the object. The base class version simply returns itself, while
     * expression subclasses return the value of the evaluation.
     */
    virtual Object* eval();

    /**
    * \return Verbose string representation of this object
    */
    virtual std::string dump() const;

    /**
     * \return Short string representation of this object.
     */
    virtual std::string toString() const;

    /**
     * Produce an external string representation. The string must be parsable.
     *
     * Externalization must produce a canonical represenation. For instance, if two
     * files are parsed containing "push int 255" and "push int 0xFF" respectively,
     * the externalization of both will be "push in 255", the canonical representation.
     *
     * A canonical representation makes it possible to compare and de-duplicate functions.
     */
    virtual void externalize(std::ostream& str, int indentation=0) const;

    std::string typestring() const;
    std::string typestring(TokenType otype) const;

    inline bool isString() const
    {
        return otype == TokenType::TypeString;
    }

    inline bool isSymbol() const
    {
        return otype == TokenType::TypeSymbol;
    }

    inline bool isInteger() const
    {
        return otype >= TokenType::TypeInt && otype < TokenType::TypeFloat;
    }

    inline bool isFloat() const
    {
        return otype >= TokenType::TypeFloat && otype <= TokenType::TypeFloat;
    }

    inline bool isNumeric() const
    {
        return isInteger() || isFloat();
    }

    inline bool isArray() const
    {
        return otype == TokenType::TypeArray;
    }

    inline bool isUnorderedSet() const
    {
        return otype == TokenType::TypeUnorderedSet;
    }

    inline bool isUnorderedMap() const
    {
        return otype == TokenType::TypeUnorderedMap;
    }

    inline bool isPair() const
    {
        return otype == TokenType::TypePair;
    }

    inline bool isProjection() const
    {
        return otype == TokenType::TypeProjection;
    }

    /**
     * A container holding other Object's
     */
    inline bool isContainer() const
    {
        return isArray() || isUnorderedSet() || isUnorderedMap() || isPair() || isProjection();
    }

    /**
     * A sequence is anything that the "coll" functions can operate on,
     * namely containers and strings.
     */
    inline bool isSequence() const
    {
        return isContainer() || isString();
    }

    /**
     * In-place conversion
     */
    void castTo(TokenType target);

    /**
     * Call to let the vm track this object. If not found on any stacks, it will
     * be elligible for collection. Never call this for pinned objects.
     */
    void track();

    /**
     * Called by the GC to mark this object as reachable. Override to mark owned objects.
     *
     * Overrides should always call this base class method first to avoid cycles.
     */
    virtual void mark();
};

inline Object* track(Object* obj)
{
    obj->track();
    return obj;
}

#ifdef WIN32
    #pragma pack(pop)
#endif

}//ns


#include "StdUtil.h"
namespace std
{

// TODO: specialize std::less<lake::Object*> for ordering

// Object equality testing
template <> struct equal_to<lake::Object*>
{
    bool operator()(const lake::Object* obj, const lake::Object* other) const
    {
        bool res = obj->otype == other->otype;
        if (!res)
            return false;

        // Comparing collections means comparing pointers

        if (obj->otype == lake::TokenType::TypeString)
        {
            res = obj->str_value->compare(*other->str_value) == 0;
        }
        else if (obj->otype == lake::TokenType::TypeInt)
        {
            res = mpz_cmp(obj->mpz, other->mpz) == 0;
        }
        else if (obj->otype == lake::TokenType::TypeFloat)
        {
            // TODO: configurable epsilon
            res = mpf_cmp(obj->mpf, other->mpf) == 0;
        }
        else if (obj->otype == lake::TokenType::TypeBool)
        {
            res = obj->bool_value == other->bool_value;
        }
        else if (obj->otype == lake::TokenType::TypeChar)
        {
            res = obj->char_value == other->char_value;
        }
        else if (obj->otype == lake::TokenType::TypeFunction)
        {
            res = obj->fndata == other->fndata;
        }
        else if (obj->otype == lake::TokenType::TypeObject)
        {
            res = obj == other;
        }
        else if (obj->otype == lake::TokenType::TypeViewPointer)
        {
            res = obj->ptr_value == other->ptr_value;
        }
        else if(obj->otype == lake::TokenType::TypeFFISymbol)
        {
            res = obj->symdata->name.compare(other->symdata->name) == 0;
        }
        else if (obj->otype == lake::TokenType::TypePair)
        {
            res = obj->pair == other->pair;
        }
        else if (obj->otype == lake::TokenType::TypeArray)
        {
            res = obj->array == other->array;
        }
        else if (obj->otype == lake::TokenType::TypeUnorderedMap)
        {
            res = obj->umap == other->umap;
        }
        else if (obj->otype == lake::TokenType::TypeUnorderedSet)
        {
            res = obj->uset == other->uset;
        }
        else
        {
            throw std::runtime_error("Unsupported equality test");
        }

        return res;
    }
};

template <> struct hash<lake::Object*>
{
    size_t operator()(lake::Object* o) const
    {
        std::size_t seed = 0;

        hash_combine(seed, o->otype);

        if (o->otype == lake::TokenType::TypeString)
        {
            hash_combine(seed, *o->str_value);
        }
        else if (o->otype == lake::TokenType::TypeInt)
        {
            hash_combine(seed, o->toString());
        }
        else if (o->otype == lake::TokenType::TypeFloat)
        {
            hash_combine(seed, o->toString());
        }
        else if (o->otype == lake::TokenType::TypeViewPointer)
        {
            hash_combine(seed, (ptrdiff_t)o->ptr_value);
        }
        else if (o->otype == lake::TokenType::TypeFunction)
        {
            hash_combine(seed, (ptrdiff_t)o->fndata);
        }
        else if (o->otype == lake::TokenType::TypeObject)
        {
            hash_combine(seed, (ptrdiff_t)o);
        }
        else if (o->otype == lake::TokenType::TypePair)
        {
            hash_combine(seed, (ptrdiff_t)o->pair);
        }
        else if (o->otype == lake::TokenType::TypeArray)
        {
            hash_combine(seed, (ptrdiff_t)o->array);
        }
        else if (o->otype == lake::TokenType::TypeUnorderedMap)
        {
            hash_combine(seed, (ptrdiff_t)o->umap);
        }
        else if (o->otype == lake::TokenType::TypeUnorderedSet)
        {
            hash_combine(seed, (ptrdiff_t)o->uset);
        }
        else if (o->otype == lake::TokenType::TypeFFISymbol)
        {
            hash_combine(seed, o->symdata->name);
        }
        else if (o->otype == lake::TokenType::TypeChar)
        {
            hash_combine(seed, o->char_value);
        }
        else if (o->otype == lake::TokenType::TypeBool)
        {
            hash_combine(seed, o->bool_value);
        }
        else
        {
            throw std::runtime_error("Unsupported hash");
        }

        return seed;
    }
};
}
#endif //LAKE_OBJECT_H
