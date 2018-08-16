#include <fstream>
#include "boost/endian/conversion.hpp"
#include <sstream>
#include <iostream>
#include "Bundles.h"
#include "Process.h"
#include "lz4/lz4.h"
#include "ExecLocation.h"

namespace lake {

void BundleWriter::execute()
{
    std::ofstream out(output, std::ios::binary | std::ios::trunc);

    // Copy executable
    std::string exe = interpreterPath;

    // Use the compiler binary as the interpreter (useful for testing and debugging) ?
    if (interpreterPath.length() == 0 || interpreterPath == ".")
        exe = BundleUtil::instance().getExecutablePath();

    if (exe.size() == 0)
        throw std::runtime_error("Could not find executable path");

    std::ifstream in(exe, std::ios::binary);
    out << in.rdbuf();

    // The total size in bytes of the bundle (that is, the size of everything
    // added to the interpreter executable, excluding the length- and magic markers)
    uint64_t bundleSize = 0;

    // Write resource count
    uint64_t resourceCount = input.size();
    resourceCount = boost::endian::native_to_big(resourceCount);
    out.write(reinterpret_cast<const char *>(&resourceCount), sizeof(resourceCount));

    bundleSize+=sizeof(resourceCount);

    // Write each resource
    for (const auto& path : input)
    {
        trace_infof("Packaging %s", path.c_str());

        /// Read entire file
        std::ifstream t(path, std::ios::binary);
        std::stringstream buffer;
        buffer << t.rdbuf();
        auto str = buffer.str();
        uint64_t len = str.size();
        t.close();

        // Compress it
        char* pchCompressed = new char[len];
        int64_t nCompressedSize = LZ4_compress_fast(str.c_str(), pchCompressed, len, len,9);
        if (nCompressedSize < 0)
            throw std::runtime_error("Invalid compressed buffer length");

        // Write the pathname length in big endian, followed by the path
        uint64_t pathLen = boost::endian::native_to_big((uint64_t)path.size());
        out.write(reinterpret_cast<const char *>(&pathLen),sizeof(pathLen));
        out.write(path.c_str(), path.size());

        // Write length of compressed buffer in big endian, original length in big endian, followed by the buffer
        uint64_t compressedSize = boost::endian::native_to_big(uint64_t(nCompressedSize));
        uint64_t originalSize = boost::endian::native_to_big(len);
        out.write(reinterpret_cast<const char *>(&compressedSize),sizeof(compressedSize));
        out.write(reinterpret_cast<const char *>(&originalSize),sizeof(originalSize));
        out.write(pchCompressed, nCompressedSize);

        bundleSize += sizeof(pathLen) + path.size() + sizeof(compressedSize) + sizeof(originalSize) + nCompressedSize;

        delete [] pchCompressed;
    }

    // Write bundle size
    bundleSize = boost::endian::native_to_big(bundleSize);
    out.write(reinterpret_cast<const char*>(&bundleSize), sizeof(bundleSize));

    // Write bundle marker as big endian (big endian is simply less
    // confusing to debug, so let's spend a cycle on it)
    auto marker = boost::endian::native_to_big(bundleMarker);
    out.write(reinterpret_cast<const char*>(&marker),sizeof(marker));
    out.close();
}

bool BundleReader::read()
{
    std::string exe = BundleUtil::instance().getExecutablePath();
    if (exe.size() == 0)
        throw std::runtime_error("Could not find executable path");

    std::ifstream in(exe, std::ios::binary);
    in.seekg (0, in.end);
    int length = in.tellg();

    // Magic+len minimum
    uint64_t bundleSize;
    uint64_t marker;

    if (length >= sizeof(marker)+sizeof(bundleSize))
    {
        in.seekg(-(sizeof(marker)+sizeof(bundleSize)), std::ios::end);

        in.read(reinterpret_cast<char*>(&bundleSize), sizeof(bundleSize));
        bundleSize = boost::endian::big_to_native(bundleSize);

        in.read(reinterpret_cast<char*>(&marker), sizeof(marker));
        marker = boost::endian::big_to_native(marker);

        if (marker == bundleMarker)
        {
            std::cout << "bundle size: " << bundleSize << ", marker: " << std::hex << bundleMarker << std::dec << std::endl;

            in.seekg(-((int64_t)bundleSize+sizeof(marker)+sizeof(bundleSize)), std::ios::end);

            uint64_t resourceCount;
            in.read(reinterpret_cast<char*>(&resourceCount), sizeof(resourceCount));
            resourceCount = boost::endian::big_to_native(resourceCount);

            // Sanity check
            if (resourceCount > 1024*1024)
                throw std::runtime_error("Invalid resource count");

            std::cout << "Reading " << resourceCount << " resources" << std::endl;

            for (uint64_t i=0; i < resourceCount; i++)
            {
                // Path length
                uint64_t pathLen;
                in.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
                pathLen = boost::endian::big_to_native(pathLen);

                // Path
                char* tmp = new char[pathLen+1];
                in.read(tmp, pathLen);
                tmp[pathLen] = 0;
                std::string path(tmp);

                // Compressed length
                uint64_t compressedLen;
                in.read(reinterpret_cast<char*>(&compressedLen), sizeof(compressedLen));
                compressedLen = boost::endian::big_to_native(compressedLen);

                // Uncompressed length
                uint64_t originalSize;
                in.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
                originalSize = boost::endian::big_to_native(originalSize);

                // Decompress
                char* compressed = new char[compressedLen];
                in.read(compressed, compressedLen);

                char* dest = new char[originalSize];
                LZ4_decompress_fast(compressed, dest, originalSize);

                // Note that the decompressed 'dest' buffer is getting owned by Resource below,
                // and deleted in the d'tor
                delete [] compressed;

                resources[path] = std::make_unique<Resource>(path, originalSize, dest);
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

std::string BundleUtil::getExecutablePath()
{
    if (path.size() == 0)
    {
        int length, dirname_length;

        length = wai_getExecutablePath(nullptr, 0, &dirname_length);

        if (length > 0)
        {
            char *_path = new char[length + 1];

            wai_getExecutablePath(_path, length, &dirname_length);
            _path[length] = '\0';

            trace_infof("Executable path: %s\n", _path);

            // Copy
            path = _path;
            delete[] _path;
        }
    }

    return path;
}

}//ns
