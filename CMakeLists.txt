CMAKE_MINIMUM_REQUIRED(VERSION 3.4)
PROJECT(lake)

if (MSVC)
	SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000 /SAFESEH:NO")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++14 -DU_STATIC_IMPLEMENTATION ")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -std=c++14 -DU_STATIC_IMPLEMENTATION -O0 -D_DEBUG=3 -Wall")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG=4")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -stdlib=libc++ -std=c++14 -DU_STATIC_IMPLEMENTATION -DVM_DEBUG")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -stdlib=libc++ -std=c++14 -D_DEBUG=3 -DU_STATIC_IMPLEMENTATION -DVM_DEBUG")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -D_DEBUG=4 -DU_STATIC_IMPLEMENTATION -DVM_DEBUG")
endif ()

# -- BOOST
SET(Boost_USE_STATIC_LIBS ON)
SET(BOOST_ROOT "${CMAKE_SOURCE_DIR}/../libs/boost")

if (MSVC)
	SET(BOOST_LIBRARYDIR "${CMAKE_SOURCE_DIR}/../libs/boost/stage/lib")
endif()

if (MSVC)
    SET(MPIR_PATH ${CMAKE_SOURCE_DIR}/../libs/mpir/lib/x64/Release/mpir.lib)
else()
    SET(MPIR_PATH ${CMAKE_SOURCE_DIR}/../libs/mpir/.libs/libgmp.a)
endif()

# -- BOOST HEADERS
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/../libs/boost" ${CMAKE_SOURCE_DIR})

# -- GMP/MPIR HEADERS
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/../libs/mpir" ${CMAKE_SOURCE_DIR})

# -- EXTERNAL LIB HEADERS (lz4, etc)
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/ext" ${CMAKE_SOURCE_DIR})

#-- NUGET in Visual Studio
if (MSVC)
    INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/bin/packages/libffi.3.2.1.1/build/native/include")
	SET(LIBFFI_LIB_PATH "${CMAKE_SOURCE_DIR}/bin/packages/libffi.3.2.1.1/build/native/lib/x64/v140/static/Debug/libffi.lib")
else()
    INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/../libs/libffi/lib/libffi-3.2.1/include")
    SET(LIBFFI_LIB_PATH "${CMAKE_SOURCE_DIR}/../libs/libffi/lib/libffi.a")
endif()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out")

FILE(GLOB BUNDLED_INTERPRETER_SOURCE "src/vmbundle/*.cpp" "src/vmbundle/*.h")
FILE(GLOB DRIVER_SOURCE "src/vmcompile/*.cpp" "src/vmcompile/*.h")
FILE(GLOB LIB_SOURCE "src/vmlib/*.cpp" "src/vmlib/*.h")
FILE(GLOB RTLIB_SOURCE "src/rtlib/*.cpp" "src/rtlib/*.h")
FILE(GLOB VMFFI_SOURCE "src/vmffi/*.cpp" "src/vmffi/*.h")
FILE(GLOB LZ4_SRC "src/ext/lz4/*.h" "src/ext/lz4/*.c")
FILE(GLOB TESTS_BASIC_SOURCE "src/tests/BasicTests.cpp")

# Platform specific files
FILE(GLOB LIB_PLATFORM_SPECIFIC_SOURCE "src/vmplatform/*.h")

if (MSVC)
    FILE(GLOB LIB_PLATFORM_SPECIFIC_SOURCE ${LIB_PLATFORM_SPECIFIC_SOURCE} "src/vmplatform/windows/*.cpp" "src/vmplatform/windows/*.h")
else()
    FILE(GLOB LIB_PLATFORM_SPECIFIC_SOURCE ${LIB_PLATFORM_SPECIFIC_SOURCE} "src/vmplatform/unix/*.cpp" "src/vmplatform/unix/*.h")
endif()

ADD_LIBRARY(vmlib STATIC ${LIB_SOURCE} ${LZ4_SRC})
ADD_LIBRARY(vmffi STATIC ${VMFFI_SOURCE})
ADD_LIBRARY(vmplatform STATIC ${LIB_PLATFORM_SPECIFIC_SOURCE})

# We make rtlib an OBJECT library, otherwise the linker will strip the unreferenced symbols (which we dynload)
ADD_LIBRARY(rtlib OBJECT ${RTLIB_SOURCE})

# The compiler/interpreter/builder
ADD_EXECUTABLE(lake $<TARGET_OBJECTS:rtlib> ${DRIVER_SOURCE})

# The bundle interpreter
ADD_EXECUTABLE(lakei $<TARGET_OBJECTS:rtlib> ${BUNDLED_INTERPRETER_SOURCE})

# The api test executable
ADD_EXECUTABLE(tests-basic $<TARGET_OBJECTS:rtlib> ${TESTS_BASIC_SOURCE})

TARGET_LINK_LIBRARIES(lake vmlib vmplatform vmffi ${MPIR_PATH} ${LIBFFI_LIB_PATH})
TARGET_LINK_LIBRARIES(lakei vmlib vmplatform vmffi ${MPIR_PATH} ${LIBFFI_LIB_PATH})
TARGET_LINK_LIBRARIES(tests-basic vmlib vmplatform vmffi ${MPIR_PATH} ${LIBFFI_LIB_PATH})

# Some recent Linux distros require us to link with -ldl as well (for dlopen, etc)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	TARGET_LINK_LIBRARIES(lakei dl)
	TARGET_LINK_LIBRARIES(lake dl)
endif()
