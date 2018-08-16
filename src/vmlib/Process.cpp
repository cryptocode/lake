#include "Process.h"
#include "VM.h"
#include "ExprExpressionList.h"
#include <iostream>

namespace lake {

void unexpectedHandler()
{
    std::cerr << "Unhandled exception thrown: " << std::endl;
    
#ifndef _WIN32
    static int tried_throw = false;

    try {
        if (!tried_throw++) throw;
        std::cerr << "No active exception found." << std::endl;
    }
    catch (const char* str)
    {
        std::cerr << "\"" << str << "\"" << std::endl;
    }
    catch (int val){
        std::cerr << "Exception code: " << val << std::endl;
    }
    catch (const std::runtime_error &err) {
        std::cerr << "Runtime error: " << err.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Exception type unknown." << std::endl;
    }
#endif

    // Do not call std::terminate here, as that would cause a loop into this set_terminate handler
    exit(1);
}

Process::Process()
{
    // For exception spec violations
    std::set_unexpected(unexpectedHandler);

    // For unhandled exceptions
    std::set_terminate(unexpectedHandler);
}


}//ns
