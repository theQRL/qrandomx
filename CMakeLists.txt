# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.10)
project(qrandomx)

set(CMAKE_CXX_STANDARD 14)

if (NOT WIN32)
    # ISA baseline.
    #
    # Building with -march=native produces binaries that only run on CPUs at
    # least as capable as the build machine. That is fine for a local build but
    # wrong for anything distributed (PyPI wheels, container images): a CI
    # runner with, say, AVX-512 VBMI yields code that dies with SIGILL on an
    # older AVX-512 part such as Skylake-SP/W. Default to a portable baseline
    # and let a machine-local build opt in with -DQRX_ARCH_X86=native.
    if (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-march=x86-64-v2" QRX_HAS_X86_64_V2)
        if (QRX_HAS_X86_64_V2)
            set(_QRX_ARCH_X86_DEFAULT "x86-64-v2")
        else()
            # -march=x86-64-v2 needs GCC >= 11 / Clang >= 12; nehalem is the
            # equivalent baseline understood by older compilers.
            set(_QRX_ARCH_X86_DEFAULT "nehalem")
        endif()
        set(QRX_ARCH_X86 "${_QRX_ARCH_X86_DEFAULT}" CACHE STRING
            "x86-64 ISA baseline; use 'native' for a machine-local build")
        set(_BASE_CXX_FLAGS "-march=${QRX_ARCH_X86} -mtune=generic -msse2 -maes")
        set(_BASE_C_FLAGS   "-march=${QRX_ARCH_X86} -mtune=generic -msse2 -maes -fPIC")
    else()
        set(QRX_ARCH_ARM "armv8-a+crypto+fp+simd" CACHE STRING
            "AArch64 ISA baseline; use 'native' for a machine-local build")
        set(_BASE_CXX_FLAGS "-march=${QRX_ARCH_ARM} -mtune=generic")
        set(_BASE_C_FLAGS   "-march=${QRX_ARCH_ARM} -mtune=generic -fPIC")
    endif()

    # Caller-supplied flags go last so CXXFLAGS/CFLAGS can still override.
    set(CMAKE_CXX_FLAGS "${_BASE_CXX_FLAGS} -pthread ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS   "${_BASE_C_FLAGS} ${CMAKE_C_FLAGS}")
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
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/virtual_memory.c
        # Explicitly exclude the old virtual_memory.cpp to prevent build errors
        # ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/virtual_memory.cpp
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

# The vectorised Argon2 implementations are chosen at runtime -- see
# deps/RandomX/src/randomx.cpp, which only uses randomx_argon2_impl_avx2() when
# cpu.hasAvx2() is also true -- so they carry their own ISA flags instead of
# relying on the global baseline. Mirrors deps/RandomX/CMakeLists.txt; without
# this, both files compile to a NULL impl on a portable baseline and the Argon2
# dataset initialisation silently falls back to the reference code.
if (NOT WIN32 AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    include(CheckCCompilerFlag)
    check_c_compiler_flag(-mssse3 QRX_HAS_MSSSE3)
    if (QRX_HAS_MSSSE3)
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_ssse3.c
            PROPERTIES COMPILE_FLAGS -mssse3)
    endif()
    check_c_compiler_flag(-mavx2 QRX_HAS_MAVX2)
    if (QRX_HAS_MAVX2)
        set_source_files_properties(
            ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/argon2_avx2.c
            PROPERTIES COMPILE_FLAGS -mavx2)
    endif()
endif()

if (CMAKE_C_COMPILER_ID MATCHES MSVC)
        enable_language(ASM_MASM)
        list(APPEND REF_RANDOMX_SRC
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.asm
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86.cpp
                )
elseif (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND REF_RANDOMX_SRC
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.S
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86.cpp
                )
        # cheat because cmake and ccache hate each other
        set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_x86_static.S PROPERTY LANGUAGE C)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        enable_language(ASM)
        list(APPEND REF_RANDOMX_SRC
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_a64_static.S
                ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_a64.cpp
                )
        # Set ASM language for ARM64
        set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_a64_static.S PROPERTY LANGUAGE ASM)
        # Set assembler flags for ARM64
        set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -march=armv8-a+crypto")
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/deps/RandomX/src/jit_compiler_a64_static.S 
            PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -march=armv8-a+crypto")
        # Define ARM64 macro
        add_definitions(-DRANDOMX_AARCH64=1)
        # Ensure proper compiler flags for ARM64 JIT
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")
        endif()
endif()

## SWIG + API - Python related stuff
if (BUILD_PYTHON)
        message(STATUS "Python wrapper enabled")

        if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
                set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        endif ()

        set(language python)

        if(${CMAKE_VERSION} VERSION_GREATER "3.12")
          set(Python_ADDITIONAL_VERSIONS 3 3.7 3.8 3.9 3.6 3.5)
          find_package(Python COMPONENTS Interpreter Development REQUIRED)
          message("Python_FOUND:${Python_FOUND}")
          message("Python_VERSION:${Python_VERSION}")
          message("Python_Development_FOUND:${Python_Development_FOUND}")
          message("Python_LIBRARIES:${Python_LIBRARIES}")
        else ()
          set(Python_ADDITIONAL_VERSIONS 3 3.6 3.5)
          find_package(PythonLibs REQUIRED)
          find_package(PythonInterp REQUIRED)
          set(Python_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")
          set(Python_LIBRARIES "${PYTHON_LIBRARIES}")
        endif ()

        include_directories(
                ${Python_INCLUDE_DIRS}
        )
        set(CMAKE_SWIG_OUTDIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pyqrandomx)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pyqrandomx)

        set(SWIG_INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/api/qrandomx.i)
        set(SWIG_LANG_LIBRARIES ${Python_LIBRARIES})
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
                ${Python_INCLUDE_PATH}
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
        # Based on instructions in https://github.com/google/googletest/tree/main/googletest#incorporating-into-an-existing-cmake-project
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
