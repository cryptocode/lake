#ifndef LAKE_LOADER_H
#define LAKE_LOADER_H

#include <boost/predef.h>
#include <boost/algorithm/string/predicate.hpp>
#include <unordered_map>
#include <string>

#if (BOOST_OS_WINDOWS)

    #include <windows.h>

    #define VMFFI_MOD_TYPE HMODULE
    #define VMFFI_MOD_NULL 0
    #define VMFFI_PROC_TYPE FARPROC
    #define VMFFI_PROC_NULL 0

#else

    #include <dlfcn.h>

    #define VMFFI_MOD_TYPE void*
    #define VMFFI_MOD_NULL nullptr
    #define VMFFI_PROC_TYPE void*
    #define VMFFI_PROC_NULL nullptr

#endif

namespace lake {

/**
 * Load dynamic library, or the current executable
 */
class ModuleLoader
{
public:

    static ModuleLoader& instance()
    {
        static ModuleLoader loader;
        return loader;
    }

    /**
     * Load the given library path. If an empty string is passed, the current executable is assumed.
     */
    VMFFI_MOD_TYPE loadModule(std::string moduleName)
    {
        VMFFI_MOD_TYPE mod = VMFFI_MOD_NULL;

        std::string path = moduleName;
        std::string ext;

        if (path.length() > 0)
        {
            #if (BOOST_OS_WINDOWS)
                ext = ".dll";
            #elif (BOOST_OS_MACOS)
                ext = ".dylib";
            #else
                ext = ".so";
            #endif

            if (!boost::algorithm::iends_with(path, ext))
                path += ext;
        }

        auto match = modules.find(path);
        if (match == modules.end())
        {
            #if (BOOST_OS_WINDOWS)

                if (moduleName.length() == 0)
                {
                    // Current process
                    mod = GetModuleHandle(0);
                }
                else
                {
                    mod = LoadLibrary(path.c_str());
                }

                if (mod == NULL)
                    return VMFFI_MOD_NULL;

            #else

                // If opening current exec, -rdynamic / --dynamic_list, and link with -pic ?
                // http://stackoverflow.com/questions/6292473/how-to-call-function-in-executable-from-my-library/6298434#6298434
                mod = dlopen(path.length() == 0 ? nullptr : path.c_str(), RTLD_LAZY);

                if (!mod)
                    return VMFFI_MOD_NULL;
            #endif

            // Cache it
            modules[moduleName] = mod;
        }
        else
        {
            // Cache lookup matched
            mod = match->second;
        }

        return mod;
    }

private:
    ModuleLoader(){}

    std::unordered_map<std::string, VMFFI_MOD_TYPE> modules;
};

/**
 * Load symbol from given library
 */
class SymbolLoader
{
public:

    static SymbolLoader& instance()
    {
        static SymbolLoader instance;
        return instance;
    }

    VMFFI_PROC_TYPE loadSymbol(VMFFI_MOD_TYPE module, std::string sym)
    {
        VMFFI_PROC_TYPE res = VMFFI_PROC_NULL;

        #if (BOOST_OS_WINDOWS)

            res = GetProcAddress(module, sym.c_str());

        #else

            res = dlsym(module, sym.c_str());

        #endif

        return res;
    }

private:
    SymbolLoader() {}
};

}//ns

#endif
