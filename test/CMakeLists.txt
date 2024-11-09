add_executable(test_cd test_cd.cpp)
add_executable(test_cp test_cp.cpp)

set_target_properties(test_cd PROPERTIES CXX_STANDARD 20)
target_link_libraries(test_cd PUBLIC PCB-UI)

set_target_properties(test_cp PROPERTIES CXX_STANDARD 20)
target_link_libraries(test_cp PUBLIC PCB-UI)

if (MSVC)
    target_compile_options(test_cd
            PUBLIC
            "-openmp:experimental")
else ()
    target_compile_options(test_cd
            PUBLIC
            ${OpenMP_CXX_FLAGS})
endif ()

if (MSVC)
    target_compile_options(test_cp
            PUBLIC
            "-openmp:experimental")
else ()
    target_compile_options(test_cp
            PUBLIC
            ${OpenMP_CXX_FLAGS})
endif ()

add_custom_command(TARGET test_cd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pcb_data/initial_normal.txt $<TARGET_FILE_DIR:test_cd>/initial_normal.txt
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pcb_data/initial_hard.txt $<TARGET_FILE_DIR:test_cd>/initial_hard.txt)

add_custom_command(TARGET test_cp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pcb_data/initial_normal.txt $<TARGET_FILE_DIR:test_cp>/initial_normal.txt
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pcb_data/initial_hard.txt $<TARGET_FILE_DIR:test_cp>/initial_hard.txt)
