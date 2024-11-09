add_library(PCB-Core STATIC
        pcb_scene.h
        pcb_scene.cpp)

set_target_properties(PCB-Core PROPERTIES CXX_STANDARD 20)
target_link_libraries(PCB-Core PUBLIC PCB-BVH Eigen3::Eigen)

target_include_directories(PCB-Core PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)