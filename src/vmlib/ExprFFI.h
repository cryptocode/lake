#ifndef LAKE_EXPRFFI_H
#define LAKE_EXPRFFI_H

#include "Object.h"
#include "Process.h"
#include "VM.h"
#include "ffi.h"
#include "../vmffi/Loader.h"
#include <vector>
#include <algorithm>
#include <stdlib.h>

namespace lake
{

// https://github.com/tromey/libffi/commit/38a4d72c95936d27cba1ac6e84e3094ffdfaa77c

/* Perform machine independent initialization of aggregate type
   specifications. */

#define ALIGN(v, a)  (((((size_t) (v))-1) | ((a)-1))+1)

    static ffi_status initialize_aggregate(ffi_type *arg, size_t *offsets)
    {
        ffi_type **ptr;

        if ((arg == NULL || arg->elements == NULL))
            return FFI_BAD_TYPEDEF;

        arg->size = 0;
        arg->alignment = 0;

        ptr = &(arg->elements[0]);

        if (ptr == 0)
            return FFI_BAD_TYPEDEF;

        while ((*ptr) != NULL)
        {
            if ((((*ptr)->size == 0)
                 && (initialize_aggregate((*ptr), NULL) != FFI_OK)))
                return FFI_BAD_TYPEDEF;

            arg->size = ALIGN(arg->size, (*ptr)->alignment);
            if (offsets)
                *offsets++ = arg->size;
            arg->size += (*ptr)->size;

            arg->alignment = (arg->alignment > (*ptr)->alignment) ?
                             arg->alignment : (*ptr)->alignment;

            ptr++;
        }

        /* Structure size includes tail padding.  This is important for
           structures that fit in one register on ABIs like the PowerPC64
           Linux ABI that right justify small structs in a register.
           It's also needed for nested structure layout, for example
           struct A { long a; char b; }; struct B { struct A x; char y; };
           should find y at an offset of 2*sizeof(long) and result in a
           total size of 3*sizeof(long).  */
        arg->size = ALIGN (arg->size, arg->alignment);

        /* On some targets, the ABI defines that structures have an additional
           alignment beyond the "natural" one based on their elements.  */
#ifdef FFI_AGGREGATE_ALIGNMENT
        if (FFI_AGGREGATE_ALIGNMENT > arg->alignment)
        arg->alignment = FFI_AGGREGATE_ALIGNMENT;
#endif

        if (arg->size == 0)
            return FFI_BAD_TYPEDEF;
        else
            return FFI_OK;
    }

    static ffi_status ffi_get_struct_offsets(ffi_abi abi, ffi_type *struct_type, size_t *offsets)
    {
        if (!(abi > FFI_FIRST_ABI && abi < FFI_LAST_ABI))
            return FFI_BAD_ABI;
        if (struct_type->type != FFI_TYPE_STRUCT)
            return FFI_BAD_TYPEDEF;

#if HAVE_LONG_DOUBLE_VARIANT
        ffi_prep_types (abi);
#endif

        return initialize_aggregate(struct_type, offsets);
    }

    static ffi_type *toFFIType(TokenType ttype)
    {
        ffi_type *type;

        if (ttype == TokenType::TypeObject)
            type = nullptr;
        else if (ttype == TokenType::TypeViewPointer)
            type = &ffi_type_pointer;
        else if (ttype == TokenType::TypeViewVoid)
            type = &ffi_type_void;
        else if (ttype == TokenType::TypeViewSint8)
            type = &ffi_type_sint8;
        else if (ttype == TokenType::TypeViewSint16)
            type = &ffi_type_sint16;
        else if (ttype == TokenType::TypeViewSint32)
            type = &ffi_type_sint32;
        else if (ttype == TokenType::TypeViewSint64)
            type = &ffi_type_sint64;
        else if (ttype == TokenType::TypeViewUint8)
            type = &ffi_type_uint8;
        else if (ttype == TokenType::TypeViewUint16)
            type = &ffi_type_uint16;
        else if (ttype == TokenType::TypeViewUint32)
            type = &ffi_type_uint32;
        else if (ttype == TokenType::TypeViewUint64)
            type = &ffi_type_uint64;
        else if (ttype == TokenType::TypeViewSchar)
            type = &ffi_type_schar;
        else if (ttype == TokenType::TypeViewSshort)
            type = &ffi_type_sshort;
        else if (ttype == TokenType::TypeViewSint)
            type = &ffi_type_sint;
        else if (ttype == TokenType::TypeViewSlong)
            type = &ffi_type_slong;
        else if (ttype == TokenType::TypeViewUchar)
            type = &ffi_type_uchar;
        else if (ttype == TokenType::TypeViewUshort)
            type = &ffi_type_ushort;
        else if (ttype == TokenType::TypeViewUint)
            type = &ffi_type_uint;
        else if (ttype == TokenType::TypeViewUlong)
            type = &ffi_type_ulong;
        else if (ttype == TokenType::TypeViewFloat)
            type = &ffi_type_float;
        else if (ttype == TokenType::TypeViewDouble)
            type = &ffi_type_double;
        else if (ttype == TokenType::TypeViewLongDouble)
            type = &ffi_type_longdouble;
        else
            throw std::runtime_error("FFI type not supported");

        return type;
    }

