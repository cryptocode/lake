#include <iostream>
#include <sstream>
#include <fstream>
#include "../vmlib/Object.h"
#include "../vmlib/OptParser.h"
#include "../vmlib/AsmParser.h"
#include "../vmlib/Bundles.h"
#include "../vmplatform/Platform.h"

using namespace std;

using namespace lake;

int main(int argc, const char * argv[])
{
    int exitcode = 0;

    OptParser opt;
    opt.addOption("help", "h", "Display usage information");
    opt.addOption("version", "v", "Display version information");
    opt.addOption("trace", "t", "Trace execution level, 0 (off)..5 (high)", 1);
    opt.addOption("tracestack", "", "Display stack content");
    opt.addOption("run", "r", "Execute files directly. Specify input files with --source.");
    opt.addOption("source", "s", "Parse input file(s)", -1);
    opt.addOption("externalize", "", "Write assembly to file", 1);
    opt.addOption("dbg", "", "Include and use debug information. This may substantially affect performance.", -1);
    opt.addOption("appname", "", "Optional application name. Used as user subdir when exporting resources.", 1);
    opt.addOption("resource", "", "Embed these files", -1);
    opt.addOption("build", "b", "Create a binary with the given name. Requires --build-interpreter and --source, optionally --resource.", 1);
    opt.addOption("build-interpreter","", "Path to interpreter. The source input will be compressed and added to this executable.",1);
    opt.addOption("exec", "e", "Execute bundle attached to this executable");

    const char* error = opt.parse(argc, argv);
    if(error)
    {
        printf("Invalid options: %s\n", error);
        return 1;
    }

    if(opt.hasOption("help"))
    {
        printf("Lake Virtual Machine\n\n");
        opt.printUsage();
        return 0;
    }

    if (opt.hasOption("trace"))
    {
        Process::instance().traceLevel = std::stoi(opt.getFirstValue("trace"));
    }

    if (opt.hasOption("tracestack"))
    {
        Process::instance().traceStack = true;
    }

    if (opt.hasOption("dbg"))
    {
        Process::instance().debugInfo = true;
    }

    if(opt.hasOption("version"))
    {
        std::cout << "Version 1.0, Built " __DATE__ "  " __TIME__ << std::endl << std::endl;

        std::cout << "Details:" << std::endl;
        std::cout << "   mpir: " << __MPIR_VERSION << "." << __MPIR_VERSION_MINOR << "." << __MPIR_VERSION_PATCHLEVEL << std::endl;
        std::cout << "  boost: " << (BOOST_VERSION / 100000) << "." << ((BOOST_VERSION / 100) % 1000) << "." << (BOOST_VERSION % 100) <<  std::endl;

        std::cout << "   path: " << BundleUtil::instance().getExecutablePath().c_str() << std::endl;
        std::cout << "   home: " << OS::instance().getHomeDir().c_str() << std::endl;
        std::cout << "   data: " << OS::instance().getDataDir().c_str() << std::endl;

        return 0;
    }

    auto start = chrono::steady_clock::now();

    // Test self-execution. An actual bundle executable will pass arguments directly to the VM.
    if (opt.hasOption("exec"))
    {
        trace_info("Loading bundle...");

        BundleReader r;

        if (!r.read())
        {
            std::cout << "No bundle found" << std::endl;
        }
        else
        {
            size_t fileIndex;
            for (const auto& entry : r.resources)
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

    // This code is basically all a hosting interpreter frontend needs to do in order to
    // parse and execute generated Lake code. This allows vmlib to be linked, along with runtime
    // libraries for the host language, producing a single binary for the frontend.
    if (opt.hasOption("run") || opt.hasOption("build") || opt.hasOption("externalize"))
    {
        try
        {
            VM vm;
            Object::setDefaultPrecision(128);

            // Only parses the first file (TODO: the rest...)
            AsmParser(vm).parse(opt.getFirstValue("source"));

            // Externalization must occur before evaluation, since AST nodes may be reused, GC'ed and
            // transformed in arbitrary ways during execution.
            if (opt.hasOption("externalize"))
            {
                std::string filename = opt.getFirstValue("externalize");
                std::ofstream os(filename);
                vm.externalize(os);
                os.close();
            }

            if (opt.hasOption("run"))
            {
                vm.eval();
            }
            else if (opt.hasOption("build"))
            {
                if (opt.hasOption("build-interpreter"))
                {
                    BundleWriter bw(opt.getFirstValue("build"), opt.getFirstValue("build-interpreter"), opt.getOption("source")->values);
                    bw.execute();
                }
                else
                {
                    std::cout << "You must specifiy --build-interpreter" << std::endl;
                    return 0;
                }
            }

            if (Process::instance().traceLevel >= Process::DEBUG)
            {
                std::cout << "Execution completed successfully" << std::endl;
            }
        }
        catch (std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            exitcode=1;
        }
        catch (...)
        {
            std::cout << "Unknown exception occurred" << std::endl;
            exitcode=1;
        }
    }

    auto end = chrono::steady_clock::now();
    auto diff = end - start;

    if (Process::instance().traceLevel > 1)
    {
        std::cout << "Execution time: " << chrono::duration <double, milli> (diff).count() << " ms" << std::endl;
    }

    return exitcode;
}
