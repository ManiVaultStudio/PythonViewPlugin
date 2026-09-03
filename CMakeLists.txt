cmake_minimum_required(VERSION 3.31)

project(PythonViewPlugin
    VERSION 0.1.0
    DESCRIPTION "A Python-backed view plugin for ManiVault Studio"
    LANGUAGES CXX
)

option(MV_UNITY_BUILD "Combine target source files into batches for faster compilation" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
find_package(ManiVault COMPONENTS Core PointData ClusterData ImageData CONFIG QUIET)
find_package(Python COMPONENTS Development Interpreter REQUIRED)

mv_project_defaults()

get_filename_component(PYTHON_VIEW_PYTHON_HOME "${Python_EXECUTABLE}" DIRECTORY)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PythonBuildConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/PythonBuildConfig.h"
    @ONLY
)

# Reuse the data-model bindings maintained by JupyterPlugin without building its
# notebook/xeus targets. Use that repository's dependency bootstrap, but fetch
# only pybind11 rather than the full Xeus dependency graph.
set(PYBIND11_FINDPYTHON ON)
include(extern/JupyterPlugin/cmake/get_cpm.cmake)
CPMAddPackage(
    NAME pybind
    GITHUB_REPOSITORY pybind/pybind11
    GIT_TAG v3.0.4
    EXCLUDE_FROM_ALL YES
    OPTIONS "PYBIND11_FINDPYTHON ON"
)

set(SOURCES
    src/PythonRuntime.h
    src/PythonRuntime.cpp
    src/PythonViewPlugin.h
    src/PythonViewPlugin.cpp
    extern/JupyterPlugin/src/JupyterPlugin/MVData.h
    extern/JupyterPlugin/src/JupyterPlugin/MVData.cpp
    extern/JupyterPlugin/src/JupyterPlugin/BindingUtils.h
    extern/JupyterPlugin/src/JupyterPlugin/BindingUtils.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} PluginInfo.json)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${ManiVault_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/JupyterPlugin/src/JupyterPlugin"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    UNITY_BUILD ${MV_UNITY_BUILD}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    ManiVault::Core
    ManiVault::PointData
    ManiVault::ClusterData
    ManiVault::ImageData
    pybind11::embed
    Python::Python
)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl util m)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS
)

install(FILES examples/basic_view.py
    DESTINATION examples/PythonViewPlugin
    COMPONENT PYTHON_VIEW_EXAMPLES
)

# Python is a load-time dependency of the plugin DLL. ManiVault explicitly
# preloads libraries from PluginDependencies/<plugin target> before loading a
# plugin, which also works when the executable is launched from a build tree.
if(WIN32)
    install(FILES "$<TARGET_FILE:Python::Python>"
        DESTINATION PluginDependencies/${PROJECT_NAME}
        COMPONENT PYTHON_RUNTIME
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

mv_handle_plugin_config(${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
        $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND
        $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