    // For externalization
    static const char *toFFITypeString(TokenType ttype)
    {
        const char *type;

        if (ttype == TokenType::TypeObject)
            type = TOK_TYPEOBJECT;
        else if (ttype == TokenType::TypeViewPointer)
            type = TOK_TYPEVIEWPOINTER_UNDERSCORE;
        else if (ttype == TokenType::TypeViewVoid)
            type = TOK_TYPEVIEWVOID;
        else if (ttype == TokenType::TypeViewSint8)
            type = TOK_TYPEVIEWSINT8;
        else if (ttype == TokenType::TypeViewSint16)
            type = TOK_TYPEVIEWSINT16;
        else if (ttype == TokenType::TypeViewSint32)
            type = TOK_TYPEVIEWSINT32;
        else if (ttype == TokenType::TypeViewSint64)
            type = TOK_TYPEVIEWSINT64;
        else if (ttype == TokenType::TypeViewUint8)
            type = TOK_TYPEVIEWUINT8;
        else if (ttype == TokenType::TypeViewUint16)
            type = TOK_TYPEVIEWUINT16;
        else if (ttype == TokenType::TypeViewUint32)
            type = TOK_TYPEVIEWUINT32;
        else if (ttype == TokenType::TypeViewUint64)
            type = TOK_TYPEVIEWUINT64;
        else if (ttype == TokenType::TypeViewSchar)
            type = TOK_TYPEVIEWSCHAR;
        else if (ttype == TokenType::TypeViewSshort)
            type = TOK_TYPEVIEWSSHORT;
        else if (ttype == TokenType::TypeViewSint)
            type = TOK_TYPEVIEWSINT;
        else if (ttype == TokenType::TypeViewSlong)
            type = TOK_TYPEVIEWSLONG;
        else if (ttype == TokenType::TypeViewUchar)
            type = TOK_TYPEVIEWUCHAR;
        else if (ttype == TokenType::TypeViewUshort)
            type = TOK_TYPEVIEWUSHORT;
        else if (ttype == TokenType::TypeViewUint)
            type = TOK_TYPEVIEWUINT;
        else if (ttype == TokenType::TypeViewUlong)
            type = TOK_TYPEVIEWULONG;
        else if (ttype == TokenType::TypeViewFloat)
            type = TOK_TYPEVIEWFLOAT;
        else if (ttype == TokenType::TypeViewDouble)
            type = TOK_TYPEVIEWDOUBLE;
        //else if (ttype == TokenType::TypeViewLongDouble)
        //    type = TOK_TYPEVIEWDOUBLE;
        else
            throw std::runtime_error("FFI type not supported");

        return type;
    }

    static TokenType fromFFIType(ffi_type *type)
    {
        TokenType res;

        // If the ffi_type is null, treat as Object* (useful to pass and return
        // raw Object pointers to and from the VM's stdlib)
        if (type == nullptr)
        {
            res = TokenType::TypeObject;
        }
        else if (type == &ffi_type_pointer)
        {
            res = TokenType::TypeViewPointer;
        }
        else if (type == &ffi_type_sint64)
        {
            res = TokenType::TypeViewSint64;
        }
        else if (type == &ffi_type_uint64)
        {
            res = TokenType::TypeViewUint64;
        }
        else if (type == &ffi_type_uint32)
        {
            res = TokenType::TypeViewUint32;
        }
        else if (type == &ffi_type_sint32)
        {
            res = TokenType::TypeViewSint32;
        }
        else if (type == &ffi_type_uint16)
        {
            res = TokenType::TypeViewUint16;
        }
        else if (type == &ffi_type_sint16)
        {
            res = TokenType::TypeViewSint16;
        }
        else if (type == &ffi_type_uint8)
        {
            res = TokenType::TypeViewUint8;
        }
        else if (type == &ffi_type_sint8)
        {
            res = TokenType::TypeViewSint8;
        }
        else if (type == &ffi_type_schar)
        {
            res = TokenType::TypeViewSchar;
        }
        else if (type == &ffi_type_uchar)
        {
            res = TokenType::TypeViewUchar;
        }
        else if (type == &ffi_type_sshort)
        {
            res = TokenType::TypeViewSshort;
        }
        else if (type == &ffi_type_ushort)
        {
            res = TokenType::TypeViewUshort;
        }
        else if (type == &ffi_type_sint)
        {
            res = TokenType::TypeViewSint;
        }
        else if (type == &ffi_type_uint)
        {
            res = TokenType::TypeViewUint;
        }
        else if (type == &ffi_type_slong)
        {
            res = TokenType::TypeViewSlong;
        }
        else if (type == &ffi_type_ulong)
        {
            res = TokenType::TypeViewUlong;
        }
        else if (type == &ffi_type_double)
        {
            res = TokenType::TypeViewDouble;
        }
        else if (type == &ffi_type_float)
        {
            res = TokenType::TypeViewFloat;
        }
        else
        {
            throw std::runtime_error("Unsupported ffi type");
        }

        return res;

    }

