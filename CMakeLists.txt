cmake_minimum_required(VERSION 3.16)

project(OpenIndyCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Xml SerialPort)

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

# --- plugin ABI version ---
#
# Deliberately NOT derived from the git version above. The product version
# should change on every commit; the plugin ABI must change only when the
# plugin contract changes. See include/plugin/pluginabi.h for the reasoning
# and for when to bump it.
#
# The header is the single source of truth. It is read here only so plugins
# can stamp the value into their metaInfo.json, letting the app report a
# mismatch by reading metadata instead of discovering it after a failed load.

set(OI_PLUGIN_ABI_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/pluginabi.h")
file(STRINGS "${OI_PLUGIN_ABI_HEADER}" OI_PLUGIN_ABI_LINE
     REGEX "^#define[ \t]+OI_PLUGIN_ABI_VERSION[ \t]+[0-9]+")
string(REGEX MATCH "[0-9]+$" OI_PLUGIN_ABI_VERSION_VALUE "${OI_PLUGIN_ABI_LINE}")
if(NOT OI_PLUGIN_ABI_VERSION_VALUE)
    message(FATAL_ERROR
        "Could not read OI_PLUGIN_ABI_VERSION from ${OI_PLUGIN_ABI_HEADER}. "
        "It must contain a line of the form '#define OI_PLUGIN_ABI_VERSION <integer>'.")
endif()

# CACHE INTERNAL so it crosses the FetchContent/add_subdirectory boundary into
# the plugin repos, which build core as a subproject.
set(OI_PLUGIN_ABI_VERSION "${OI_PLUGIN_ABI_VERSION_VALUE}" CACHE INTERNAL
    "OpenIndy plugin ABI version, read from include/plugin/pluginabi.h" FORCE)

message(STATUS "OpenIndy plugin ABI version: ${OI_PLUGIN_ABI_VERSION}")

# Bumping the header must re-run configure, otherwise the plugins' generated
# metaInfo.json keeps the stale number while their binaries get the new one -
# which would be a worse version of the bug this is fixing.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${OI_PLUGIN_ABI_HEADER}")

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

# --- third-party packages (provided by Conan, see conanfile.txt) ---

find_package(Protobuf CONFIG REQUIRED)

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
    src/plugin/function/pipelinestage.cpp
    src/plugin/networkAdjustment/bundleadjustment.cpp
    src/plugin/sensor/sensor.cpp
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
    src/reading.cpp
    src/sensorconfiguration.cpp
    src/sensortype.cpp
    src/connectionkind.cpp
    src/calibratedconstellation.cpp
    src/group.cpp
    src/pointgroup.cpp
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
    include/plugin/function/pipelinestage.h
    include/plugin/function/producertransform.h
    include/plugin/function/generatefeaturefunction.h
    include/plugin/function/geodeticfunction.h
    include/plugin/function/objecttransformation.h
    include/plugin/function/systemtransformation.h
    include/plugin/function/specialfunction.h
    include/plugin/sensor/lasertracker.h
    include/plugin/sensor/sensor.h
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
    include/measurementintent.h
    include/observation.h
    include/oijob.h
    include/oirequestresponse.h
    include/position.h
    include/radius.h
    include/reading.h
    include/sensorconfiguration.h
    include/sensortype.h
    include/connectionkind.h
    include/calibratedconstellation.h
    include/group.h
    include/pointgroup.h
    include/pointcollection.h
    include/sensorcontrol.h
    include/sensorworker.h
    include/sensorworkermessage.h
    include/station.h
    include/core_export.h
    include/progress.h
    include/snapshot.h
    include/statistic.h
    include/trafoparam.h
    include/plugin/networkAdjustment/bundleadjustment.h
    include/plugin/pluginabi.h
)

target_compile_definitions(OpenIndyCore
    PRIVATE
        OI_CORE_LIB
    PUBLIC
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
        OpenIndyMath
)

