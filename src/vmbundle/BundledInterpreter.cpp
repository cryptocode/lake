#include <iostream>
#include <sstream>
#include "../vmffi/Loader.h"
#include "../vmlib/Bundles.h"
#include "../vmlib/VM.h"
#include "../vmlib/AsmParser.h"

using namespace lake;

int main(int argc, const char * argv[])
{
    BundleReader reader;

    if (!reader.read())
    {
        std::cout << "No bundle found" << std::endl;
    }
    else
    {
        for (const auto& entry : reader.resources)
        {
            auto& res = entry.second;
            std::cout << "Resource: " << entry.first.c_str() << ", len: " << res->length << std::endl;

            VM vm;

            std::istringstream str(std::string(res->data));
            AsmParser(vm).parse(str, res->path);

            vm.eval();
        }
    }

    return 0;
}
