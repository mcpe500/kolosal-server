# MakeDist.cmake
# Arguments (passed via -D):
# EXECUTABLE_PATH, LIB_SERVER_PATH, LIB_INFERENCE_PATH, DIST_DIR, METAL_BACKEND
# Produces a self-contained dist directory (macOS focused)

if(NOT DEFINED DIST_DIR)
  message(FATAL_ERROR "DIST_DIR not set")
endif()
file(REMOVE_RECURSE "${DIST_DIR}")
file(MAKE_DIRECTORY "${DIST_DIR}")
file(MAKE_DIRECTORY "${DIST_DIR}/lib")

# Copy main artifacts
foreach(_f IN ITEMS EXECUTABLE_PATH LIB_SERVER_PATH LIB_INFERENCE_PATH)
  if(DEFINED ${_f} AND EXISTS "${${_f}}")
    file(COPY "${${_f}}" DESTINATION "${DIST_DIR}")
  endif()
endforeach()

# Try to copy Metal shader if present
if(METAL_BACKEND)
  set(_METAL_CANDIDATES
    "${CMAKE_BINARY_DIR}/bin/ggml-metal.metal"
    "${CMAKE_BINARY_DIR}/inference/external/llama.cpp/ggml/src/ggml-metal.metal"
    "${CMAKE_SOURCE_DIR}/inference/external/llama.cpp/ggml/src/ggml-metal.metal"
  )
  foreach(_m IN LISTS _METAL_CANDIDATES)
    if(EXISTS "${_m}")
      file(COPY "${_m}" DESTINATION "${DIST_DIR}")
      break()
    endif()
  endforeach()
endif()

# Copy sample config files if present
foreach(_cfg IN ITEMS config.yaml config.json config_rms.yaml local-retrieval-config.yaml)
  if(EXISTS "${CMAKE_SOURCE_DIR}/configs/${_cfg}")
    file(COPY "${CMAKE_SOURCE_DIR}/configs/${_cfg}" DESTINATION "${DIST_DIR}")
  endif()
endforeach()

# Function to run otool -L and parse dependencies
function(get_deps input out_var)
  if(NOT EXISTS "${input}")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()
  execute_process(COMMAND otool -L "${input}" OUTPUT_VARIABLE _raw OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Split lines explicitly
  string(REPLACE "\n" ";" _lines "${_raw}")
  set(_deps "")
  foreach(line IN LISTS _lines)
    string(STRIP "${line}" line)
    # Skip empty lines, header line ending with ':' and @rpath lines
    if(line STREQUAL "" OR line MATCHES ":$" OR line MATCHES "^@")
      continue()
    endif()
    # Extract absolute path before first space
    if(line MATCHES "^/[^ ]+")
      string(REGEX MATCH "^/[^ ]+" path "${line}")
      if(path)
        if(NOT path STREQUAL "${input}" AND
           NOT path MATCHES "/System/Library/Frameworks" AND
           NOT path MATCHES "/usr/lib/" AND
           NOT path MATCHES ".sdk/usr/lib")
          list(APPEND _deps "${path}")
        endif()
      endif()
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _deps)
  set(${out_var} "${_deps}" PARENT_SCOPE)
endfunction()

set(_QUEUE)
foreach(_artifact IN ITEMS EXECUTABLE_PATH LIB_SERVER_PATH LIB_INFERENCE_PATH)
  if(DEFINED ${_artifact} AND EXISTS "${${_artifact}}")
    list(APPEND _QUEUE "${${_artifact}}")
  endif()
endforeach()

set(_COPIED)
while(_QUEUE)
  list(POP_FRONT _QUEUE current)
  get_deps("${current}" deps)
  foreach(d IN LISTS deps)
    if(NOT d IN_LIST _COPIED)
      # Copy into lib subdir
      get_filename_component(_name "${d}" NAME)
      set(_dest "${DIST_DIR}/lib/${_name}")
      file(COPY "${d}" DESTINATION "${DIST_DIR}/lib")
      # If dependency is a symlink, also copy its real target (versioned filename) so link isn't dangling
      if(IS_SYMLINK "${d}")
        get_filename_component(_real "${d}" REALPATH)
        if(EXISTS "${_real}")
          get_filename_component(_real_name "${_real}" NAME)
          if(NOT EXISTS "${DIST_DIR}/lib/${_real_name}")
            file(COPY "${_real}" DESTINATION "${DIST_DIR}/lib")
          endif()
        endif()
      endif()
      list(APPEND _COPIED "${d}")
      list(APPEND _QUEUE "${_dest}") # process copied one (install names adjust still needed)
    endif()
  endforeach()
endwhile()

# Adjust install names to @loader_path
function(fix_ids path)
  if(NOT EXISTS "${path}")
    return()
  endif()
  execute_process(COMMAND otool -D "${path}" OUTPUT_VARIABLE _id_raw OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCHALL "^.+$" _lines "${_id_raw}")
  list(LENGTH _lines _len)
  if(_len GREATER 1)
    list(GET _lines 1 old_id)
    get_filename_component(base "${path}" NAME)
    set(new_id "@loader_path/${base}")
    execute_process(COMMAND install_name_tool -id "${new_id}" "${path}")
  endif()
endfunction()

# Fix ids for copied dylibs
file(GLOB _dylibs "${DIST_DIR}/lib/*.dylib")
foreach(dyl IN LISTS _dylibs)
  fix_ids("${dyl}")
endforeach()

# Adjust references inside each binary (exe + libs + local dylibs)
set(_ALL_BINARIES)
file(GLOB _lib_dylibs "${DIST_DIR}/lib/*.dylib")
list(APPEND _ALL_BINARIES ${_lib_dylibs})
foreach(_main IN ITEMS EXECUTABLE_PATH LIB_SERVER_PATH LIB_INFERENCE_PATH)
  if(DEFINED ${_main})
    get_filename_component(_n "${${_main}}" NAME)
    if(EXISTS "${DIST_DIR}/${_n}")
      list(APPEND _ALL_BINARIES "${DIST_DIR}/${_n}")
    endif()
  endif()
endforeach()

foreach(bin IN LISTS _ALL_BINARIES)
  get_deps("${bin}" deps)
  foreach(dep IN LISTS deps)
    get_filename_component(dep_name "${dep}" NAME)
    if(EXISTS "${DIST_DIR}/lib/${dep_name}")
      execute_process(COMMAND install_name_tool -change "${dep}" "@loader_path/lib/${dep_name}" "${bin}")
    endif()
  endforeach()
endforeach()

# README
file(WRITE "${DIST_DIR}/README.txt" "kolosal-server distribution\n\nRun ./kolosal-server\n")

message(STATUS "Created self-contained distribution at ${DIST_DIR}")
