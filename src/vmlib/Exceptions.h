#ifndef LAKE__EXCEPTIONS_H
#define LAKE__EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <sstream>
#include "AsmLexer.h"
#include "DebugInfo.h"
#include "Process.h"

using namespace std::string_literals;

namespace lake {

/**
 * Thrown if the assembly lexer or parser detects an error in the source input.
 *
 * The what() override contains a descriptive error message, prefixed with location
 * information.
 */
class AsmException : public std::exception
{
public:

    AsmException(std::string msg, Location loc) : std::exception()
    {
        this->msg = msg;
        this->loc = loc;
    }

    virtual ~AsmException() { }

    virtual const char* what() const noexcept override
    {
        std::ostringstream str;
        str << Process::instance().filenameByIndex(loc.getFileIndex())
            <<  " [" << loc.getLine() << ":" << loc.getCol() << "] " << msg;
        reason = str.str();

        return reason.c_str();
    }

private:

    mutable std::string reason;
    mutable std::string msg;
    mutable Location loc;
};

/**
 * Thrown during VM expression list evaluation, along with any debug information.
 */
class EvalException : public std::exception
{
public:

    EvalException(std::string msg, DebugInfo di) : std::exception()
    {
        this->msg = msg;
        this->di = di;
    }

    virtual ~EvalException() { }

    virtual const char* what() const noexcept override
    {
        std::string fname = Process::instance().filenameByIndex(di.loc.getFileIndex());
        std::ostringstream str;
        str << fname << " [" << di.loc.getLine() << ":" << di.loc.getCol() << "] " << msg;
        reason = str.str();

        return reason.c_str();
    }

private:

    mutable std::string reason;
    mutable std::string msg;
    mutable DebugInfo di;
};

}//ns

#endif //LAKE__EXCEPTIONS_H
