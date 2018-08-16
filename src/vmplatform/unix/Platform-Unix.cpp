#include <string>
#include <stdlib.h>
#include <sys/stat.h>
#include "../Platform.h"

namespace lake {

std::string OS::getHomeDir()
{
    if (homeDirectory.length() == 0)
        homeDirectory = getenv("HOME");

    return homeDirectory;
}

std::string OS::getDataDir()
{
    if (dataDirectory.length() == 0)
    {
        dataDirectory = getHomeDir();
        dataDirectory.append(1, getPathSeparator());

        // TODO: replace with appname
        dataDirectory.append(".lakeapp");

        // Create directory if missing
        mkdir(dataDirectory.c_str(), 0644 /*owner read/write, rest readonly*/);
    }

    return dataDirectory;
}

char OS::getPathSeparator()
{
    return '/';
}

}//ns
