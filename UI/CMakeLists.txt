set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/PCB-UI/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/PCB-UI/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/PCB-UI/lib)

add_library(PCB-UI STATIC
        null.h
        snap_to_fixed_up.h
        look_at.h
        ortho.h
        frustum.h
        ViewerData.h
        ViewerData.cpp
        Viewer.h
        Viewer.cpp)

set_target_properties(PCB-UI PROPERTIES CXX_STANDARD 20)
target_link_libraries(PCB-UI INTERFACE Eigen3::Eigen)
target_link_libraries(PCB-UI PUBLIC PCB-Core imgui)

target_include_directories(PCB-UI PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)

if (MSVC)
    target_compile_options(PCB-UI
            PUBLIC
            "-openmp:experimental")
else ()
    target_compile_options(PCB-UI
            PUBLIC
            ${OpenMP_CXX_FLAGS})
endif ()