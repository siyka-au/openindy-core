cmake_minimum_required(VERSION 3.16)

project(OpenIndyCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Xml SerialPort Svg Network)

# --- version, derived from git describe (mirrors the old version.pri logic) ---

find_package(Git QUIET)
set(OI_CORE_GIT_VERSION "0.0.0")
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OI_CORE_GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE OI_CORE_GIT_RESULT
    )
    if(OI_CORE_GIT_RESULT EQUAL 0 AND OI_CORE_GIT_DESCRIBE)
        set(OI_CORE_GIT_VERSION "${OI_CORE_GIT_DESCRIBE}")
    endif()
endif()

# strip "-g<hash>" suffix, then turn the remaining hyphen into a dot
string(REGEX REPLACE "-g[0-9a-f]+$" "" OI_CORE_GIT_VERSION "${OI_CORE_GIT_VERSION}")
string(REGEX REPLACE "-" "." OI_CORE_GIT_VERSION "${OI_CORE_GIT_VERSION}")

set(OPENINDY_VERSION "${OI_CORE_GIT_VERSION}" CACHE STRING
    "Version string embedded via the OPENINDY_VERSION define")

string(REGEX REPLACE "[.]" "" PLUGIN_INTERFACE_VERSION "${OI_CORE_GIT_VERSION}")
if(NOT PLUGIN_INTERFACE_VERSION)
    set(PLUGIN_INTERFACE_VERSION 0)
endif()

# --- Math dependency ---

include(FetchContent)

set(OI_MATH_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../openindy-math" CACHE PATH
    "Local OpenIndy-Math checkout to build against instead of fetching from git")

if(EXISTS "${OI_MATH_LOCAL_DIR}/CMakeLists.txt")
    FetchContent_Declare(OpenIndyMath SOURCE_DIR "${OI_MATH_LOCAL_DIR}")
else()
    FetchContent_Declare(OpenIndyMath
        GIT_REPOSITORY git@github.com:siyka-au/openindy-math.git
        GIT_TAG        master
    )
endif()
FetchContent_MakeAvailable(OpenIndyMath)

# --- library ---

add_library(OpenIndyCore SHARED
    src/geometry/circle.cpp
    src/geometry/cone.cpp
    src/geometry/cylinder.cpp
    src/geometry/ellipse.cpp
    src/geometry/ellipsoid.cpp
    src/geometry/hyperboloid.cpp
    src/geometry/line.cpp
    src/geometry/nurbs.cpp
    src/geometry/paraboloid.cpp
    src/geometry/plane.cpp
    src/geometry/point.cpp
    src/geometry/pointcloud.cpp
    src/geometry/scalarentityangle.cpp
    src/geometry/scalarentitydistance.cpp
    src/geometry/scalarentitymeasurementseries.cpp
    src/geometry/scalarentitytemperature.cpp
    src/geometry/slottedhole.cpp
    src/geometry/sphere.cpp
    src/geometry/torus.cpp
    src/plugin/exchange/exchangedefinedformat.cpp
    src/plugin/exchange/exchangeinterface.cpp
    src/plugin/exchange/exchangesimpleascii.cpp
    src/plugin/function/function.cpp
    src/plugin/networkAdjustment/bundleadjustment.cpp
    src/plugin/sensor/sensor.cpp
    src/plugin/sensor/sensorfacade.cpp
    src/plugin/simulation/simulationmodel.cpp
    src/plugin/tool/tool.cpp
    src/util/util.cpp
    src/coordinatesystem.cpp
    src/direction.cpp
    src/element.cpp
    src/feature.cpp
    src/featurecontainer.cpp
    src/featurewrapper.cpp
    src/geometry.cpp
    src/measurementconfig.cpp
    src/observation.cpp
    src/oijob.cpp
    src/position.cpp
    src/radius.cpp
    src/reading.cpp
    src/sensorconfiguration.cpp
    src/sensorcontrol.cpp
    src/sensorworker.cpp
    src/sensorworkermessage.cpp
    src/station.cpp
    src/statistic.cpp
    src/trafoparam.cpp

    include/geometry/circle.h
    include/geometry/cone.h
    include/geometry/cylinder.h
    include/geometry/ellipse.h
    include/geometry/ellipsoid.h
    include/geometry/hyperboloid.h
    include/geometry/line.h
    include/geometry/nurbs.h
    include/geometry/paraboloid.h
    include/geometry/plane.h
    include/geometry/point.h
    include/geometry/pointcloud.h
    include/geometry/scalarentityangle.h
    include/geometry/scalarentitydistance.h
    include/geometry/scalarentitymeasurementseries.h
    include/geometry/scalarentitytemperature.h
    include/geometry/slottedhole.h
    include/geometry/sphere.h
    include/geometry/torus.h
    include/plugin/exchange/exchangedefinedformat.h
    include/plugin/exchange/exchangeinterface.h
    include/plugin/exchange/exchangesimpleascii.h
    include/plugin/plugin.h
    include/plugin/pluginmetadata.h
    include/plugin/function/constructfunction.h
    include/plugin/function/fitfunction.h
    include/plugin/function/function.h
    include/plugin/function/generatefeaturefunction.h
    include/plugin/function/geodeticfunction.h
    include/plugin/function/objecttransformation.h
    include/plugin/function/systemtransformation.h
    include/plugin/function/specialfunction.h
    include/plugin/sensor/lasertracker.h
    include/plugin/sensor/sensor.h
    include/plugin/sensor/sensorfacade.h
    include/plugin/sensor/totalstation.h
    include/plugin/simulation/simulationmodel.h
    include/plugin/tool/tool.h
    include/util/types.h
    include/util/util.h
    include/coordinatesystem.h
    include/direction.h
    include/element.h
    include/feature.h
    include/featureattributes.h
    include/featurecontainer.h
    include/featurewrapper.h
    include/geometry.h
    include/measurementconfig.h
    include/observation.h
    include/oijob.h
    include/oirequestresponse.h
    include/position.h
    include/radius.h
    include/reading.h
    include/sensorconfiguration.h
    include/sensorcontrol.h
    include/sensorworker.h
    include/sensorworkermessage.h
    include/station.h
    include/statistic.h
    include/trafoparam.h
    include/plugin/networkAdjustment/bundleadjustment.h
)

target_compile_definitions(OpenIndyCore
    PRIVATE
        OI_CORE_LIB
    PUBLIC
        PLUGIN_INTERFACE_VERSION=${PLUGIN_INTERFACE_VERSION}
        OPENINDY_VERSION="${OPENINDY_VERSION}"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(OpenIndyCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_include_directories(OpenIndyCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/geometry>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/util>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/exchange>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/function>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/networkAdjustment>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/sensor>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/simulation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/tool>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(OpenIndyCore
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Xml
        Qt6::SerialPort
        Qt6::Svg
        Qt6::Network
        OpenIndyMath
)

# --- tests ---

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_reading test/reading/tst_reading.cpp)
target_compile_definitions(tst_reading PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/reading")
target_link_libraries(tst_reading PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_reading PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME reading COMMAND tst_reading)
