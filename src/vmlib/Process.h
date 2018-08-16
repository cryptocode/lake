#ifndef LAKE_PROCESS_H
#define LAKE_PROCESS_H

#include <vector>
#include <mutex>
#include <map>
#include <string>
#include <cinttypes>
#include <unordered_map>
#include "../vmffi/Loader.h"
#include "AsmLexer.h"
#include "DebugInfo.h"

namespace lake {

#define trace_stack()  {if (Process::instance().traceStack) vm().dumpStack();}
#define trace_debug(s) {if (Process::instance().traceLevel >= Process::DEBUG) printf(s "\n");}
#define trace_debugf(s,...) {if (Process::instance().traceLevel >= Process::DEBUG) printf(s "\n", ##__VA_ARGS__);}
#define trace_info(s) {if (Process::instance().traceLevel >= Process::INFO) printf(s "\n");}
#define trace_infof(s,...) {if (Process::instance().traceLevel >= Process::INFO) printf(s "\n", ##__VA_ARGS__);}

class Object;
class VM;
class ExprExpressionList;

/**
 * Process wide state, shared between multiple VM instances.
 */
class Process
{
public:

    enum TraceLevel {OFF=0, INFO, WARN, DEBUG};

    /**
     * This singleton idiom is both valid and threadsafe from C++11 onwards (cppstd §6.7.4)
     */
    static inline Process& instance()
    {
        static Process instance;
        return instance;
    }

    /**
     * The VM currently associated with the executing thread. Usually there's
     * on thread per VM, but the implementation (and specification) allows for
     * multiplexing multiple VM's on a single thread. In that case, the vm field
     * is updated when the scheduler swaps VM on a thread.
     */
    /*__thread*/ VM* vm = nullptr;

    int traceLevel = TraceLevel::OFF;

    bool traceStack = false;

    bool debugInfo = false;

    /**
     * Side channel with debug info. We *could* put a DebugInfo ptr into Object*, but then
     * we would pay the price of 8 bytes on 64-bit machines, for every object, even when debug
     * mode was off.
     *
     * Key is address of expression list entry (since expression
     * objects themselves can be singletons/reused, we need actual expression-location as key)
     */
    std::unordered_map<StableExprListReference, DebugInfo> debugMap;

    /**
     * Return the filename at the given debuginfo index, or string("") if not found.
     */
    std::string filenameByIndex(size_t index)
    {
        if (index < filenames.size())
        {
            return filenames.at(index);
        }

        return "";
    }

    size_t filenameCount()
    {
        return filenames.size();
    }

    void addFilename(std::string filename)
    {
        filenames.push_back(filename);
    }

    void registerLibrary(std::string alias, VMFFI_MOD_TYPE lib)
    {
        libmap[alias] = lib;
    }

    /** Get library by alias. The alias MUST exist. */
    VMFFI_MOD_TYPE getLibrary(std::string alias)
    {
        return libmap[alias];
    }

    int getExitCode() const
    {
        return exitCode;
    }

    void setExitCode(int exitCode)
    {
        Process::exitCode = exitCode;
    }

private:

    Process();

    /**
     * A map of named closures in this VM. These can be executed directly in case of a simple function call, or
     * new instances can be created by cloning the Closure.
     */
    std::map<std::string, Object*> closures;
    std::mutex closuresMutex;

    /**
     * File names being parsed or executed. Used to display debug information.
     */
    std::vector<std::string> filenames;

    std::unordered_map<std::string, VMFFI_MOD_TYPE> libmap;

    int exitCode;
};

/**
 * Returns the current VM
 */
inline VM& vm()
{
    return *(Process::instance().vm);
}

}//ns

#endif //LAKE_PROCESS_H

