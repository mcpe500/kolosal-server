# ==============================================================================
# KOLOSAL SERVER CMAKE CONFIGURATION
# ==============================================================================

cmake_minimum_required(VERSION 3.14)

# ==============================================================================
# CMAKE POLICIES
# ==============================================================================

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# ==============================================================================
# PROJECT DECLARATION
# ==============================================================================

project(KolosalServer VERSION 1.0.0 LANGUAGES C CXX)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/overrides")

# ==============================================================================
# BUILD CONFIGURATION
# ==============================================================================

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add DEBUG definition for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

# UCM Configuration
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
ucm_set_runtime(STATIC)

# Output Directories
if(CMAKE_CONFIGURATION_TYPES)
    foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${_cfg} _CFG_UP)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG_UP} "${CMAKE_BINARY_DIR}/${_cfg}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_CFG_UP} "${CMAKE_BINARY_DIR}/${_cfg}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_CFG_UP} "${CMAKE_BINARY_DIR}/${_cfg}")
    endforeach()
elseif(CMAKE_BUILD_TYPE)
    set(_single_dir "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
    file(MAKE_DIRECTORY "${_single_dir}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_single_dir}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_single_dir}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${_single_dir}")
endif()

# VCPKG Configuration (Windows)
if(WIN32)
    set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "Disable vcpkg applocal dependency copying" FORCE)
    set(VCPKG_PWSH_PATH "powershell.exe" CACHE STRING "Path to PowerShell for vcpkg applocal step" FORCE)
endif()

# ==============================================================================
# BUILD OPTIONS
# ==============================================================================

option(USE_CUDA   "Compile with CUDA support" OFF)
option(USE_VULKAN "Compile with VULKAN support" OFF)

set(_KOLOSAL_DEFAULT_USE_METAL OFF)
if(APPLE)
    set(_KOLOSAL_DEFAULT_USE_METAL ON)
endif()
option(USE_METAL  "Compile with Metal support (macOS only)" ${_KOLOSAL_DEFAULT_USE_METAL})
unset(_KOLOSAL_DEFAULT_USE_METAL)

option(USE_MPI    "Compile with MPI support" OFF)
option(DEBUG      "Compile with debugging information" OFF)
option(USE_PODOFO "Compile with PoDoFo PDF support" ON)
option(USE_FAISS  "Compile with FAISS support" ON)
option(ENABLE_NATIVE_OPTIMIZATION "Enable native CPU optimization" OFF)
option(INSTALL_HEADERS "Install header files for development" OFF)

# ==============================================================================
# PLATFORM-SPECIFIC COMPILE DEFINITIONS
# ==============================================================================

if(MINGW)
    add_compile_definitions(_WIN32_WINNT=0x602)
endif()

if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(UNIX AND NOT APPLE)
    add_compile_definitions(
        _GNU_SOURCE 
        __LINUX__ 
        _FILE_OFFSET_BITS=64
        _LARGEFILE_SOURCE
    )
elseif(APPLE)
    add_compile_definitions(
        _DARWIN_C_SOURCE 
        __APPLE__
        _FILE_OFFSET_BITS=64
        _LARGEFILE_SOURCE
    )
endif()

# ==============================================================================
# INCLUDE DIRECTORIES
# ==============================================================================

# Core Include Directories
set(KOLOSAL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/inference/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/external/yaml-cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/maddy/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/zlib
    ${CMAKE_CURRENT_SOURCE_DIR}/external/zlib/contrib/minizip
    ${CMAKE_CURRENT_SOURCE_DIR}/external/curl/include
)

# Optional Include Directories
if(USE_PODOFO)
    list(APPEND KOLOSAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/podofo/src)
endif()

if(USE_FAISS)
    list(APPEND KOLOSAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external/faiss)
endif()

