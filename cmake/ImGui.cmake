include(FetchContent)

#FetchContent_Declare(
#        cmake-imgui
#        GIT_REPOSITORY https://github.com/Alan-Leo-Wong/cmake-imgui.git
#        GIT_TAG master
#)
#FetchContent_MakeAvailable(cmake-imgui)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG master
)
FetchContent_MakeAvailable(imgui)

#FetchContent_Declare(
#        glfw
#        GIT_REPOSITORY https://github.com/glfw/glfw.git
#        GIT_TAG master
#)
#FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3327050ca66ad34426a82c217c2d60ced61526b7
)

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_VULKAN_STATIC "Use the Vulkan loader statically linked into application" OFF)
FetchContent_MakeAvailable(glfw)

add_library(glfw::glfw ALIAS glfw)

set_target_properties(glfw PROPERTIES FOLDER ThirdParty)

# Warning config
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    target_compile_options(glfw PRIVATE
            "-Wno-missing-field-initializers"
            "-Wno-objc-multiple-method-names"
    )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(glfw PRIVATE
            "-Wno-missing-field-initializers"
            "-Wno-objc-multiple-method-names"
    )
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(glfw PRIVATE
            "-Wno-missing-field-initializers"
            "-Wno-sign-compare"
            "-Wno-unused-parameter"
    )
endif()

###### glad
#option(IGG_GLAD_DOWNLOAD "If set to ON the glad gl loader will be generated and downloaded, if OFF the included version (gl4.5/core) will be used" ON)
#if (IGG_GLAD_DOWNLOAD)
#    set(IGG_GLAD_GL_VERSION 4.5 CACHE STRING "The target gl version")
#    option(IGG_GLAD_GL_CORE "The target gl profile. ON = core profile, OFF = compatibility profile" ON)
#    if (IGG_GLAD_GL_CORE)
#        set(GLAD_GL_PROFILE core)
#    else ()
#        set(GLAD_GL_PROFILE compatibility)
#    endif ()
#else ()
#    unset(IGG_GLAD_GL_VERSION CACHE)
#    unset(IGG_GLAD_GL_CORE CACHE)
#endif ()
#
###############################
##            glad            #
###############################
#if (NOT IGG_GLAD_DOWNLOAD)
#    message(STATUS "Using included version of the glad loader sources (gl 4.5/core) ")
#    set(glad_SOURCE_DIR glad)
#else ()
#    if ("${glad_INSTALLED_VERSION}" STREQUAL "${IGG_GLAD_GL_VERSION}-${GLAD_GL_PROFILE}")
#        message(STATUS "Avoiding repeated download of glad gl ${IGG_GLAD_GL_VERSION}/${GLAD_GL_PROFILE}")
#        set(glad_SOURCE_DIR ${glad_LAST_SOURCE_DIR})
#    else ()
#        find_program(IGG_CURL NAMES curl curl.exe)
#        if (NOT IGG_CURL)
#            message(STATUS "Could not find curl, using included version of the glad loader sources (gl 4.5/core)")
#            set(glad_SOURCE_DIR glad)
#        else ()
#            execute_process(
#                    COMMAND ${IGG_CURL} -s -D - -X POST -d generator=c&api=egl%3Dnone&api=gl%3D${IGG_GLAD_GL_VERSION}&profile=gl%3D${GLAD_GL_PROFILE}&api=gles1%3Dnone&profile=gles1%3Dcommon&api=gles2%3Dnone&api=glsc2%3Dnone&api=glx%3Dnone&api=vulkan%3Dnone&api=wgl%3Dnone&options=LOADER https://gen.glad.sh/generate
#                    OUTPUT_VARIABLE out
#                    RESULT_VARIABLE res
#                    ERROR_VARIABLE err
#            )
#            if (NOT res EQUAL "0")
#                message(WARNING "${IGG_CURL} returned: " ${res})
#                if (err)
#                    message(WARNING "Error message: " ${err})
#                endif ()
#                message(STATUS "Using included version of the glad loader sources (gl 4.5/core)")
#                set(glad_SOURCE_DIR glad)
#            else ()
#                string(REGEX MATCH "[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn]: ([A-Za-z0-9_\\:/\\.]+)" location ${out})
#                set(location "${CMAKE_MATCH_1}")
#                if (NOT location OR location STREQUAL "/")
#                    message(WARNING "Could not extract location from http response, using included version of the glad loader sources (gl 4.5/core)")
#                    message(STATUS "Response: " ${out})
#                    set(glad_SOURCE_DIR glad)
#                else ()
#                    string(REGEX REPLACE "/$" "" location ${location})
#                    string(APPEND location "/glad.zip")
#                    if (NOT ${location} MATCHES "^http")
#                        string(PREPEND location "https://gen.glad.sh")
#                    endif ()
#                    message(STATUS "Downloading glad loader sources for gl${IGG_GLAD_GL_VERSION}/${GLAD_GL_PROFILE} from ${location}")
#                    FetchContent_Declare(
#                            glad
#                            URL ${location}
#                    )
#                    FetchContent_MakeAvailable(glad)
#                    set(glad_INSTALLED_VERSION ${IGG_GLAD_GL_VERSION}-${GLAD_GL_PROFILE} CACHE INTERNAL "")
#                    set(glad_LAST_SOURCE_DIR ${glad_SOURCE_DIR} CACHE INTERNAL "")
#                endif ()
#            endif ()
#        endif ()
#    endif ()
#endif ()
#
#add_library(
#        glad
#        ${glad_SOURCE_DIR}/src/gl.c
#        ${glad_SOURCE_DIR}/include/glad/gl.h
#        ${glad_SOURCE_DIR}/include/KHR/khrplatform.h
#)
#target_include_directories(glad PUBLIC ${glad_SOURCE_DIR}/include)

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/libigl/libigl-glad.git
        GIT_TAG        ead2d21fd1d9f566d8f9a9ce99ddf85829258c7a
)
FetchContent_MakeAvailable(glad)

file(GLOB imgui_SRC ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
add_library(
        imgui
        ${imgui_SRC}
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
)

#target_link_libraries(imgui PUBLIC glad)
#if (NOT EMSCRIPTEN)
#    target_link_libraries(imgui PUBLIC glfw)
#endif ()
target_include_directories(
        imgui
        PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(imgui PUBLIC
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS # to check for obsolete functions
)

if (NOT TARGET imgui)
    message(FATAL_ERROR "Creation of target 'ImGui::imgui' failed")
endif ()

if (NOT TARGET glfw)
    message(FATAL_ERROR "Creation of target 'glfw' failed")
endif ()

target_link_libraries(imgui PUBLIC glfw glad)