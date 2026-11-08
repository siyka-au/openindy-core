cmake_minimum_required(VERSION 3.16)

project(OpenIndyCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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
# plugin contract changes. See include/plugin/plugin_abi.h for the reasoning
# and for when to bump it.
#
# The header is the single source of truth. It is read here only so plugins
# can stamp the value into their meta_info.json, letting the app report a
# mismatch by reading metadata instead of discovering it after a failed load.

set(OI_PLUGIN_ABI_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/plugin_abi.h")
file(STRINGS "${OI_PLUGIN_ABI_HEADER}" OI_PLUGIN_ABI_LINE
     REGEX "^inline constexpr int kPluginAbiVersion[ \t]*=[ \t]*[0-9]+")
string(REGEX MATCH "[0-9]+" OI_PLUGIN_ABI_VERSION_VALUE "${OI_PLUGIN_ABI_LINE}")
if(NOT OI_PLUGIN_ABI_VERSION_VALUE)
    message(FATAL_ERROR
        "Could not read kPluginAbiVersion from ${OI_PLUGIN_ABI_HEADER}. "
        "It must contain a line of the form 'inline constexpr int kPluginAbiVersion = <integer>;'.")
endif()

# CACHE INTERNAL so it crosses the FetchContent/add_subdirectory boundary into
# the plugin repos, which build core as a subproject.
set(OI_PLUGIN_ABI_VERSION "${OI_PLUGIN_ABI_VERSION_VALUE}" CACHE INTERNAL
    "OpenIndy plugin ABI version, read from include/plugin/plugin_abi.h" FORCE)

message(STATUS "OpenIndy plugin ABI version: ${OI_PLUGIN_ABI_VERSION}")

# Bumping the header must re-run configure, otherwise the plugins' generated
# meta_info.json keeps the stale number while their binaries get the new one -
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
find_package(spdlog CONFIG REQUIRED)

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
    src/geometry/point_cloud.cpp
    src/geometry/scalar_entity_angle.cpp
    src/geometry/scalar_entity_distance.cpp
    src/geometry/scalar_entity_measurement_series.cpp
    src/geometry/scalar_entity_temperature.cpp
    src/geometry/slotted_hole.cpp
    src/geometry/sphere.cpp
    src/geometry/torus.cpp
    src/plugin/exchange/exchange_defined_format.cpp
    src/plugin/exchange/exchange_interface.cpp
    src/plugin/exchange/exchange_simple_ascii.cpp
    src/plugin/function/function.cpp
    src/plugin/function/pipeline_stage.cpp
    src/plugin/plugin_loader.cpp
    src/plugin/network_adjustment/bundle_adjustment.cpp
    src/plugin/sensor/sensor.cpp
    src/plugin/simulation/simulation_model.cpp
    src/util/util.cpp
    src/coordinate_system.cpp
    src/direction.cpp
    src/element.cpp
    src/executor.cpp
    src/log.cpp
    src/feature.cpp
    src/feature_container.cpp
    src/feature_wrapper.cpp
    src/geometry.cpp
    src/measurement_config.cpp
    src/observation.cpp
    src/oi_job.cpp
    src/position.cpp
    src/reading.cpp
    src/sensor_configuration.cpp
    src/sensor_type.cpp
    src/connection_kind.cpp
    src/calibrated_constellation.cpp
    src/group.cpp
    src/point_group.cpp
    src/sensor_control.cpp
    src/sensor_worker.cpp
    src/sensor_worker_message.cpp
    src/station.cpp
    src/statistic.cpp
    src/trafo_param.cpp

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
    include/geometry/point_cloud.h
    include/geometry/scalar_entity_angle.h
    include/geometry/scalar_entity_distance.h
    include/geometry/scalar_entity_measurement_series.h
    include/geometry/scalar_entity_temperature.h
    include/geometry/slotted_hole.h
    include/geometry/sphere.h
    include/geometry/torus.h
    include/plugin/exchange/exchange_defined_format.h
    include/plugin/exchange/exchange_interface.h
    include/plugin/exchange/exchange_observer.h
    include/plugin/exchange/exchange_simple_ascii.h
    include/plugin/plugin.h
    include/plugin/plugin_loader.h
    include/plugin/toolchain.h
    include/plugin/plugin_metadata.h
    include/plugin/function/construct_function.h
    include/plugin/function/fit_function.h
    include/plugin/function/function.h
    include/plugin/function/function_observer.h
    include/plugin/function/pipeline_stage.h
    include/plugin/function/producer_transform.h
    include/plugin/function/generate_feature_function.h
    include/plugin/function/geodetic_function.h
    include/plugin/function/object_transformation.h
    include/plugin/function/system_transformation.h
    include/plugin/function/special_function.h
    include/plugin/sensor/laser_tracker.h
    include/plugin/sensor/sensor.h
    include/plugin/sensor/sensor_sink.h
    include/plugin/sensor/total_station.h
    include/plugin/simulation/simulation_model.h
    include/util/types.h
    include/util/util.h
    include/coordinate_system.h
    include/direction.h
    include/element.h
    include/feature.h
    include/feature_attributes.h
    include/feature_observer.h
    include/job_observer.h
    include/log.h
    include/message_sink.h
    include/executor.h
    include/station_observer.h
    include/feature_container.h
    include/feature_wrapper.h
    include/geometry.h
    include/measurement_config.h
    include/measurement_intent.h
    include/observation.h
    include/oi_job.h
    include/oi_request_response.h
    include/position.h
    include/radius.h
    include/reading.h
    include/sensor_configuration.h
    include/sensor_type.h
    include/connection_kind.h
    include/calibrated_constellation.h
    include/group.h
    include/guarded.h
    include/point_group.h
    include/point_collection.h
    include/sensor_control.h
    include/sensor_worker.h
    include/sensor_worker_message.h
    include/station.h
    include/core_export.h
    include/progress.h
    include/snapshot.h
    include/statistic.h
    include/trafo_param.h
    include/plugin/network_adjustment/bundle_adjustment.h
    include/plugin/plugin_abi.h
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
# Source files and string literals are UTF-8 (std::string holds UTF-8); MSVC
# needs to be told, and consumers of the headers must compile the same way.
target_compile_options(OpenIndyCore PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_include_directories(OpenIndyCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/geometry>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/util>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/exchange>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/function>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/network_adjustment>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/sensor>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/plugin/simulation>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(OpenIndyCore
    PRIVATE
        ${CMAKE_DL_LIBS}
        spdlog::spdlog
    PUBLIC
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
    src/codec/config.cpp
    src/codec/convert.cpp
    src/codec/functions.cpp
    src/codec/geometry.cpp
    src/codec/entities.cpp
    src/codec/network.cpp
    src/codec/project.cpp
)
set_target_properties(OpenIndyCodec PROPERTIES POSITION_INDEPENDENT_CODE ON)
set(OI_PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
protobuf_generate(
    TARGET OpenIndyCodec
    LANGUAGE cpp
    PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/openindy/project.proto
           ${CMAKE_CURRENT_SOURCE_DIR}/proto/openindy/network.proto
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
target_compile_options(OpenIndyCodec PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# --- tests ---

enable_testing()

find_package(Catch2 3 CONFIG REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test/common)

add_executable(tst_reading test/reading/tst_reading.cpp)
target_compile_definitions(tst_reading PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/reading")
target_link_libraries(tst_reading PRIVATE Catch2::Catch2WithMain OpenIndyCodec OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_reading PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME reading COMMAND tst_reading)

# Header-only: no link to OpenIndyCore.
add_executable(tst_progress test/progress/tst_progress.cpp)
target_link_libraries(tst_progress PRIVATE Catch2::Catch2WithMain)
target_include_directories(tst_progress PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_progress PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME progress COMMAND tst_progress)

add_executable(tst_valuetypes test/value_types/tst_value_types.cpp)
target_link_libraries(tst_valuetypes PRIVATE OpenIndyCore OpenIndyMath Catch2::Catch2WithMain)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_valuetypes PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME valuetypes COMMAND tst_valuetypes)

# Codec tests reach the private conversion headers directly.
add_executable(tst_codec test/codec/tst_codec.cpp)
target_link_libraries(tst_codec PRIVATE
    OpenIndyCodec OpenIndyCore OpenIndyMath Catch2::Catch2WithMain)
add_test(NAME codec COMMAND tst_codec)

add_executable(tst_strings test/strings/tst_strings.cpp)
target_link_libraries(tst_strings PRIVATE Catch2::Catch2WithMain)
target_include_directories(tst_strings PRIVATE include)
add_test(NAME strings COMMAND tst_strings)

add_executable(tst_featurenames test/feature_names/tst_feature_names.cpp)
target_link_libraries(tst_featurenames PRIVATE OpenIndyCore OpenIndyMath Catch2::Catch2WithMain)
add_test(NAME featurenames COMMAND tst_featurenames)

add_executable(tst_guarded test/guarded/tst_guarded.cpp)
target_include_directories(tst_guarded PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tst_guarded PRIVATE Catch2::Catch2WithMain)
add_test(NAME guarded COMMAND tst_guarded)

add_executable(tst_measurementconfig test/measurement_config/tst_measurement_config.cpp)
target_compile_definitions(tst_measurementconfig PRIVATE SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/test/measurement_config")
target_link_libraries(tst_measurementconfig PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_measurementconfig PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME measurementconfig COMMAND tst_measurementconfig)

add_executable(tst_pointcollection test/point_collection/tst_point_collection.cpp)
target_link_libraries(tst_pointcollection PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_pointcollection PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME pointcollection COMMAND tst_pointcollection)

add_executable(tst_ownership test/ownership/tst_ownership.cpp)
target_link_libraries(tst_ownership PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_ownership PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ownership COMMAND tst_ownership)

add_executable(tst_featurestorage test/feature_storage/tst_feature_storage.cpp)
target_link_libraries(tst_featurestorage PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_featurestorage PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME featurestorage COMMAND tst_featurestorage)

add_executable(tst_featureobserver test/feature_observer/tst_feature_observer.cpp)
target_link_libraries(tst_featureobserver PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_featureobserver PRIVATE test/common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_featureobserver PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME featureobserver COMMAND tst_featureobserver)

add_executable(tst_jobobserver test/job_observer/tst_job_observer.cpp)
target_link_libraries(tst_jobobserver PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_jobobserver PRIVATE test/common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_jobobserver PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME jobobserver COMMAND tst_jobobserver)

add_executable(tst_savedchanges test/job_observer/tst_saved_changes.cpp)
target_link_libraries(tst_savedchanges PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_savedchanges PRIVATE test/common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_savedchanges PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME savedchanges COMMAND tst_savedchanges)

add_executable(tst_messagesink test/message_sink/tst_message_sink.cpp)
target_link_libraries(tst_messagesink PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_messagesink PRIVATE test/common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_messagesink PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME messagesink COMMAND tst_messagesink)

add_executable(tst_ownedsubgeometry test/owned_subgeometry/tst_owned_subgeometry.cpp)
target_link_libraries(tst_ownedsubgeometry PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_ownedsubgeometry PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ownedsubgeometry COMMAND tst_ownedsubgeometry)

add_executable(tst_measurementintent test/measurement_intent/tst_measurement_intent.cpp)
target_link_libraries(tst_measurementintent PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_measurementintent PRIVATE test/common)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_measurementintent PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME measurementintent COMMAND tst_measurementintent)

add_executable(tst_sensorworker test/sensor_worker/tst_sensor_worker.cpp)
target_link_libraries(tst_sensorworker PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)
target_include_directories(tst_sensorworker PRIVATE test/common)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tst_sensorworker PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME sensorworker COMMAND tst_sensorworker)

add_executable(tst_pipeline test/pipeline/tst_pipeline.cpp)
target_link_libraries(tst_pipeline PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME pipeline COMMAND tst_pipeline)

add_executable(tst_typenames test/type_names/tst_type_names.cpp)
target_link_libraries(tst_typenames PRIVATE Catch2::Catch2WithMain OpenIndyCore OpenIndyMath)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tst_typenames PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME typenames COMMAND tst_typenames)

# Plugin loader: tiny plugins built here (a good one, one with the wrong ABI
# version, one without entry points) are loaded by path.
add_library(oi_test_plugin_good MODULE test/plugin_loader/good_plugin.cpp)
add_library(oi_test_plugin_wrong_abi MODULE test/plugin_loader/wrong_abi_plugin.cpp)
add_library(oi_test_plugin_no_entry MODULE test/plugin_loader/no_entry_plugin.cpp)
# same source as the good plugin, but claiming a foreign toolchain
add_library(oi_test_plugin_wrong_toolchain MODULE test/plugin_loader/good_plugin.cpp)
target_compile_definitions(oi_test_plugin_wrong_toolchain PRIVATE
    OI_TOOLCHAIN_OVERRIDE="itanium-libfoo-plan9-m68k")
# a current-ABI plugin that predates the toolchain entry point
add_library(oi_test_plugin_no_toolchain MODULE test/plugin_loader/no_toolchain_plugin.cpp)
foreach(t oi_test_plugin_good oi_test_plugin_wrong_abi oi_test_plugin_no_entry
          oi_test_plugin_wrong_toolchain oi_test_plugin_no_toolchain)
    target_link_libraries(${t} PRIVATE OpenIndyCore OpenIndyMath)
    set_target_properties(${t} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test-plugins)
endforeach()
add_executable(tst_pluginloader test/plugin_loader/tst_plugin_loader.cpp)
target_link_libraries(tst_pluginloader PRIVATE OpenIndyCore OpenIndyMath Catch2::Catch2WithMain)
target_compile_definitions(tst_pluginloader PRIVATE
    OI_TEST_GOOD_PLUGIN="$<TARGET_FILE:oi_test_plugin_good>"
    OI_TEST_WRONG_ABI_PLUGIN="$<TARGET_FILE:oi_test_plugin_wrong_abi>"
    OI_TEST_NO_ENTRY_PLUGIN="$<TARGET_FILE:oi_test_plugin_no_entry>"
    OI_TEST_WRONG_TOOLCHAIN_PLUGIN="$<TARGET_FILE:oi_test_plugin_wrong_toolchain>"
    OI_TEST_NO_TOOLCHAIN_PLUGIN="$<TARGET_FILE:oi_test_plugin_no_toolchain>"
    OI_TEST_NOT_A_LIBRARY="${CMAKE_CURRENT_SOURCE_DIR}/test/plugin_loader/tst_plugin_loader.cpp")
add_dependencies(tst_pluginloader oi_test_plugin_good oi_test_plugin_wrong_abi oi_test_plugin_no_entry
    oi_test_plugin_wrong_toolchain oi_test_plugin_no_toolchain)
add_test(NAME pluginloader COMMAND tst_pluginloader)

add_executable(tst_log test/log/tst_log.cpp)
target_link_libraries(tst_log PRIVATE OpenIndyCore Catch2::Catch2WithMain)
add_test(NAME log COMMAND tst_log)

# --- oi-cli: end-to-end example, Qt-free ---
#
# Loads a project, runs a fit through a compute plugin (oi::PluginLoader),
# saves and reloads the protobuf. The CTest runs it against the plugin given
# by -DOI_CLI_PLUGIN=<library> (default: the environment variable
# OI_DEFAULT_PLUGIN at configure time) and is skipped (exit code 77) without one. openindy-default-plugin registers
# the same test with its own library.
option(OI_BUILD_CLI "Build the oi-cli example" ON)
if(OI_BUILD_CLI)
    add_executable(oi-cli tools/oi_cli/main.cpp)
    target_link_libraries(oi-cli PRIVATE OpenIndyCodec OpenIndyCore OpenIndyMath)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(oi-cli PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    set(OI_CLI_PLUGIN "$ENV{OI_DEFAULT_PLUGIN}" CACHE FILEPATH
        "Plugin library the oi-cli test runs against")
    add_test(NAME oi_cli
        COMMAND oi-cli --selftest --plugin "${OI_CLI_PLUGIN}")
    set_tests_properties(oi_cli PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Core and math must stay Qt-free: fail on any Qt include, identifier or CMake
# package in their sources, tests and build files.
add_test(NAME no_qt
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check-no-qt.sh
            ${CMAKE_CURRENT_SOURCE_DIR} ${OI_MATH_LOCAL_DIR})

# --- Plugin ABI check (libabigail), optional ---
#
# scripts/abi-check.sh compares libOpenIndyCore's public ABI with the committed
# abi/baseline.abi. It needs libabigail (abidw, abidiff) and a build with debug
# info (-DCMAKE_CXX_FLAGS=-g on top of Release), so it is never part of the
# normal build or ctest run: opt in with -DOI_ABI_CHECK=ON. Targets abi-check
# and abi-update are then available too. See abi/README.md.
option(OI_ABI_CHECK "Register the libabigail plugin ABI check (needs abidw/abidiff and -g)" OFF)
if(OI_ABI_CHECK)
    find_program(OI_ABIDW abidw)
    find_program(OI_ABIDIFF abidiff)
    if(OI_ABIDW AND OI_ABIDIFF)
        add_test(NAME abi_check
            COMMAND ${CMAKE_COMMAND} -E env ABIDW=${OI_ABIDW} ABIDIFF=${OI_ABIDIFF}
                    bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/abi-check.sh
                    --lib $<TARGET_FILE:OpenIndyCore>)
        set_tests_properties(abi_check PROPERTIES LABELS abi SKIP_RETURN_CODE 77)
        add_custom_target(abi-check
            COMMAND ${CMAKE_COMMAND} -E env ABIDW=${OI_ABIDW} ABIDIFF=${OI_ABIDIFF}
                    bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/abi-check.sh
                    --lib $<TARGET_FILE:OpenIndyCore>
            DEPENDS OpenIndyCore USES_TERMINAL)
        add_custom_target(abi-update
            COMMAND ${CMAKE_COMMAND} -E env ABIDW=${OI_ABIDW} ABIDIFF=${OI_ABIDIFF}
                    bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/abi-check.sh --update
                    --lib $<TARGET_FILE:OpenIndyCore>
            DEPENDS OpenIndyCore USES_TERMINAL)
    else()
        message(STATUS "OI_ABI_CHECK=ON but abidw/abidiff (libabigail) not found: ABI check skipped")
    endif()
endif()
