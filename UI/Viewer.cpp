//
// Created by Lei on 10/6/2024.
//

#include "Viewer.h"
#include "null.h"
#include "snap_to_fixed_up.h"
#include "look_at.h"
#include "ortho.h"
#include "frustum.h"
#include "cmake-build-release/_deps/eigen-src/Eigen/Core"

#include <bvh/v2/pcb_data.h>

#include <cstddef>
#include <random>
#include <chrono>
#include <omp.h>

#include <Eigen/Geometry>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace ui {

    Viewer::Viewer(int window_width, int window_height) {
        // Default colors
        background_color << 0.3f, 0.3f, 0.5f, 1.0f;

        // Default lights settings
        light_position << 0.0f, 0.3f, 0.0f;
        is_directional_light = false;
        is_shadow_mapping = false;
        shadow_width = 2056;
        shadow_height = 2056;

        lighting_factor = 1.0f; //on

        // Default trackball
        trackball_angle = Eigen::Quaternionf::Identity();
        rotation_type = RotationType::ROTATION_TYPE_TRACKBALL;
        set_rotation_type(RotationType::ROTATION_TYPE_TWO_AXIS_VALUATOR_FIXED_UP);

        // Camera parameters
        camera_base_zoom = 1.0f;
        camera_zoom = 1.0f;
        orthographic = false;
        camera_view_angle = 45.0;
        camera_dnear = 1.0;
        camera_dfar = 100.0;
        camera_base_translation << 0, 0, 0;
        camera_translation << 0, 0, 0;
        camera_eye << 0, 0, 5;
        camera_center << 0, 0, 0;
        camera_up << 0, 1, 0;

        depth_test = true;

        is_animating = false;
        animation_max_fps = 30.;

        viewport.setZero();

        init_context(window_width, window_height);

        setup_buffers();
        setup_shaders();
        if (is_shadow_mapping) initialize_shadow_pass();
    }

    Viewer::~Viewer() {
        if (window != nullptr) {
            // Cleanup
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(window);
            window = nullptr;  // 避免悬空指针
        }
        glDeleteVertexArrays(1, &viewer_data.seg_VAO);
        glDeleteVertexArrays(1, &viewer_data.arc_VAO);
        glDeleteVertexArrays(1, &viewer_data.dp_VAO);
        glDeleteVertexArrays(1, &viewer_data.db_VAO);

        glDeleteBuffers(1, &viewer_data.seg_VBO);
        glDeleteBuffers(1, &viewer_data.arc_VBO);
        glDeleteBuffers(1, &viewer_data.dp_VBO);
        glDeleteBuffers(1, &viewer_data.db_VBO);

        glDeleteBuffers(1, &viewer_data.seg_EBO);
        glDeleteBuffers(1, &viewer_data.arc_EBO);
        glDeleteBuffers(1, &viewer_data.db_EBO);

        glDeleteProgram(viewer_data.scene_shader_program);
        glDeleteProgram(viewer_data.dp_shader_program);
        glDeleteProgram(viewer_data.dp_line_shader_program);
        glDeleteProgram(viewer_data.db_shader_program);
        glfwTerminate();  // 如果需要，也可以在这里结束 GLFW
    }

    void Viewer::set_rotation_type(const RotationType &value) {
        using namespace Eigen;
        using namespace std;
        const RotationType old_rotation_type = rotation_type;
        rotation_type = value;
        if (rotation_type == RotationType::ROTATION_TYPE_TWO_AXIS_VALUATOR_FIXED_UP &&
            old_rotation_type != RotationType::ROTATION_TYPE_TWO_AXIS_VALUATOR_FIXED_UP) {
            snap_to_fixed_up(Quaternionf(trackball_angle), trackball_angle);
        }
    }

    ERROR_CODE
    Viewer::init_context(int window_width, int window_height) {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            std::cerr << "GLFW Failed\n";
            return ERROR_CODE::ERROR_UI_INIT_FAILURE;
        }

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
        // GL 3.0 + GLSL 130
        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        // Create window with graphics context
        window = glfwCreateWindow(window_width, window_height, "PCB Viewer", nullptr, nullptr);
        if (window == nullptr) return ERROR_CODE::ERROR_UI_INIT_FAILURE;
        glfwMakeContextCurrent(window);

        // Load OpenGL and its extensions
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            printf("Failed to load OpenGL and its extensions\n");
            return ERROR_CODE::ERROR_UI_INIT_FAILURE;
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSwapInterval(1); // Enable vsync

        // Set viewport
        int width_window, height_window;
        glfwGetWindowSize(window, &width_window, &height_window);
        double highdpiw = window_width / width_window;
        double highdpih = window_height / height_window;
        int w = window_width * highdpiw;
        int h = window_height * highdpih;
        viewport = Eigen::Vector4f(0, 0, w, h);

        // Register callbacks
        glfwSetKeyCallback(window, glfw_key_callback);
        glfwSetCursorPosCallback(window, glfw_mouse_move);
        glfwSetWindowSizeCallback(window, glfw_window_size);
        glfwSetMouseButtonCallback(window, glfw_mouse_press);
        glfwSetScrollCallback(window, glfw_mouse_scroll);
        glfwSetCharModsCallback(window, glfw_char_mods_callback);
        glfwSetDropCallback(window, glfw_drop_callback);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
        ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
        ImGui_ImplOpenGL3_Init(glsl_version);

        return ERROR_CODE::SUCCESS;
    }

    void Viewer::initialize_shadow_pass() {
        // attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_depth_fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, shadow_color_rbo);
        // clear buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // In the libigl viewer setup, each mesh has its own shader program. This is
        // kind of funny because they should all be the same, just different uniform
        // values.
        glViewport(0, 0, shadow_width, shadow_height);
        // Assumes light is directional
        assert(is_directional_light);
        Eigen::Vector3f camera_eye = light_position.normalized() * 5;
        Eigen::Vector3f camera_up = [&camera_eye]() {
            Eigen::Matrix<float, 3, 2> T;
            null(camera_eye.transpose().eval(), T);
            return T.col(0);
        }();
        Eigen::Vector3f camera_center = this->camera_center;
        // Same camera parameters except 2× field of view and reduced far plane
        float camera_view_angle = 2 * this->camera_view_angle;
        float camera_dnear = this->camera_dnear;
        float camera_dfar = this->camera_dfar;
        Eigen::Quaternionf trackball_angle = this->trackball_angle;
        float camera_zoom = this->camera_zoom;
        float camera_base_zoom = this->camera_base_zoom;
        Eigen::Vector3f camera_translation = this->camera_translation;
        Eigen::Vector3f camera_base_translation = this->camera_base_translation;
        camera_dfar = exp2(0.5 * (log2(camera_dnear) + log2(camera_dfar)));
        look_at(camera_eye, camera_center, camera_up, shadow_view);
        shadow_view = shadow_view
                      * (trackball_angle * Eigen::Scaling(camera_zoom * camera_base_zoom)
                         * Eigen::Translation3f(camera_translation + camera_base_translation)).matrix();

        float length = (camera_eye - camera_center).norm();
        float h = tan(camera_view_angle / 360.0 * bvh::v2::M_PI) * (length);
        ortho(-h * shadow_width / shadow_height, h * shadow_width / shadow_height, -h, h, camera_dnear,
              camera_dfar, shadow_proj);
    }

    void Viewer::setup_shaders() {
        const char *scene_vertex_shader_source = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec3 aColor;

            uniform mat4 MVP;

            out vec3 vertexColor;
            void main() {
                gl_Position = MVP * vec4(aPos, 0.0, 1.0);
                vertexColor = aColor;
            }
        )";

        const char *scene_fragment_shader_source = R"(
            #version 330 core
            in vec3 vertexColor;
            out vec4 FragColor;
            void main() {
                FragColor = vec4(vertexColor, 1.0f);
                // FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // orange
            }
        )";

        const char *dp_vertex_shader_source = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;

            uniform mat4 MVP;

            void main() {
                gl_Position = MVP * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char *dp_fragment_shader_source = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec3 color;  // 动态传递颜色
            void main() {
                // vec2 coord = gl_PointCoord - vec2(0.5, 0.5);  // from [0,1] to [-0.5,0.5]
                // if(length(coord) > 0.5)                  // outside of circle radius?
                   // discard;

                FragColor = vec4(color, 1.0);
            }
        )";

        const char *dp_line_vertex_shader_source = R"(
            #version 330 core

            layout (location = 0) in vec2 inPos;

            flat out vec3 startPos;
            out vec3 vertPos;

            uniform mat4 MVP;

            void main()
            {
                vec4 pos    = MVP * vec4(inPos, 0.0, 1.0);
                gl_Position = pos;
                vertPos     = pos.xyz / pos.w;
                startPos    = vertPos;
            }
        )";

        const char *dp_line_fragment_shader_source = R"(
            #version 330 core
            flat in vec3 startPos;
            in vec3 vertPos;

            out vec4 fragColor;

            uniform vec2  u_resolution;
            uniform float u_dashSize;
            uniform float u_gapSize;

            void main()
            {
                vec2  dir  = (vertPos.xy-startPos.xy) * u_resolution/2.0;
                float dist = length(dir);

                if (fract(dist / (u_dashSize + u_gapSize)) > u_dashSize/(u_dashSize + u_gapSize))
                    discard;
                fragColor = vec4(1.0);
            }
        )";

        GLuint scene_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint dp_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint dp_line_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        GLuint db_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(scene_vertex_shader, 1, &scene_vertex_shader_source, nullptr);
        glShaderSource(dp_vertex_shader, 1, &dp_vertex_shader_source, nullptr);
        glShaderSource(dp_line_vertex_shader, 1, &dp_line_vertex_shader_source, nullptr);
        glShaderSource(db_vertex_shader, 1, &dp_vertex_shader_source, nullptr);

        glCompileShader(scene_vertex_shader);
        glCompileShader(dp_vertex_shader);
        glCompileShader(dp_line_vertex_shader);
        glCompileShader(db_vertex_shader);

        GLuint scene_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint dp_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint dp_line_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint db_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(scene_fragment_shader, 1, &scene_fragment_shader_source, nullptr);
        glShaderSource(dp_fragment_shader, 1, &dp_fragment_shader_source, nullptr);
        glShaderSource(dp_line_fragment_shader, 1, &dp_line_fragment_shader_source, nullptr);
        glShaderSource(db_fragment_shader, 1, &dp_fragment_shader_source, nullptr);

        glCompileShader(scene_fragment_shader);
        glCompileShader(dp_fragment_shader);
        glCompileShader(dp_line_fragment_shader);
        glCompileShader(db_fragment_shader);

        viewer_data.scene_shader_program = glCreateProgram();
        viewer_data.dp_shader_program = glCreateProgram();
        viewer_data.dp_line_shader_program = glCreateProgram();
        viewer_data.db_shader_program = glCreateProgram();

        glAttachShader(viewer_data.scene_shader_program, scene_vertex_shader);
        glAttachShader(viewer_data.dp_shader_program, dp_vertex_shader);
        glAttachShader(viewer_data.dp_line_shader_program, dp_line_vertex_shader);
        glAttachShader(viewer_data.db_shader_program, db_vertex_shader);

        glAttachShader(viewer_data.scene_shader_program, scene_fragment_shader);
        glAttachShader(viewer_data.dp_shader_program, dp_fragment_shader);
        glAttachShader(viewer_data.dp_line_shader_program, dp_line_fragment_shader);
        glAttachShader(viewer_data.db_shader_program, db_fragment_shader);

        glLinkProgram(viewer_data.scene_shader_program);
        glLinkProgram(viewer_data.dp_shader_program);
        glLinkProgram(viewer_data.dp_line_shader_program);
        glLinkProgram(viewer_data.db_shader_program);

        glDeleteShader(scene_vertex_shader);
        glDeleteShader(dp_vertex_shader);
        glDeleteShader(dp_line_vertex_shader);
        glDeleteShader(db_vertex_shader);

        glDeleteShader(scene_fragment_shader);
        glDeleteShader(dp_fragment_shader);
        glDeleteShader(dp_line_fragment_shader);
        glDeleteShader(db_fragment_shader);
    }

    void Viewer::setup_buffers() {
        glGenVertexArrays(1, &viewer_data.seg_VAO);
        glGenVertexArrays(1, &viewer_data.arc_VAO);
        glGenVertexArrays(1, &viewer_data.dp_VAO);
        glGenVertexArrays(1, &viewer_data.dp_line_VAO);
        glGenVertexArrays(1, &viewer_data.db_VAO);

        glGenBuffers(1, &viewer_data.seg_VBO);
        glGenBuffers(1, &viewer_data.arc_VBO);
        glGenBuffers(1, &viewer_data.dp_VBO);
        glGenBuffers(1, &viewer_data.dp_line_VBO);
        glGenBuffers(1, &viewer_data.db_VBO);

        glGenBuffers(1, &viewer_data.seg_EBO);
        glGenBuffers(1, &viewer_data.arc_EBO);
        glGenBuffers(1, &viewer_data.db_EBO);
    }

    void Viewer::set_seg_data(const PCBSeg &seg, const Eigen::Vector3f &color) {
        Eigen::Vector2f pos_0(seg.p0[0], seg.p0[1]);
        viewer_data.seg_vertices.emplace_back(pos_0, color);

        Eigen::Vector2f pos_1(seg.p1[0], seg.p1[1]);
        viewer_data.seg_vertices.emplace_back(pos_1, color);

        viewer_data.seg_indices.emplace_back(viewer_data.seg_beg_indice);
        viewer_data.seg_indices.emplace_back(viewer_data.seg_beg_indice + 1);
        viewer_data.seg_beg_indice += 2;
    }

    void Viewer::set_arc_data(const PCBArc &arc, const Eigen::Vector3f &color) {
        glLineWidth(arc_line_width);

        int num_segments = 100;
        double theta_step = (arc.arc_data.theta_1 - arc.arc_data.theta_0) / num_segments;

        for (int i = 0; i <= num_segments; ++i) {
            double theta = arc.arc_data.theta_0 + i * theta_step;
            double x = arc.arc_data.center[0] + arc.arc_data.radius * std::cos(theta);
            double y = arc.arc_data.center[1] + arc.arc_data.radius * std::sin(theta);
            Eigen::Vector2f pos(x, y);
            viewer_data.arc_vertices.emplace_back(pos, color);

            if (i >= 1) {
                viewer_data.arc_indices.push_back(viewer_data.arc_beg_indice - 1);
                viewer_data.arc_indices.push_back(viewer_data.arc_beg_indice);
            }
            ++viewer_data.arc_beg_indice;
        }
    }

    void Viewer::set_bbox_data(const BBox2 &bbox) {
        glLineWidth(seg_line_width);
    }

    void Viewer::set_scene_data(Eigen::Matrix4f &MVP, const float scale_factor) {
        const auto &pcb_data = pcb_scene->get_data();
        const auto &pcb_box = pcb_scene->get_bounding_box();

        using namespace std;
        using namespace Eigen;

        float scene_min_x = pcb_box.min[0];
        float scene_min_y = pcb_box.min[1];
        float scene_max_x = pcb_box.max[0];
        float scene_max_y = pcb_box.max[1];

        float scene_width = scene_max_x - scene_min_x;
        float scene_height = scene_max_y - scene_min_y;
        float scene_center_x = (scene_min_x + scene_max_x) / 2.0f;
        float scene_center_y = (scene_min_y + scene_max_y) / 2.0f;

        Matrix4f projection = Matrix4f::Identity();

        Matrix4f view = Matrix4f::Identity();

        Matrix4f model = Matrix4f::Identity();
        Matrix4f scale_model = Matrix4f::Identity();
        scale_model(0, 0) = 2.0f * scale_factor / scene_width;
        scale_model(1, 1) = 2.0f * scale_factor / scene_height;
        scale_model(2, 2) = -1.0f;
        Matrix4f rotate_model = Matrix4f::Identity();
        Matrix4f trans_model = Matrix4f::Identity();
        trans_model(0, 3) = -scene_center_x * (2.0f * scale_factor / scene_width);
        trans_model(1, 3) = -scene_center_y * (2.0f * scale_factor / scene_height);
        model = trans_model * rotate_model * scale_model;

        MVP = projection * view * model;

        const Vector3f seg_color(0.0f, 0.5f, 0.2f);
        const Vector3f arc_color(1.0f, 0.5f, 0.2f);
        for (const auto &pri: pcb_data) {
            if (pri->is_arc) {
                auto arc = dynamic_cast<PCBArc *>(pri.get());
                set_arc_data(*arc, arc_color);
            } else {
                auto seg = dynamic_cast<PCBSeg *>(pri.get());
                set_seg_data(*seg, seg_color);
            }
        }

        // VAO
        glBindVertexArray(viewer_data.seg_VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.seg_VBO);
        glBufferData(GL_ARRAY_BUFFER, viewer_data.seg_vertices.size() * sizeof(Vertex), viewer_data.seg_vertices.data(),
                     GL_STATIC_DRAW);
        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, viewer_data.seg_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, viewer_data.seg_indices.size() * sizeof(GLuint),
                     viewer_data.seg_indices.data(), GL_STATIC_DRAW);
        // Link point attributes
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, color));
        glEnableVertexAttribArray(1);

        // VAO
        glBindVertexArray(viewer_data.arc_VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.arc_VBO);
        glBufferData(GL_ARRAY_BUFFER, viewer_data.arc_vertices.size() * sizeof(Vertex),
                     viewer_data.arc_vertices.data(), GL_STATIC_DRAW);
        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, viewer_data.arc_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, viewer_data.arc_indices.size() * sizeof(GLuint),
                     viewer_data.arc_indices.data(), GL_STATIC_DRAW);
        // Link point attributes
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, color));
        glEnableVertexAttribArray(1);

        {
            view = Eigen::Matrix4f::Identity();
            proj = Eigen::Matrix4f::Identity();
            norm = Eigen::Matrix4f::Identity();

            float width = viewport(2);
            float height = viewport(3);

            // Set view
            look_at(camera_eye, camera_center, camera_up, view);
            view = view
                   * (trackball_angle * Eigen::Scaling(camera_zoom * camera_base_zoom)
                      * Eigen::Translation3f(camera_translation + camera_base_translation)).matrix();

            norm = view.inverse().transpose();

            // Set projection
            if (orthographic) {
                float length = (camera_eye - camera_center).norm();
                float h = tan(camera_view_angle / 360.0 * bvh::v2::M_PI) * (length);
                ortho(-h * width / height, h * width / height, -h, h, camera_dnear, camera_dfar, proj);
            } else {
                float fH = tan(camera_view_angle / 360.0 * bvh::v2::M_PI) * camera_dnear;
                float fW = fH * (double) width / (double) height;
                frustum(-fW, fW, -fH, fH, camera_dnear, camera_dfar, proj);
            }

            // Send transformations to the GPU
            GLint viewi = glGetUniformLocation(viewer_data.shader_mesh, "view");
            GLint proji = glGetUniformLocation(viewer_data.shader_mesh, "proj");
            GLint normi = glGetUniformLocation(viewer_data.shader_mesh, "normal_matrix");
            glUniformMatrix4fv(viewi, 1, GL_FALSE, view.data());
            glUniformMatrix4fv(proji, 1, GL_FALSE, proj.data());
            glUniformMatrix4fv(normi, 1, GL_FALSE, norm.data());

            // Light parameters
            GLint specular_exponenti = glGetUniformLocation(viewer_data.shader_mesh, "specular_exponent");
            GLint light_position_eyei = glGetUniformLocation(viewer_data.shader_mesh, "light_position_eye");
            GLint lighting_factori = glGetUniformLocation(viewer_data.shader_mesh, "lighting_factor");
            GLint fixed_colori = glGetUniformLocation(viewer_data.shader_mesh, "fixed_color");
            GLint texture_factori = glGetUniformLocation(viewer_data.shader_mesh, "texture_factor");
            GLint matcap_factori = glGetUniformLocation(viewer_data.shader_mesh, "matcap_factor");
            GLint double_sidedi = glGetUniformLocation(viewer_data.shader_mesh, "double_sided");

            const bool eff_is_directional_light = is_directional_light || is_shadow_mapping;
            glUniform1f(specular_exponenti, viewer_data.shininess);
            if (eff_is_directional_light) {
                Eigen::Vector3f light_direction = light_position.normalized();
                glUniform3fv(light_position_eyei, 1, light_direction.data());
            } else {
                glUniform3fv(light_position_eyei, 1, light_position.data());
            }
            if (is_shadow_mapping) {
                glUniformMatrix4fv(glGetUniformLocation(viewer_data.shader_mesh, "shadow_view"), 1, GL_FALSE,
                                   shadow_view.data());
                glUniformMatrix4fv(glGetUniformLocation(viewer_data.shader_mesh, "shadow_proj"), 1, GL_FALSE,
                                   shadow_proj.data());
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
                {
                    glUniform1i(glGetUniformLocation(viewer_data.shader_mesh, "shadow_tex"), 1);
                }
            }
            glUniform1f(lighting_factori, lighting_factor); // enables lighting
            glUniform4f(fixed_colori, 0.0, 0.0, 0.0, 0.0);

            glUniform1i(glGetUniformLocation(viewer_data.shader_mesh, "is_directional_light"),
                        eff_is_directional_light);
            glUniform1i(glGetUniformLocation(viewer_data.shader_mesh, "is_shadow_mapping"), is_shadow_mapping);
            glUniform1i(glGetUniformLocation(viewer_data.shader_mesh, "shadow_pass"), false);
        }
    }

    void Viewer::set_dp_data(Eigen::Matrix4f &MVP, const int num_pts, const float scale_factor) {
        using namespace std;
        using namespace Eigen;

        const auto &pcb_box = pcb_scene->get_bounding_box();

        float scene_min_x = pcb_box.min[0];
        float scene_min_y = pcb_box.min[1];
        float scene_max_x = pcb_box.max[0];
        float scene_max_y = pcb_box.max[1];

        float scene_width = scene_max_x - scene_min_x;
        float scene_height = scene_max_y - scene_min_y;
        float scene_center_x = (scene_min_x + scene_max_x) / 2.0f;
        float scene_center_y = (scene_min_y + scene_max_y) / 2.0f;
        float bbox_len = Vector2f(scene_width, scene_height).norm();

        Matrix4f projection = Matrix4f::Identity();

        Matrix4f view = Matrix4f::Identity();

        Matrix4f model = Matrix4f::Identity();
        Matrix4f scale_model = Matrix4f::Identity();
        scale_model(0, 0) = 2.0f * scale_factor / scene_width;
        scale_model(1, 1) = 2.0f * scale_factor / scene_height;
        scale_model(2, 2) = -1.0f;
        Matrix4f rotate_model = Matrix4f::Identity();
        Matrix4f trans_model = Matrix4f::Identity();
        trans_model(0, 3) = -scene_center_x * (2.0f * scale_factor / scene_width);
        trans_model(1, 3) = -scene_center_y * (2.0f * scale_factor / scene_height);
        model = trans_model * rotate_model * scale_model;

        MVP = projection * view * model;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis_position_x(scene_min_x, scene_max_x);
        std::uniform_real_distribution<> dis_position_y(scene_min_y, scene_max_y);
        std::uniform_real_distribution<> dis_velocity(-1e-3, 1e-3);
        viewer_data.dynamic_points.resize(num_pts);
#pragma omp parallel for
        for (int i = 0; i < num_pts; ++i) {
            Vector2f pos(dis_position_x(gen), dis_position_y(gen));
            Vector2f velocity = Eigen::Vector2f(dis_velocity(gen), dis_velocity(gen)) * bbox_len;
//            viewer_data.dynamic_points.emplace_back();
            viewer_data.dynamic_points[i].position = pos;
            viewer_data.dynamic_points[i].velocity = velocity;
        }

        glPointSize(point_width);
        // VAO
        glBindVertexArray(viewer_data.dp_VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.dp_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2f),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        // Link point attributes
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2f),
                              (void *) 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // VAO
        glBindVertexArray(viewer_data.dp_line_VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.dp_line_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2f) * 2,
                     nullptr,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2f),
                              (void *) 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void Viewer::set_db_data(Eigen::Matrix4f &MVP, const int num_db, const float scale_factor) {
        using namespace std;
        using namespace Eigen;

        const auto &pcb_box = pcb_scene->get_bounding_box();

        float scene_min_x = pcb_box.min[0];
        float scene_min_y = pcb_box.min[1];
        float scene_max_x = pcb_box.max[0];
        float scene_max_y = pcb_box.max[1];

        float scene_width = scene_max_x - scene_min_x;
        float scene_height = scene_max_y - scene_min_y;
        float scene_center_x = (scene_min_x + scene_max_x) / 2.0f;
        float scene_center_y = (scene_min_y + scene_max_y) / 2.0f;
        float bbox_len = Vector2f(scene_width, scene_height).norm();

        Matrix4f projection = Matrix4f::Identity();

        Matrix4f view = Matrix4f::Identity();

        Matrix4f model = Matrix4f::Identity();
        Matrix4f scale_model = Matrix4f::Identity();
        scale_model(0, 0) = 2.0f * scale_factor / scene_width;
        scale_model(1, 1) = 2.0f * scale_factor / scene_height;
        scale_model(2, 2) = -1.0f;
        Matrix4f rotate_model = Matrix4f::Identity();
        Matrix4f trans_model = Matrix4f::Identity();
        trans_model(0, 3) = -scene_center_x * (2.0f * scale_factor / scene_width);
        trans_model(1, 3) = -scene_center_y * (2.0f * scale_factor / scene_height);
        model = trans_model * rotate_model * scale_model;

        MVP = projection * view * model;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> widthDist(scene_width * 0.1f, scene_width * 0.3f); // 盒子的宽度范围（10%到30%场景宽度）
        std::uniform_real_distribution<> heightDist(scene_height * 0.1f, scene_height * 0.3f); // 盒子的高度范围（10%到30%场景高度）
        std::uniform_real_distribution<> xDist(scene_min_x, scene_max_x);
        std::uniform_real_distribution<> yDist(scene_min_y, scene_max_y);
        std::uniform_real_distribution<> dis_velocity(-1e-3, 1e-3); // 速度范围
        viewer_data.dynamic_bbox.resize(num_db);

#pragma omp parallel for
        for (int i = 0; i < num_db; ++i) {
            float width = widthDist(gen);
            float height = heightDist(gen);

            float x = xDist(gen);
            float y = yDist(gen);

            if (x + width > scene_max_x) {
                x = scene_max_x - width;
            }
            if (y + height > scene_max_y) {
                y = scene_max_y - height;
            }

            Vector2f velocity = Vector2f(dis_velocity(gen), dis_velocity(gen)) * bbox_len;
//            viewer_data.dynamic_bbox.emplace_back();
            viewer_data.dynamic_bbox[i].position = {
                    Vector2f(x, y),
                    Vector2f(x + width, y),
                    Vector2f(x + width, y + height),
                    Vector2f(x, y + height)
            };
            viewer_data.dynamic_bbox[i].velocity = velocity;
        }

        glLineWidth(seg_line_width);
        // VAO
        glBindVertexArray(viewer_data.db_VAO);
        // VBO
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.db_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2f) * 4,
                     nullptr,
                     GL_DYNAMIC_DRAW);
        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, viewer_data.db_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, viewer_data.db_indices.size() * sizeof(GLuint),
                     viewer_data.db_indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2f),
                              (void *) 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void Viewer::update_dp(int num_dp) {
        using namespace Eigen;
        const auto &pcb_box = pcb_scene->get_bounding_box();
        float scene_min_x = pcb_box.min[0];
        float scene_min_y = pcb_box.min[1];
        float scene_max_x = pcb_box.max[0];
        float scene_max_y = pcb_box.max[1];

        if (viewer_data.dynamic_points.size() < num_dp) {
            float scene_width = scene_max_x - scene_min_x;
            float scene_height = scene_max_y - scene_min_y;
            float bbox_len = Vector2f(scene_width, scene_height).norm();

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis_position_x(scene_min_x, scene_max_x);
            std::uniform_real_distribution<> dis_position_y(scene_min_y, scene_max_y);
            std::uniform_real_distribution<> dis_velocity(-1e-3, 1e-3);
            int num_delta = num_dp - viewer_data.dynamic_points.size();
            for (int i = 0; i < num_delta; ++i) {
                Vector2f pos(dis_position_x(gen), dis_position_y(gen));
                Vector2f velocity = Eigen::Vector2f(dis_velocity(gen), dis_velocity(gen)) * bbox_len;
                viewer_data.dynamic_points.emplace_back();
                viewer_data.dynamic_points.back().position = pos;
                viewer_data.dynamic_points.back().velocity = velocity;
            }
        } else if (viewer_data.dynamic_points.size() > num_dp) {
            viewer_data.dynamic_points.resize(num_dp);
        }

#pragma omp parallel for
        for (int i = 0; i < viewer_data.dynamic_points.size(); ++i) {
            auto &point = viewer_data.dynamic_points[i];
            if (point.position.x() < scene_min_x || point.position.x() > scene_max_x) {
                point.velocity.x() = -point.velocity.x();
            }
            if (point.position.y() < scene_min_y || point.position.y() > scene_max_y) {
                point.velocity.y() = -point.velocity.y();
            }

            point.position += point.velocity;
            Vec2 _p = {(double) point.position.x(), (double) point.position.y()};
            double dis;
            Vec2 closest;
            pcb_scene->get_closest(_p, dis, closest);
            point.closest_point = Eigen::Vector2f(closest[0], closest[1]);
        }

        for (const auto &point: viewer_data.dynamic_points) {
            glUseProgram(viewer_data.dp_shader_program);
            GLuint dp_mvp_loc = glGetUniformLocation(viewer_data.dp_shader_program, "MVP");
            glUniformMatrix4fv(dp_mvp_loc, 1, GL_FALSE, scene_mvp.data());
            glUniform3f(glGetUniformLocation(viewer_data.dp_shader_program, "color"), 0.0f, 0.0f, 1.0f);  // 蓝色
            draw_dp(point.closest_point);

            glUniformMatrix4fv(dp_mvp_loc, 1, GL_FALSE, dp_mvp.data());
            glUniform3f(glGetUniformLocation(viewer_data.dp_shader_program, "color"), 1.0f, 0.0f, 0.0f);  // 红色
            draw_dp(point.position);

            glUseProgram(viewer_data.dp_line_shader_program);
            glBindVertexArray(viewer_data.dp_line_VAO);
            GLint loc_mvp = glGetUniformLocation(viewer_data.dp_line_shader_program, "MVP");
            glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, scene_mvp.data());
            glBindBuffer(GL_ARRAY_BUFFER, viewer_data.dp_line_VBO);
            std::array<Eigen::Vector2f, 2> line_data = {
                    point.position,
                    point.closest_point
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line_data), line_data.data());
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
        }
    }

    void Viewer::update_db(int num_db, bool &scene_collision) {
        using namespace Eigen;
        using PCBData2 = bvh::v2::PCBData<double, 2>;
        using BBox2 = bvh::v2::BBox<double, 2>;
        const auto &pcb_box = pcb_scene->get_bounding_box();
        float scene_min_x = pcb_box.min[0];
        float scene_min_y = pcb_box.min[1];
        float scene_max_x = pcb_box.max[0];
        float scene_max_y = pcb_box.max[1];

        if (viewer_data.dynamic_bbox.size() < num_db) {
            float scene_width = scene_max_x - scene_min_x;
            float scene_height = scene_max_y - scene_min_y;
            float bbox_len = Vector2f(scene_width, scene_height).norm();

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> widthDist(scene_width * 0.1f, scene_width * 0.3f); // 盒子的宽度范围（10%到30%场景宽度）
            std::uniform_real_distribution<> heightDist(scene_height * 0.1f,
                                                        scene_height * 0.3f); // 盒子的高度范围（10%到30%场景高度）
            std::uniform_real_distribution<> xDist(scene_min_x, scene_max_x);
            std::uniform_real_distribution<> yDist(scene_min_y, scene_max_y);
            std::uniform_real_distribution<> dis_velocity(-1e-3, 1e-3); // 速度范围
            int num_delta = num_db - viewer_data.dynamic_bbox.size();
            for (int i = 0; i < num_delta; ++i) {
                float width = widthDist(gen);
                float height = heightDist(gen);

                float x = xDist(gen);
                float y = yDist(gen);

                if (x + width > scene_max_x) {
                    x = scene_max_x - width;
                }
                if (y + height > scene_max_y) {
                    y = scene_max_y - height;
                }

                Vector2f velocity = Vector2f(dis_velocity(gen), dis_velocity(gen)) * bbox_len;
                viewer_data.dynamic_bbox.emplace_back();
                viewer_data.dynamic_bbox.back().position = {
                        Vector2f(x, y),
                        Vector2f(x + width, y),
                        Vector2f(x + width, y + height),
                        Vector2f(x, y + height)
                };
                viewer_data.dynamic_bbox.back().velocity = velocity;
            }
        } else if (viewer_data.dynamic_bbox.size() > num_db) {
            viewer_data.dynamic_bbox.resize(num_db);
        }

#pragma omp parallel for
        for (int i = 0; i < viewer_data.dynamic_bbox.size(); ++i) {
            auto &bbox = viewer_data.dynamic_bbox[i];
            if (bbox.position[0].x() < scene_min_x || bbox.position[2].x() > scene_max_x) {
                bbox.velocity.x() = -bbox.velocity.x();
            }
            if (bbox.position[0].y() < scene_min_y || bbox.position[2].y() > scene_max_y) {
                bbox.velocity.y() = -bbox.velocity.y();
            }

            std::vector<PCBData2 *> inter_pris;
            for (int i = 0; i < 4; ++i)
                bbox.position[i] += bbox.velocity;
            BBox2 _bbox;
            _bbox.min = {bbox.position[0].x(), bbox.position[0].y()};
            _bbox.max = {bbox.position[2].x(), bbox.position[2].y()};
            pcb_scene->collision_detection(_bbox, inter_pris);
            bbox.is_collision = (inter_pris.size() > 0);
            if (bbox.is_collision && !scene_collision) {
#pragma omp critical
                scene_collision = true;
            }
            /*if (!scene_collision && (inter_pris.size() > 0))
                scene_collision = true;*/
        }
        for (const auto &bbox: viewer_data.dynamic_bbox) {
            glUseProgram(viewer_data.db_shader_program);
            GLuint db_mvp_loc = glGetUniformLocation(viewer_data.db_shader_program, "MVP");
            glUniformMatrix4fv(db_mvp_loc, 1, GL_FALSE, db_mvp.data());
            glUniform3f(glGetUniformLocation(viewer_data.db_shader_program, "color"), 0.5f, 0.2f, 0.5f);  // 蓝色

            draw_db(bbox.position);
        }
    }

    void Viewer::draw_dp(const Eigen::Vector2f &p) {
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.dp_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Eigen::Vector2f),
                        p.data());

        glBindVertexArray(viewer_data.dp_VAO);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

    void Viewer::draw_db(const std::array<Eigen::Vector2f, 4> &bbox_pos) {
        glBindBuffer(GL_ARRAY_BUFFER, viewer_data.db_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Eigen::Vector2f) * 4,
                        bbox_pos.data());

        glBindVertexArray(viewer_data.db_VAO);
        glDrawElements(GL_LINE_LOOP, viewer_data.db_indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Viewer::run_cp() {
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        float elapsedTime = 0.0f;
        int frameCount = 0;
        float fps = 0.0f;
        float fontSize = 2.0f;
        auto startTime = std::chrono::high_resolution_clock::now();

        int num_dp = 10;
        set_dp_data(dp_mvp, num_dp);
        set_scene_data(scene_mvp);

        GLint loc_res = glGetUniformLocation(viewer_data.dp_line_shader_program, "u_resolution");
        GLint loc_dash = glGetUniformLocation(viewer_data.dp_line_shader_program, "u_dashSize");
        GLint loc_gap = glGetUniformLocation(viewer_data.dp_line_shader_program, "u_gapSize");
        int vpSize[2]{0, 0};
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            if (w != vpSize[0] || h != vpSize[1]) {
                glUseProgram(viewer_data.dp_line_shader_program);
                vpSize[0] = w;
                vpSize[1] = h;
                glUniform2f(loc_res, (float) 20.0f, (float) 20.0f);

                glUniform1f(loc_dash, 10.0f);
                glUniform1f(loc_gap, 10.0f);
            }

            // Calculate FPS
            {
                auto currentTime = std::chrono::high_resolution_clock::now();
                elapsedTime += std::chrono::duration<float>(currentTime - startTime).count();
                startTime = currentTime;
                frameCount++;
                if (elapsedTime >= 1.0f) {
                    fps = frameCount;
                    frameCount = 0;
                    elapsedTime = 0;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("FPS Display", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            // FPS
            {
                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::Text("FPS: %.1f", fps);
                ImGui::GetFont()->Scale = fontSize;
            }

            {
                ImGui::SliderInt("Point Count", &num_dp, 1, 1000);

                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red: ");
                ImGui::SameLine();
                ImGui::Text("Query");

                ImGui::TextColored(ImVec4(0.2f, 0.3f, 0.8f, 1.0f), "Blue: ");
                ImGui::SameLine();
                ImGui::Text("Closest");
            }
            ImGui::End();

            // Clear the window
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                         clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(viewer_data.scene_shader_program);

            GLuint scene_mvp_loc = glGetUniformLocation(viewer_data.scene_shader_program, "MVP");
            glUniformMatrix4fv(scene_mvp_loc, 1, GL_FALSE, scene_mvp.data());

            glBindVertexArray(viewer_data.seg_VAO);
            glDrawElements(GL_LINES, viewer_data.seg_indices.size(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(viewer_data.arc_VAO);
            glDrawElements(GL_LINES, viewer_data.arc_indices.size(), GL_UNSIGNED_INT, 0);

            update_dp(num_dp);

            // Rendering ImGui
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }

    void Viewer::run_cd() {
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        float elapsedTime = 0.0f;
        int frameCount = 0;
        float fps = 0.0f;
        float fontSize = 2.0f;
        auto startTime = std::chrono::high_resolution_clock::now();

        int num_db = 1;
        set_db_data(db_mvp, num_db);
        set_scene_data(scene_mvp);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            // Calculate FPS
            {
                auto currentTime = std::chrono::high_resolution_clock::now();
                elapsedTime += std::chrono::duration<float>(currentTime - startTime).count();
                startTime = currentTime;
                frameCount++;
                if (elapsedTime >= 1.0f) {
                    fps = frameCount;
                    frameCount = 0;
                    elapsedTime = 0;
                }
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("FPS Display", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            // FPS
            {
                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::Text("FPS: %.1f", fps);
                ImGui::GetFont()->Scale = fontSize;
            }

            {
                ImGui::SliderInt("BBox Count", &num_db, 1, 1000);
            }

            // Clear the window
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                         clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(viewer_data.scene_shader_program);

            GLuint scene_mvp_loc = glGetUniformLocation(viewer_data.scene_shader_program, "MVP");
            glUniformMatrix4fv(scene_mvp_loc, 1, GL_FALSE, scene_mvp.data());

            glBindVertexArray(viewer_data.seg_VAO);
            glDrawElements(GL_LINES, viewer_data.seg_indices.size(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(viewer_data.arc_VAO);
            glDrawElements(GL_LINES, viewer_data.arc_indices.size(), GL_UNSIGNED_INT, 0);

            bool scene_collision = false;
            update_db(num_db, scene_collision);

            {
                ImGui::Text("AABB Collision Detection:");
                if (scene_collision)
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Colliding");
                else
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Not Colliding");
            }
            ImGui::End();

            // Rendering ImGui
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }
}