# --- codec: project file / wire format (protobuf) ---
#
# A separate static library so that the core model carries no protobuf and so
# that each final binary (the application, a command-line tool, a test) gets
# exactly one copy of the protobuf runtime. Mixing a runtime inside a shared
# library with another in the executable breaks reflection and descriptor
# lookup. The converters reach the model's private state through the
# codec::Access friend declared in the model classes.
add_library(OpenIndyCodec STATIC
    src/codec/convert.cpp
    src/codec/functions.cpp
    src/codec/geometry.cpp
    src/codec/entities.cpp
    src/codec/project.cpp
)
set_target_properties(OpenIndyCodec PROPERTIES POSITION_INDEPENDENT_CODE ON)
set(OI_PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
protobuf_generate(
    TARGET OpenIndyCodec
    LANGUAGE cpp
    PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/openindy/project.proto
    IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
    PROTOC_OUT_DIR ${OI_PROTO_GENERATED_DIR}
)
target_include_directories(OpenIndyCodec PUBLIC
    $<BUILD_INTERFACE:${OI_PROTO_GENERATED_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/codec>
)
target_link_libraries(OpenIndyCodec PUBLIC OpenIndyCore PUBLIC protobuf::libprotobuf)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(OpenIndyCodec PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- tests ---

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_reading test/reading/tst_reading.cpp)
target_compile_definitions(tst_reading PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/reading")
target_link_libraries(tst_reading PRIVATE Qt6::Test OpenIndyCodec OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_reading PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME reading COMMAND tst_reading)

# Header-only and deliberately Qt-free: no Qt::Test and no link to OpenIndyCore.
add_executable(tst_progress test/progress/tst_progress.cpp)
target_include_directories(tst_progress PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_progress PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME progress COMMAND tst_progress)

# Plain-assert test for the value types; no Qt::Test, only the library itself.
add_executable(tst_valuetypes test/valuetypes/tst_valuetypes.cpp)
target_link_libraries(tst_valuetypes PRIVATE OpenIndyCore OpenIndyMath)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_valuetypes PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME valuetypes COMMAND tst_valuetypes)

# Codec tests use Catch2 (from Conan). They reach the private conversion
# headers directly.
find_package(Catch2 3 CONFIG REQUIRED)
add_executable(tst_codec test/codec/tst_codec.cpp)
target_link_libraries(tst_codec PRIVATE
    OpenIndyCodec OpenIndyCore OpenIndyMath Catch2::Catch2WithMain)
add_test(NAME codec COMMAND tst_codec)

add_executable(tst_measurementconfig test/measurementconfig/tst_measurementconfig.cpp)
target_compile_definitions(tst_measurementconfig PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/measurementconfig")
target_link_libraries(tst_measurementconfig PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_measurementconfig PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME measurementconfig COMMAND tst_measurementconfig)

add_executable(tst_pointcollection test/pointcollection/tst_pointcollection.cpp)
target_link_libraries(tst_pointcollection PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_pointcollection PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME pointcollection COMMAND tst_pointcollection)

add_executable(tst_ownership test/ownership/tst_ownership.cpp)
target_link_libraries(tst_ownership PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_ownership PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ownership COMMAND tst_ownership)

add_executable(tst_ownedsubgeometry test/ownedsubgeometry/tst_ownedsubgeometry.cpp)
target_link_libraries(tst_ownedsubgeometry PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_ownedsubgeometry PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ownedsubgeometry COMMAND tst_ownedsubgeometry)

add_executable(tst_measurementintent test/measurementintent/tst_measurementintent.cpp)
target_link_libraries(tst_measurementintent PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_measurementintent PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME measurementintent COMMAND tst_measurementintent)

add_executable(tst_pluginabi test/pluginabi/tst_pluginabi.cpp)
target_link_libraries(tst_pluginabi PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)
target_compile_definitions(tst_pluginabi PRIVATE SOURCEDIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_pluginabi PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME pluginabi COMMAND tst_pluginabi)

add_executable(tst_pipeline test/pipeline/tst_pipeline.cpp)
target_link_libraries(tst_pipeline PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME pipeline COMMAND tst_pipeline)

add_executable(tst_typenames test/typenames/tst_typenames.cpp)
target_link_libraries(tst_typenames PRIVATE Qt6::Test OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_typenames PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME typenames COMMAND tst_typenames)
