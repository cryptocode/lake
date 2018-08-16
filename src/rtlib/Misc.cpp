#include <time.h>
#include "../vmlib/Object.h"

extern "C" const char* lake_version()
{
    return "Lake version 1.0";
}

extern "C" int64_t lake_version_int()
{
    return 1;
}

/* Test functions */

extern "C" void lake_print_ints(int32_t normal, int native, uint64_t large, int16_t ashort)
{
    std::cout << "Outputting some values" << std::endl;
    std::cout << "Received ints: 32-bit: " << normal << ", native: " << native << ", 64-bit:" << large << ", short: "  << ashort << std::endl;
}

extern "C" void lake_print_str(const char* str)
{
    std::cout << "Received str: " << str << std::endl;
}

extern "C" tm* lake_get_tm()
{
    time_t rawtime;
    time( &rawtime );

    struct tm *info;

    info = localtime( &rawtime );

    printf("Return tm @ %p\n", info);
    return info;
}

extern "C" void lake_print_tm(tm* info)
{
    printf("Current local time and date: %s '%s'\n", asctime(info), info->tm_zone == nullptr ? "<null>" : info->tm_zone);
}

