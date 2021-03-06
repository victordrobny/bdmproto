cmake_minimum_required(VERSION 2.8.9)

#-------------------- set compilers --------------
#set(CMAKE_C_COMPILER "/usr/local/bin/gcc-6")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-6")

project(BioDynaMo)

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.
option(valgrind "Enable valgrind tests and make build compatible with valgrind tool." ON)
option(coverage "Enable test coverage report generation." OFF)



#--------------------- find packages -------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(Vc REQUIRED)
find_package(OpenMP REQUIRED)
# find_package(VTune REQUIRED)
if (test)
  # must be before include(${ROOT_USE_FILE}) otherwise test compile fails
  find_package(Threads REQUIRED)
endif()
#find_package(ROOT REQUIRED COMPONENTS Eve Geom Gui)

#--------------------- includes -------------
include(ExternalProject)
include(cmake/doxygen.cmake)

#--------------------- check if compiler supports C++11 -------------
# http://stackoverflow.com/questions/10851247/how-to-activate-c-11-in-cmake
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

#--------------------- set compiler flags -------------
if (coverage)
  set(CMAKE_BUILD_TYPE "Debug")
  set(GCC_COMPILE_FLAGS "-Wall -Wno-missing-braces -g -O0 -fopenmp -fprofile-arcs -ftest-coverage")
else()
  set(GCC_COMPILE_FLAGS "-Wall -Wno-missing-braces -fopenmp -ldl")
endif()
set(GCC_LINK_FLAGS    "")
# append
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_LINK_FLAGS}")
# special clang flag
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  # silence unsupported -pthread argument at link time
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
endif()

#--------------------- set rpath options -------------
# When building, use the RPATH
set(CMAKE_SKIP_BUILD_RPATH FALSE)          # don't skip the full RPATH for the build tree
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)   # use always the build RPATH for the build tree
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # point to directories outside the build tree to the install RPATH

if(APPLE)
  set(CMAKE_MACOSX_RPATH TRUE)                 # use RPATH for MacOSX
  set(CMAKE_INSTALL_NAME_DIR "@rpath")
  set(CMAKE_INSTALL_RPATH "@loader_path/")     # self relative LIBDIR
else()
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}) # install LIBDIR
endif()

#--------------------- build shared library libbiodynamo ----------------
# add the headers into the project
#include(${ROOT_USE_FILE})
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/third_party")
include_directories(${Vc_INCLUDE_DIR})
# include_directories(${VTUNE_INCLUDE_DIR})

#ROOT_GENERATE_DICTIONARY("${CMAKE_BINARY_DIR}/bdmDict"
#    simulation/ecm.h
#    LINKDEF bdmLinkDef.h)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cc")
file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/src/*.h")
#${CMAKE_BINARY_DIR}/bdmDict.cxx)
add_library(biodynamo SHARED ${SOURCES} ${HEADERS})
#target_link_libraries(biodynamo ${ROOT_LIBRARIES})
target_link_libraries(biodynamo ${Vc_LIBRARIES})
# target_link_libraries(biodynamo ${VTUNE_LIBRARIES})

# add custom clean target for shared library
add_custom_target(libbdmclean COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/CMakeFiles/biodynamo.dir/cmake_clean.cmake")
# custom clean target for biodynamo that excludes ExternalProjects
add_custom_target(bdmclean)
add_dependencies(bdmclean libbdmclean)

# --------------------- build test project runBiodynamoTests ----------------------
# http://www.kaizou.org/2014/11/gtest-cmake/
if (test)
  enable_testing()

  # setup google test
  ExternalProject_Add(
      gtest
      URL "${CMAKE_SOURCE_DIR}/third_party/gtest-1.7.0.zip"
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest"
      CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      INSTALL_COMMAND "" # Disable install step
  )
  ExternalProject_Get_Property(gtest source_dir binary_dir)

  # Create a libgtest target to be used as a dependency by test program
  add_library(libgtest IMPORTED STATIC GLOBAL)
  add_dependencies(libgtest gtest)
  set_target_properties(libgtest PROPERTIES
      IMPORTED_LOCATION "${binary_dir}/libgtest.a"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
  )

  # add include directories for gtest
  include_directories("${CMAKE_BINARY_DIR}/gtest/src/gtest/include")

  # create test executable
  file(GLOB_RECURSE TEST_SOURCES ${CMAKE_SOURCE_DIR}/test/*.cc)
  set(TEST_TARGET runBiodynamoTests)
  add_executable(${TEST_TARGET} ${TEST_SOURCES})
  target_link_libraries(${TEST_TARGET} libgtest)
  target_link_libraries(${TEST_TARGET} biodynamo)

  # execute all tests with command: make test
  add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
  # add valgrind test
  if (valgrind)
    add_test(NAME valgrind COMMAND valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./${TEST_TARGET})
  endif()

  # create target that shows the test ouput on failure
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}  --force-new-ctest-process --output-on-failure)

  # add custom clean target for test project
  add_custom_target(testbdmclean COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/CMakeFiles/${TEST_TARGET}.dir/cmake_clean.cmake")
  add_dependencies(bdmclean testbdmclean)

  if (coverage)
    include(CodeCoverage)
    # parameter: make target name, test target, output directory name
    SETUP_TARGET_FOR_COVERAGE(coverage ${TEST_TARGET} coverage)
  endif()
endif()

# --------------------- build division module simulation executable ---------------------
set(SIMULATION_TARGET bdmp)
add_executable(${SIMULATION_TARGET} ${CMAKE_SOURCE_DIR}/src/main.cc)
target_link_libraries(${SIMULATION_TARGET} biodynamo)