    class ExprFFILoad : public Object
    {
    public:
        ExprFFILoad(std::string path, std::string alias) : Object(TokenType::TypeOperation),
                                                           path(path), alias(alias)
        {}

        virtual Object *eval() override
        {
            if (alias.size() == 0)
            {
                // Recall that the lake calling convention is first-arg-pop'ed-first (hence pushed in reverse)
                path = *vm().pop()->str_value;
                alias = *vm().pop()->str_value;
            }

            Process::instance().registerLibrary(
                    alias, ModuleLoader::instance().loadModule(path));

            return nullptr;
        }

        void externalize(std::ostream &str, int indentation) const override
        {
            str << std::string(indentation, ' ') << TOK_FFI << " " << TOK_LIB << " \"" << path << "\" " << alias
                << std::endl;
        }

    private:
        std::string alias;
        std::string path;
    };

    class ExprFFICall : public Object
    {
    public:

        ExprFFICall() : Object(TokenType::TypeOperation)
        {}

        std::vector<Object*>* structToArray(StructData *sd, uint8_t *buf)
        {
            std::vector<Object*>* arr = new std::vector<Object*>();

            ffi_type struct_type;

            // Will contain the offsets of each element, suitable for the current platform
            size_t struct_offsets[sd->elementTypes.size()];

            ffi_type **struct_type_elements = new ffi_type *[sd->elementTypes.size() + 1];
            struct_type_elements[sd->elementTypes.size()] = nullptr;
            struct_type.size = struct_type.alignment = 0;
            struct_type.type = FFI_TYPE_STRUCT;
            struct_type.elements = struct_type_elements;

            size_t i = 0;
            for (auto sdit = std::begin(sd->elementTypes); sdit != std::end(sd->elementTypes); ++sdit)
            {
                struct_type_elements[i++] = (*sdit);
            }

            // Figure out where the struct elements are located, considering size and alignment
            ffi_get_struct_offsets(FFI_DEFAULT_ABI, &struct_type, struct_offsets);

            // Read from the buffer
            i = 0;
            for (auto sdit = std::begin(sd->elementTypes); sdit != std::end(sd->elementTypes); ++sdit)
            {
                ViewType vt;
                memcpy(&vt, buf + struct_offsets[i], (*sdit)->size);
                arr->push_back(toObject(vt, *sdit));
                i++;
            }

            //auto val = lake::track(Object::create(arr));
            return arr;
        }

