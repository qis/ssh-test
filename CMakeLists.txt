cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(ssh VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "-static$")
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER CMAKE_CXX_FLAGS_${config} name)
    string(REPLACE "/MD" "/MT" ${name} ${${name}})
  endforeach()
endif()

if(MSVC)
  string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "/std:c++latest /await ${CMAKE_CXX_FLAGS} /utf-8 /wd4100")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifestuac:NO /ignore:4098 /ignore:4099")
  add_definitions(-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_ATL_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_HAS_EXCEPTIONS=_CPPUNWIND)
  add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fcoroutines-ts")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-absolute-paths -fcolor-diagnostics")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GS- /GL")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
  set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fuse-ld=lld -flto")
endif()

if(MSVC)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER build)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

file(GLOB_RECURSE headers include/ssh/*.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/ssh PREFIX include FILES ${headers})

file(GLOB_RECURSE sources src/ssh/*.h src/ssh/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src/ssh PREFIX src FILES ${sources})

add_library(ssh STATIC ${headers} ${sources})
target_include_directories(ssh PRIVATE ${CMAKE_CURRENT_BINARY_DIR} src PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

find_package(LibSSH REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC LibSSH::LibSSH)

#find_package(LibSSH2 REQUIRED)
#target_link_libraries(${PROJECT_NAME} PUBLIC LibSSH2::LibSSH2)

if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ssh-${PROJECT_NAME}-$<LOWER_CASE:$<CONFIG>>)
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ssh-${PROJECT_NAME})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(ssh PUBLIC dl)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/ssh-config.cmake)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ssh-config.cmake
    [[include("${CMAKE_CURRENT_LIST_DIR}/ssh-targets.cmake")]])
endif()

install(TARGETS ${PROJECT_NAME} EXPORT ssh
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(FILES ${headers} DESTINATION include/ssh)
install(EXPORT ssh FILE ssh-targets.cmake NAMESPACE ssh:: DESTINATION lib/cmake)

add_executable(main src/main.cpp)
source_group("" FILES src/main.cpp)
target_link_libraries(main PRIVATE ssh)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
set_target_properties(main PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
