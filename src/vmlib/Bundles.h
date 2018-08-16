#ifndef LAKE_BUNDLER_H
#define LAKE_BUNDLER_H

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace lake {

// A valid bundle executable must end with this marker (externalized as big endian)
static constexpr uint64_t bundleMarker = 0x12f91c8e3d1f62c2;

/**
 * Embeds resources at the end of the interpreter executable in a form
 * that enables the executable itself to extract the resources into memory using
 * the BundleReader.
 *
 * Resources can be sources files, ast bit code and other static resources needed by
 * the application.
 */
class BundleWriter
{
public:

    BundleWriter(std::string output, std::string interpreterPath, std::vector<std::string> input)
            : output(output), interpreterPath(interpreterPath), input(input)
    {
    }

    void execute();

private:

    std::string output;
    std::string interpreterPath;
    std::vector<std::string> input;

};

struct Resource
{
    std::string path;
    uint64_t length;
    char* data = nullptr;

    Resource(std::string path, uint64_t length, char* data) : path(path), length(length), data(data){}
    ~Resource()
    {
        if (data != nullptr)
            delete [] data;

        data = nullptr;
    }
};

class BundleReader
{
public:

    /**
     * Read bundle. Returns true if the executable had a valid bundle attached to it,
     * otherwise false.
     */
    bool read();

    std::map<std::string, std::unique_ptr<Resource>> resources;
};

class BundleUtil
{
public:

    std::string getExecutablePath();

    static BundleUtil& instance()
    {
        static BundleUtil instance;
        return instance;
    }

private:

    BundleUtil() {}

    std::string path;
};

}// ns

#endif //LAKE_BUNDLER_H