        Object* toObject(ViewType u_res, ffi_type *type)
        {
            Object *obj = nullptr;

            if (type == nullptr)
            {
                // Return immediately so we don't start tracking
                return u_res._obj;
            }
            else if (type == &ffi_type_pointer)
            {
                obj = Object::create(true);
                obj->otype = TokenType::TypeViewPointer;

                // Works for string, for now, but "cast string" should convert to std::string

                // This pointer can be anything. Use the cast operator
                // to convert to something useful, e.g. "convert array" to destructure ffi structs.
                obj->ptr_value = u_res._ptr;

                // Do not deallocate data
                obj->setFlag(FLAG_FOREIGN);
            }
            else if (type == &ffi_type_sint64)
            {
                //int64_t val = (int64_t)callres;
                obj = Object::create((int64_t) u_res._sint64);
            }
            else if (type == &ffi_type_uint64)
            {
                obj = Object::create((uint64_t) u_res._uint64);
            }
            else if (type == &ffi_type_uint32)
            {
                obj = Object::create((uint64_t) u_res._uint32);
            }
            else if (type == &ffi_type_sint32)
            {
                obj = Object::create((int64_t) u_res._sint32);
            }
            else if (type == &ffi_type_uint16)
            {
                obj = Object::create((uint64_t) u_res._uint16);
            }
            else if (type == &ffi_type_sint16)
            {
                obj = Object::create((int64_t) u_res._sint16);
            }
            else if (type == &ffi_type_uint8)
            {
                obj = Object::create((uint64_t) u_res._uint8);
            }
            else if (type == &ffi_type_sint8)
            {
                obj = Object::create((uint64_t) u_res._sint8);
            }
            else if (type == &ffi_type_schar)
            {
                obj = Object::create((int64_t) u_res._schar);
            }
            else if (type == &ffi_type_uchar)
            {
                obj = Object::create((uint64_t) u_res._uchar);
            }
            else if (type == &ffi_type_sshort)
            {
                obj = Object::create((int64_t) u_res._sshort);
            }
            else if (type == &ffi_type_ushort)
            {
                obj = Object::create((uint64_t) u_res._ushort);
            }
            else if (type == &ffi_type_sint)
            {
                obj = Object::create((int64_t) u_res._sint);
            }
            else if (type == &ffi_type_slong)
            {
                obj = Object::create((uint64_t) u_res._slong);
            }
            else if (type == &ffi_type_double)
            {
                obj = Object::create((double) u_res._double);
            }
            else if (type == &ffi_type_float)
            {
                obj = Object::create((double) u_res._float);
            }
            else
            {
                throw std::runtime_error("Unsupported ffi return type");
            }

            // Track for gc
            return lake::track(obj);
        }

        virtual Object *eval() override
        {
            SymbolData *symData = vm().pop()->symdata;

            if (symData->sym == nullptr)
                throw std::runtime_error("Unknown FFI symbol");

            ffi_cif cif;

            // Pop args (in reverse on the stack, so iterate ffi types in reverse too) and convert to correct view type
            std::vector<void *> vectArgs;

            // Lifetime management
            std::vector<std::unique_ptr<uint8_t[]>> bufsForDeletion;
            std::vector<std::unique_ptr<ViewType>> viewTypesForDeletion;

            for (auto rit = std::rbegin(symData->ffiArgTypes); rit != std::rend(symData->ffiArgTypes); ++rit)
            {
                Object *arg = vm().pop();

                if (arg->otype == TokenType::TypeFFIStruct)
                {
                    StructData *sd = arg->structdata;
                    ffi_type structType;

                    // Will contain the offsets of each element suitable for the current architecture
                    size_t struct_offsets[sd->elementTypes.size()];

                    ffi_type **struct_type_elements = new ffi_type *[sd->elementTypes.size() + 1];
                    struct_type_elements[sd->elementTypes.size()] = nullptr;
                    structType.size = structType.alignment = 0;
                    structType.type = FFI_TYPE_STRUCT;
                    structType.elements = struct_type_elements;

                    size_t i = 0;
                    for (auto sdit = std::begin(sd->elementTypes); sdit != std::end(sd->elementTypes); ++sdit)
                    {
                        struct_type_elements[i++] = (*sdit);
                    }

                    ffi_get_struct_offsets(FFI_DEFAULT_ABI, &structType, struct_offsets);

                    // At this point, the struct size is known. Let's make a buffer representing the struct instance.
                    std::unique_ptr<uint8_t[]> bufptr = std::make_unique<uint8_t[]>(structType.size);
                    uint8_t *buf = bufptr.get();

                    // Make sure it gets deleted
                    bufsForDeletion.emplace_back(std::move(bufptr));

                    // ... copy members to correct (aligned) struct offset
                    memset(buf, 0, structType.size);

                    size_t idx = sd->elementTypes.size() - 1;
                    size_t offsetFromEnd = structType.size;
                    for (auto sdit = std::rbegin(sd->elementTypes); sdit != std::rend(sd->elementTypes); ++sdit)
                    {
                        Object *elem = vm().pop();

                        ViewType vt;
                        vt.fromObjectAndViewType(*elem, fromFFIType(*sdit));

                        memcpy(buf + struct_offsets[idx], &vt, struct_type_elements[idx]->size);
                        idx--;
                    }

                    vectArgs.push_back((void *) &buf);

                    delete[] struct_type_elements;
                }
                else
                {
                    std::unique_ptr<ViewType> pvt = std::make_unique<ViewType>();
                    ViewType *vt = pvt.get();
                    viewTypesForDeletion.emplace_back(std::move(pvt));

                    vt->fromObjectAndViewType(*arg, fromFFIType(*rit));
                    vectArgs.push_back((void *) vt);
                }
            }

            std::reverse(vectArgs.begin(), vectArgs.end());

            // Prepare for call. We must first replace nullptr ffi types with pointer (nullptr communicates Object*)
            std::vector<ffi_type *> typesCopy;// = symData->ffiArgTypes;
            typesCopy.resize(symData->ffiArgTypes.size());

            std::transform(symData->ffiArgTypes.begin(), symData->ffiArgTypes.end(),
                           typesCopy.begin(),
                           [](ffi_type *x)
                           { return x == nullptr ? &ffi_type_pointer : x; });

            // We need to replace nullptr with &ffi_type_pointer. This reason is a bit subtle... We need to
            // be able to pass Object*, which is a pointer, but not the Object#ptr_value. So in symData,
            // we communicate our desire to pass Object* with a nullptr ffi type. But for our actual ffi call, we
            // must of course specify that Object* is to passed as a ffi_type_pointer.
            if (FFI_OK != ffi_prep_cif(&cif, FFI_DEFAULT_ABI, symData->ffiArgTypes.size(),
                                       symData->ffiRetType != nullptr ? symData->ffiRetType : &ffi_type_pointer,
                                       typesCopy.data()))
                throw std::runtime_error("ffi_prep_cif failed");

            ViewType u_res;
            ffi_call(&cif, FFI_FN(symData->sym), (symData->ffiRetType != &ffi_type_void) ? &u_res : nullptr,
                     vectArgs.size() > 0 ? vectArgs.data() : nullptr);

            // Wrap the return value, if any, in an Object and push it on the stack
            if (symData->ffiRetType != &ffi_type_void)
            {
                vm().push(toObject(u_res, symData->ffiRetType));
            }

            return nullptr;
        }

