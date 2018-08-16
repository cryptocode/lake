#include <windows.h>
#include <Shlobj.h>
#include "../Platform.h"

namespace lake {

std::string OS::getHomeDir()
{
    LPWSTR wszPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &wszPath);
    if (SUCCEEDED(hr))
    {
        return std::string((const char*)wszPath);
    }

    return "";
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
        CreateDirectory(dataDirectory.c_str(), nullptr);
    }

    return dataDirectory;
}

char OS::getPathSeparator()
{
    return '/';
}

}//ns