# Llama.cpp Include Directories
set(LLAMA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/inference/external/llama.cpp")
if(NOT EXISTS "${LLAMA_PATH}/include" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/include")
    set(LLAMA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp")
endif()

if(EXISTS "${LLAMA_PATH}/include")
    list(APPEND KOLOSAL_INCLUDE_DIRS
        ${LLAMA_PATH}/include
        ${LLAMA_PATH}/common
        ${LLAMA_PATH}/ggml/include
    )
endif()

# Apply all include directories
include_directories(${KOLOSAL_INCLUDE_DIRS})

# ==============================================================================
# SOURCE FILES
# ==============================================================================

# Core Server Sources
set(KOLOSAL_CORE_SOURCES
    src/server.cpp
    src/server_api.cpp    
    src/server_config.cpp
    src/logger.cpp
    src/download_utils.cpp
    src/download_manager.cpp
    src/faiss_client.cpp
    src/qdrant_client.cpp
    src/node_manager.cpp
    src/inference_loader.cpp
    src/inference_interface_impl.cpp
    src/gpu_detection.cpp
)

# Route Sources
set(KOLOSAL_ROUTE_SOURCES
    # LLM Routes
    src/routes/llm/oai_completions_route.cpp
    src/routes/llm/completion_route.cpp
    # API Routes
    src/routes/models_route.cpp
    src/routes/engines_route.cpp
    src/routes/health_status_route.cpp    
    src/routes/config/auth_config_route.cpp    
    src/routes/server_logs_route.cpp    
    src/routes/downloads_route.cpp
    # Retrieval Routes
    src/routes/retrieval/embedding_route.cpp
    src/routes/retrieval/internet_search_route.cpp
    src/routes/retrieval/parse_document_route.cpp
    src/routes/retrieval/documents_route.cpp
    src/routes/retrieval/chunking_route.cpp
)

# Retrieval Sources
set(KOLOSAL_RETRIEVAL_SOURCES
    src/retrieval/add_document_types.cpp
    src/retrieval/remove_document_types.cpp
    src/retrieval/document_list_types.cpp
    src/retrieval/retrieve_types.cpp
    src/retrieval/document_service.cpp
    src/retrieval/parse_docx.cpp
    src/retrieval/parse_html.cpp
    src/retrieval/chunking_types.cpp
)

# Model Sources
set(KOLOSAL_MODEL_SOURCES
    src/models/embedding_request_model.cpp
    src/models/embedding_response_model.cpp
    src/models/chunking_request_model.cpp
    src/models/chunking_response_model.cpp
)

# Authentication Sources
set(KOLOSAL_AUTH_SOURCES
    src/auth/rate_limiter.cpp
    src/auth/cors_handler.cpp
    src/auth/auth_middleware.cpp
)

# Combine all sources
set(KOLOSAL_SOURCES
    ${KOLOSAL_CORE_SOURCES}
    ${KOLOSAL_ROUTE_SOURCES}
    ${KOLOSAL_RETRIEVAL_SOURCES}
    ${KOLOSAL_MODEL_SOURCES}
    ${KOLOSAL_AUTH_SOURCES}
)

# Optional PDF parsing source
if(USE_PODOFO)
    list(APPEND KOLOSAL_SOURCES src/retrieval/parse_pdf.cpp)
else()
    list(APPEND KOLOSAL_SOURCES src/retrieval/parse_pdf_stub.cpp)
endif()

# ==============================================================================
# TARGET DEFINITIONS
# ==============================================================================

# Shared Library Target
add_library(kolosal_server SHARED ${KOLOSAL_SOURCES})
target_compile_definitions(kolosal_server PRIVATE 
    KOLOSAL_SERVER_BUILD 
    INFERENCE_EXPORTS
)

# Apple-specific library properties
if(APPLE)
    set_target_properties(kolosal_server PROPERTIES
        MACOSX_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
    )
endif()

# Executable Target
add_executable(kolosal_server_exe src/main.cpp)
target_link_libraries(kolosal_server_exe PRIVATE kolosal_server)
set_target_properties(kolosal_server_exe PROPERTIES OUTPUT_NAME "kolosal-server")

# ==============================================================================
# INFERENCE ENGINE CONFIGURATION
# ==============================================================================

# Determine inference target based on build options
if(USE_CUDA)
    set(INFERENCE_TARGET "llama-cuda")
elseif(USE_VULKAN)
    set(INFERENCE_TARGET "llama-vulkan")
elseif(USE_METAL)
    set(INFERENCE_TARGET "llama-metal")
else()
    set(INFERENCE_TARGET "llama-cpu")
endif()

message(STATUS "Building inference engine: ${INFERENCE_TARGET}")
add_subdirectory(inference)

# ==============================================================================
# EXTERNAL DEPENDENCIES
# ==============================================================================

# YAML-CPP Configuration and Build
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/yaml-cpp)

# CURL Dependency
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    set(CURL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/curl")
    if(EXISTS "${CURL_PATH}/include/curl/curl.h")
        find_library(CURL_LIBRARY 
            NAMES curl libcurl
            PATHS "${CURL_PATH}/lib"
            PATH_SUFFIXES Release Debug
            NO_DEFAULT_PATH
        )
    endif()
endif()

# ZLIB Dependency
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_MINIZIP OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/zlib EXCLUDE_FROM_ALL)
    set(ZLIB_FOUND TRUE)
    set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib;${CMAKE_CURRENT_BINARY_DIR}/external/zlib")
    set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIR}")
    if(TARGET zlibstatic)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
        set(ZLIB_LIBRARIES ZLIB::ZLIB)
        set(ZLIB_LIBRARY zlibstatic)  # Set the raw target name that find_package expects
    endif()
endif()

# this line causes error:
# CMake Error at CMakeLists.txt:330 (add_subdirectory):
#   The binary directory
# add_subdirectory(inference)

find_package(CURL QUIET)
if(NOT CURL_FOUND)
    set(CURL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/curl")
    if(EXISTS "${CURL_PATH}/include/curl/curl.h")
        set(CURL_INCLUDE_DIR "${CURL_PATH}/include")
        find_library(CURL_LIBRARY 
            NAMES curl libcurl
            PATHS "${CURL_PATH}/lib"
            PATH_SUFFIXES Release Debug
            NO_DEFAULT_PATH
        )
        if(CURL_LIBRARY)
            set(CURL_FOUND TRUE)
            message(STATUS "Found curl library: ${CURL_LIBRARY}")
        endif()
    endif()
endif()

# MiniZip Static Library
if(NOT TARGET minizip_static)
    set(MINIZIP_SOURCES
        external/zlib/contrib/minizip/ioapi.c
        external/zlib/contrib/minizip/mztools.c
        external/zlib/contrib/minizip/unzip.c
        external/zlib/contrib/minizip/zip.c
    )
    
    if(WIN32)
        list(APPEND MINIZIP_SOURCES external/zlib/contrib/minizip/iowin32.c)
    endif()
    
    add_library(minizip_static STATIC ${MINIZIP_SOURCES})
    target_include_directories(minizip_static PUBLIC 
        external/zlib/contrib/minizip 
        external/zlib
    )
    
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(minizip_static PUBLIC ZLIB::ZLIB)
    endif()
    
    set_target_properties(minizip_static PROPERTIES
        OUTPUT_NAME minizip
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# PugiXML Library
if(NOT TARGET pugixml)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/CMakeLists.txt")
        set(PUGIXML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory(external/pugixml EXCLUDE_FROM_ALL)
    else()
        add_library(pugixml STATIC external/pugixml/src/pugixml.cpp)
        target_include_directories(pugixml PUBLIC external/pugixml/src)
        set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
endif()

# PoDoFo PDF Library (Optional)
if(USE_PODOFO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/podofo/CMakeLists.txt")
    # Find PoDoFo dependencies
    find_package(Freetype QUIET)
    find_package(JPEG QUIET)
    find_package(PNG QUIET)
    find_package(TIFF QUIET)
    find_package(LibXml2 QUIET)
    find_package(OpenSSL QUIET)

    # Configure PoDoFo build
    set(PODOFO_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(PODOFO_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(PODOFO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PODOFO_BUILD_UNSUPPORTED_TOOLS OFF CACHE BOOL "" FORCE)
    set(PODOFO_BUILD_LIB_ONLY ON CACHE BOOL "" FORCE)
    
    # Build PoDoFo
    set(CMAKE_SKIP_INSTALL_RULES TRUE)
    add_subdirectory(external/podofo EXCLUDE_FROM_ALL)
    set(CMAKE_SKIP_INSTALL_RULES FALSE)
    
    if(TARGET podofo_static)
        set_target_properties(podofo_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    target_include_directories(kolosal_server PRIVATE 
        "${CMAKE_CURRENT_BINARY_DIR}/external/podofo/src/podofo/podofo/auxiliary"
    )
endif()

# FAISS Vector Search Library (Optional)
if(USE_FAISS)
    find_package(faiss QUIET)
    
    if(NOT faiss_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/faiss/CMakeLists.txt")
        # OpenMP Dependency for FAISS
        find_package(OpenMP QUIET)
        if(NOT OpenMP_FOUND)
            if(APPLE)
                # Try to find libomp on macOS
                find_path(OpenMP_INCLUDE_DIR
                    NAMES omp.h
                    PATHS /opt/homebrew/opt/libomp/include /usr/local/opt/libomp/include
                    NO_DEFAULT_PATH
                )
                find_library(OpenMP_libomp_LIBRARY
                    NAMES omp libomp
                    PATHS /opt/homebrew/opt/libomp/lib /usr/local/opt/libomp/lib
                    NO_DEFAULT_PATH
                )
                
                if(OpenMP_libomp_LIBRARY AND OpenMP_INCLUDE_DIR)
                    # Configure OpenMP for macOS
                    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${OpenMP_INCLUDE_DIR}" CACHE STRING "" FORCE)
                    set(OpenMP_C_LIB_NAMES "omp" CACHE STRING "" FORCE)
                    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${OpenMP_INCLUDE_DIR}" CACHE STRING "" FORCE)
                    set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "" FORCE)
                    set(OpenMP_omp_LIBRARY ${OpenMP_libomp_LIBRARY} CACHE FILEPATH "" FORCE)
                    set(OpenMP_C_LIBRARIES ${OpenMP_libomp_LIBRARY} CACHE STRING "" FORCE)
                    set(OpenMP_CXX_LIBRARIES ${OpenMP_libomp_LIBRARY} CACHE STRING "" FORCE)
                    set(OpenMP_FOUND TRUE CACHE BOOL "" FORCE)

                    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${OpenMP_INCLUDE_DIR}")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${OpenMP_INCLUDE_DIR}")
                    message(STATUS "Found libomp: ${OpenMP_libomp_LIBRARY}")
                    message(STATUS "Found omp.h: ${OpenMP_INCLUDE_DIR}")
                else()
                    message(FATAL_ERROR 
                        "OpenMP (libomp) is required for FAISS but not found.\n"
                        "Please install it with: brew install libomp\n"
                        "Or disable FAISS with: -DUSE_FAISS=OFF"
                    )
                endif()
            else()
                message(FATAL_ERROR 
                    "OpenMP is required for FAISS but not found.\n"
                    "On Ubuntu/Debian: apt install libomp-dev\n"
                    "On RHEL/CentOS: yum install libomp-devel\n"
                    "Or disable FAISS with: -DUSE_FAISS=OFF"
                )
            endif()
        endif()

        # BLAS Dependency for FAISS
        find_package(BLAS QUIET)
        if(NOT BLAS_FOUND AND APPLE)
            find_library(ACCELERATE_FRAMEWORK Accelerate)
            if(ACCELERATE_FRAMEWORK)
                set(BLAS_LIBRARIES ${ACCELERATE_FRAMEWORK})
                set(BLAS_FOUND TRUE)
                message(STATUS "Using Accelerate framework for BLAS")
            endif()
        endif()
        
        if(NOT BLAS_FOUND)
            message(FATAL_ERROR 
                "BLAS is required for FAISS. "
                "On macOS: comes with Accelerate. "
                "On Linux: sudo apt-get install libblas-dev liblapack-dev"
            )
        endif()

        # Configure FAISS build
        set(FAISS_ENABLE_GPU OFF CACHE BOOL "" FORCE)
        set(FAISS_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(FAISS_OPT_LEVEL generic CACHE STRING "Use generic optimization" FORCE)

        add_subdirectory(external/faiss EXCLUDE_FROM_ALL)
    endif()
endif()

# ==============================================================================
# TARGET CONFIGURATION AND LINKING
# ==============================================================================

# Compile Definitions
target_compile_definitions(kolosal_server PUBLIC
    $<$<BOOL:${USE_CUDA}>:USE_CUDA>
    $<$<BOOL:${USE_VULKAN}>:USE_VULKAN>
    $<$<BOOL:${USE_MPI}>:USE_MPI>
    $<$<BOOL:${DEBUG}>:DEBUG>
    $<$<BOOL:${USE_PODOFO}>:USE_PODOFO>
    $<$<BOOL:${USE_FAISS}>:USE_FAISS>
)

# Core Library Dependencies
set(KOLOSAL_LINK_LIBRARIES 
    yaml-cpp 
    ${INFERENCE_TARGET} 
    minizip_static 
    pugixml
)

# Optional Library Dependencies
if(USE_PODOFO AND TARGET podofo_static)
    list(APPEND KOLOSAL_LINK_LIBRARIES podofo_static)
    target_compile_definitions(kolosal_server PRIVATE PODOFO_STATIC)
    
    if(TARGET OpenSSL::SSL)
        list(APPEND KOLOSAL_LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

if(USE_FAISS AND TARGET faiss)
    list(APPEND KOLOSAL_LINK_LIBRARIES faiss)
endif()

if(CURL_FOUND OR CURL_LIBRARY)
    list(APPEND KOLOSAL_LINK_LIBRARIES ${CURL_LIBRARY})
    message(STATUS "Linking curl library: ${CURL_LIBRARY}")
endif()

# System Dependencies
find_package(Threads REQUIRED)
list(APPEND KOLOSAL_LINK_LIBRARIES Threads::Threads)

find_package(OpenMP QUIET)
if(OpenMP_FOUND AND TARGET OpenMP::OpenMP_CXX)
    list(APPEND KOLOSAL_LINK_LIBRARIES OpenMP::OpenMP_CXX)
endif()

# Platform-specific Libraries
if(WIN32)
    list(APPEND KOLOSAL_LINK_LIBRARIES ws2_32 wbemuuid)
elseif(APPLE)
    list(APPEND KOLOSAL_LINK_LIBRARIES dl pthread)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    if(COREFOUNDATION_FRAMEWORK)
        list(APPEND KOLOSAL_LINK_LIBRARIES ${COREFOUNDATION_FRAMEWORK})
    endif()
else()
    list(APPEND KOLOSAL_LINK_LIBRARIES dl rt pthread)
endif()

# Link all dependencies
target_link_libraries(kolosal_server PRIVATE ${KOLOSAL_LINK_LIBRARIES})
target_link_libraries(kolosal_server PUBLIC ${INFERENCE_TARGET})

if(TARGET llama)
    target_link_libraries(kolosal_server PRIVATE llama)
endif()

# ==============================================================================
# OPTIMIZATION AND PLATFORM-SPECIFIC SETTINGS
# ==============================================================================

# Optimization Flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(kolosal_server PRIVATE /O2 /Ob2)
    else()
        target_compile_options(kolosal_server PRIVATE -O3)
        if(ENABLE_NATIVE_OPTIMIZATION)
            target_compile_options(kolosal_server PRIVATE -march=native -mtune=native)
        endif()
    endif()
endif()

# Platform-specific RPATH Configuration
if(UNIX AND NOT APPLE)
    set_target_properties(kolosal_server kolosal_server_exe PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
elseif(APPLE)
    set_target_properties(kolosal_server_exe PROPERTIES
        INSTALL_RPATH "@executable_path/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        MACOSX_RPATH TRUE
    )
endif()

# ==============================================================================
# POST-BUILD COMMANDS AND DISTRIBUTION
# ==============================================================================

# Post-build Library Copying
add_custom_command(TARGET kolosal_server_exe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:kolosal_server>"
        "$<TARGET_FILE_DIR:kolosal_server_exe>/"
    COMMENT "Copying kolosal_server library to executable directory"
)

add_custom_command(TARGET kolosal_server_exe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${INFERENCE_TARGET}>"
        "$<TARGET_FILE_DIR:kolosal_server_exe>/"
    COMMENT "Copying ${INFERENCE_TARGET} to executable directory"
)

# Self-contained Distribution Package
set(KOLOSAL_FULL_DIST_DIR "${CMAKE_BINARY_DIR}/dist")
add_custom_target(dist
    DEPENDS kolosal_server_exe
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE_PATH="$<TARGET_FILE:kolosal_server_exe>"
        -DLIB_SERVER_PATH="$<TARGET_FILE:kolosal_server>"
        -DLIB_INFERENCE_PATH="$<TARGET_FILE:${INFERENCE_TARGET}>"
        -DDIST_DIR="${KOLOSAL_FULL_DIST_DIR}"
        -DMETAL_BACKEND=$<BOOL:${USE_METAL}>
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/overrides/MakeDist.cmake"
    COMMENT "Assembling self-contained distribution in dist/"
)

# ==============================================================================
# INSTALLATION RULES
# ==============================================================================

# macOS Application Bundle Installation
if(APPLE)
    install(TARGETS kolosal_server
        LIBRARY DESTINATION "Kolosal.app/Contents/Frameworks"
        COMPONENT Runtime
    )
    install(TARGETS kolosal_server_exe
        RUNTIME DESTINATION "Kolosal.app/Contents/MacOS"
        COMPONENT Runtime
    )
endif()

# Development Headers Installation (Optional)
if(INSTALL_HEADERS)
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()