        void externalize(std::ostream &str, int indentation) const override
        {
            str << std::string(indentation, ' ') << TOK_FFI << " " << TOK_CALL << std::endl;
        }
    };

    /**
     * Defines a struct
     */
    class ExprFFIStruct : public Object
    {
    public:

        ExprFFIStruct(std::string name, std::vector<TokenType> elemenTypes)
                : Object(TokenType::TypeOperation), structname(name), elemenTypes(std::move(elemenTypes))
        {}

        virtual Object *eval() override
        {
            StructData *sd = new StructData(structname);

            for (const auto &ttype : elemenTypes)
            {
                sd->elementTypes.push_back(toFFIType(ttype));
            }

            vm().push(lake::track(Object::create(sd)));

            return nullptr;
        }

        void externalize(std::ostream &str, int indentation) const override
        {
            str << std::string(indentation, ' ') << TOK_FFI << " " << TOK_STRUCT << " " << structname;

            for (auto T : elemenTypes)
            {
                str << " " << toFFITypeString(T);
            }

            str << std::endl;
        }

    private:
        // Not really used as struct defs are placed on the stack/in arrays, etc, still useful for debugging
        std::string structname;
        std::vector<TokenType> elemenTypes;
    };

    class ExprFFISym : public Object
    {
    public:
        ExprFFISym(std::string libalias, std::string sym, std::vector<TokenType> args)
                : Object(TokenType::TypeOperation), sym(sym), libalias(libalias), args(std::move(args))
        {}

        virtual Object *eval() override
        {
            auto lib = Process::instance().getLibrary(libalias);
            auto symbol = SymbolLoader::instance().loadSymbol(lib, sym);

            ffi_type *ffiRetType;
            std::vector<ffi_type *> ffiArgTypes;

            int count = 0;
            for (const auto &ttype : args)
            {
                ffi_type *type = toFFIType(ttype);

                if (count == 0)
                    ffiRetType = type;
                else
                    ffiArgTypes.push_back(type);

                count++;
            }

            vm().push(lake::track(Object::create(new SymbolData(lib, symbol, ffiRetType, std::move(ffiArgTypes), sym))));

            return nullptr;
        }

        void externalize(std::ostream &str, int indentation) const override
        {
            str << std::string(indentation, ' ') << TOK_FFI << " " << TOK_SYM << " " << libalias << " " << sym;

            for (auto T : args)
            {
                str << " " << toFFITypeString(T);
            }

            str << std::endl;
        }

    private:
        std::string sym;
        std::string libalias;
        std::vector<TokenType> args;
    };

}//ns

#endif //LAKE_EXPRFFI_H
