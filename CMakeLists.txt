# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.10)
project(qrandomx)

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS " -pthread")

if(NOT WIN32)
        set(CMAKE_CXX_FLAGS "-march=native -mtune=native ${CMAKE_CXX_FLAGS} -msse2 -maes")
        set(CMAKE_C_FLAGS "-march=native -mtune=native -fPIC ${CMAKE_C_FLAGS} -msse2 -maes")
endif()

set(BUILD_TESTS ON CACHE BOOL "Enables tests")
set(BUILD_PYTHON ON CACHE BOOL "Enables python wrapper")
set(BUILD_GO OFF CACHE BOOL "Enables go wrapper")
set(BUILD_WEBASSEMBLY OFF CACHE BOOL "Enables emscripten build")

message(STATUS "BUILD_TESTS    " ${BUILD_TESTS})
message(STATUS "PYTHON WRAPPER " ${BUILD_PYTHON})
message(STATUS "GO WRAPPER     " ${BUILD_GO})
message(STATUS "WEBASSEMBLY    " ${BUILD_WEBASSEMBLY})

if (BUILD_PYTHON OR BUILD_GO)
        find_package(SWIG REQUIRED)
        INCLUDE(${SWIG_USE_FILE})
        unset(SWIG_LANG_TYPE)
endif ()

FIND_PACKAGE(Boost 1.58 REQUIRED)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/deps
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

file(GLOB LIB_QRANDOMX_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qrandomx
        )

file(GLOB_RECURSE LIB_QRANDOMX_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pow/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qrandomx/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qrandomx/*.c"
        )

file(GLOB_RECURSE TEST_QRANDOMX_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/*.cpp")

list(APPEND REF_RANDOMX_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/aes_hash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/allocator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_core.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_ref.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/blake2_generator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/blake2/blake2b.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/bytecode_machine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/dataset.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/instructions_portable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/randomx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/reciprocal.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/soft_aes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/superscalar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/virtual_machine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/virtual_memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/vm_compiled_light.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/vm_compiled.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/vm_interpreted_light.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/vm_interpreted.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/cpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_avx2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_core.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_ref.c
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_ssse3.c
        )

if (CMAKE_C_COMPILER_ID MATCHES MSVC)
        enable_language(ASM_MASM)
        list(APPEND REF_RANDOMX_SRC
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.asm
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86.cpp
                )
elseif (NOT XMRIG_ARM AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND REF_RANDOMX_SRC
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.S
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86.cpp
                )
        # cheat because cmake and ccache hate each other
        set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.S PROPERTY LANGUAGE C)
endif()

## SWIG + API - Python related stuff
if (BUILD_PYTHON)
        message(STATUS "Python wrapper enabled")

        if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
                set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        endif ()

        set(language python)
        find_package(PythonLibs 3.4 REQUIRED)
        include_directories(
                ${PYTHON_INCLUDE_PATH}
        )
        set(CMAKE_SWIG_OUTDIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pyqrandomx)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pyqrandomx)

        set(SWIG_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/api/qrandomx.i)
        set(SWIG_LANG_LIBRARIES ${PYTHON_LIBRARIES})
        set_source_files_properties(${SWIG_INTERFACE} PROPERTIES CPLUSPLUS ON)
        set_property(SOURCE ${SWIG_INTERFACE} PROPERTY SWIG_FLAGS "-includeall" "-ignoremissing")
        message(STATUS "CMAKE_SWIG_OUTDIR: " ${CMAKE_SWIG_OUTDIR})
        message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

        # Enable threading in the wrapper (useful for GIL-safe callbacks)
        set( CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} "-threads" )

        # Intentionally use a deprecated version to provide support for the raspberry pi
        SWIG_ADD_MODULE(pyqrandomx
                ${language}
                ${SWIG_INTERFACE}
                ${LIB_QRANDOMX_SRC}
                ${REF_RANDOMX_SRC}
                )

        SWIG_LINK_LIBRARIES(pyqrandomx
                ${SWIG_LANG_LIBRARIES}
                ${REF_RANDOMX_LIBS})

        if(WIN32)
                swig_link_libraries(pyqrandomx wsock32 ws2_32)
        endif()

        include_directories(
                ${PYTHON_INCLUDE_PATH}
                ${LIB_QRANDOMX_INCLUDES}
                ${Boost_INCLUDE_DIRS}
        )

        set_target_properties(${SWIG_MODULE_pyqrandomx_REAL_NAME} PROPERTIES DEBUG_POSTFIX "_d")

        if(WIN32 AND HWLOC_ENABLE)
                add_custom_command(TARGET ${SWIG_MODULE_pyqrandomx_REAL_NAME}
                        POST_BUILD
                        COMMENT "Copying HWLOC DLL to package folder"
                        COMMAND ${CMAKE_COMMAND} -E
                        copy_if_different "${HWLOC_DLL}" "${CMAKE_CURRENT_SOURCE_DIR}/pyqrandomx/")
        endif()

        add_custom_command(TARGET ${SWIG_MODULE_pyqrandomx_REAL_NAME}
                POST_BUILD
                COMMENT "Moving SWIG files to output dir"
                COMMAND ${CMAKE_COMMAND} -E
                copy_if_different $<TARGET_FILE:${SWIG_MODULE_pyqrandomx_REAL_NAME}>
                ${CMAKE_CURRENT_SOURCE_DIR}/pyqrandomx/$<TARGET_LINKER_FILE_NAME:${SWIG_MODULE_pyqrandomx_REAL_NAME}>
                )

endif ()


set(BUILD_TESTS ON CACHE BOOL "Enables tests")
message(STATUS "BUILD_TESTS    " ${BUILD_TESTS})

if (BUILD_TESTS)
        message(STATUS "GTests enabled")

        ##############################
        # Google Test
        # Based on instructions in https://github.com/google/googletest/tree/master/googletest#incorporating-into-an-existing-cmake-project
        # Download and unpack googletest at configure time
        configure_file(CMakeLists.txt.gtest.in googletest-download/CMakeLists.txt)
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
        if (result)
                message(FATAL_ERROR "CMake step for googletest failed: ${result}")
        endif ()
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
        if (result)
                message(FATAL_ERROR "Build step for googletest failed: ${result}")
        endif ()

        # Prevent overriding the parent project's compiler/linker settings on Windows
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        # Add googletest directly to our build. This defines
        # the gtest and gtest_main targets.
        add_subdirectory(
                ${CMAKE_BINARY_DIR}/googletest-src
                ${CMAKE_BINARY_DIR}/googletest-build
        )

        # The gtest/gtest_main targets carry header search path
        # dependencies automatically when using CMake 2.8.11 or
        # later. Otherwise we have to add them here ourselves.
        if (CMAKE_VERSION VERSION_LESS 2.8.11)
                include_directories("${gtest_SOURCE_DIR}/include")
        endif ()

        ###########################
        include(CTest)

        add_executable(qrandomx_test
                ${TEST_QRANDOMX_SRC}
                ${LIB_QRANDOMX_SRC}
                ${REF_RANDOMX_SRC}
                src/qrandomx/rx-slow-hash.h src/qrandomx/rx-slow-hash.c src/qrandomx/c_threads.h)

        target_include_directories( qrandomx_test PRIVATE
                ${LIB_RANDOMX_INCLUDES} ${Boost_INCLUDE_DIRS})

        target_link_libraries(qrandomx_test
                gtest_main
                )

        add_test(gtest ${PROJECT_BINARY_DIR}/qrandomx_test)

endif ()
