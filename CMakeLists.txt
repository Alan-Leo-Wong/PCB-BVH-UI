cmake_minimum_required(VERSION 3.21)
project(PCB-BVH-UI VERSION 1.0)

###########################################
# CMake module path
list(PREPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(ImGui)
include(eigen)

find_package(OpenMP REQUIRED)

#add_subdirectory(Common)
add_subdirectory(PCB-BVH)
add_subdirectory(Core)
add_subdirectory(UI)

if (PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if (BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()