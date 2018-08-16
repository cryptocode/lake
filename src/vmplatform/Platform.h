#ifndef GC_PLATFORM_H
#define GC_PLATFORM_H

#include <string>

namespace lake {

/**
 * Operating System specific functionality
 */
class OS
{
public:

    static OS& instance()
    {
        static OS instance;
        return instance;
    }

    std::string getHomeDir();

    char getPathSeparator();

    /**
     * Data directory for the application. This is <home>/<appname>
     *
     * If an app name was not set when the VM bundle was created, an exception is thrown.
     */
    std::string getDataDir();

private:

    OS(){}
    std::string homeDirectory;
    std::string dataDirectory;

};

}//ns

#endif //GC_PLATFORM_H
