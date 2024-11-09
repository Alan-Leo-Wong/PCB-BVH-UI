//
// Created by Lei on 10/6/2024.
//

#ifndef PCB_OFFSET_SCENE_H
#define PCB_OFFSET_SCENE_H

#include "ViewerData.h"

#include <Core/pcb_scene.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Eigen/Dense>

namespace ui {

    class Viewer {
        using Scalar = double;
        using index_t = uint64_t;
        using Vec2 = bvh::v2::Vec<Scalar, 2>;
        using BBox2 = bvh::v2::BBox<Scalar, 2>;
        using PCBSeg = typename bvh::v2::PCBSeg<Scalar, 2>;
        using PCBArc = typename bvh::v2::PCBArc<Scalar, 2>;
        using PCBScene = core::PCBScene;

    private:
        /// Type of user interface for changing the view rotation based on the mouse
        /// draggin.
        enum class RotationType {
            /// Typical trackball rotation (like Meshlab)
            ROTATION_TYPE_TRACKBALL = 0,
            /// Fixed up rotation (like Blender, Maya, etc.)
            ROTATION_TYPE_TWO_AXIS_VALUATOR_FIXED_UP = 1,
            /// No rotation suitable for 2D
            ROTATION_TYPE_NO_ROTATION = 2,
            /// Total number of rotation types
            NUM_ROTATION_TYPES = 3
        };

        /// Set the current rotation type
        /// @param[in] value  the new rotation type
        void set_rotation_type(const RotationType &value);

    private:
        // UI Enumerations
        enum class MouseButton {
            Left, Middle, Right
        };
        enum class MouseMode {
            None, Rotation, Zoom, Pan, Translation
        } mouse_mode;

        GLFWwindow *window = nullptr;

    private:
        ViewerData viewer_data;

        /// Viewport size
        Eigen::Vector4f viewport;

        /// Background color as RGBA
        Eigen::Vector4f background_color;

        /// Light position (or direction to light)
        Eigen::Vector3f light_position;
        /// Whether to treat `light_position` as a point or direction
        bool is_directional_light;
        /// Whether shadow mapping is on
        bool is_shadow_mapping;
        /// Width of the shadow map
        GLuint shadow_width;
        /// Height of the shadow map
        GLuint shadow_height;
        /// Shadow map depth texture
        GLuint shadow_depth_tex;
        /// Shadow map depth framebuffer object
        GLuint shadow_depth_fbo;
        /// Shadow map color render buffer object
        GLuint shadow_color_rbo;
        /// Factor of lighting (0: no lighting, 1: full lighting)
        float lighting_factor;

        /// Type of rotation interaction
        RotationType rotation_type;
        /// View rotation as quaternion
        Eigen::Quaternionf trackball_angle;

        /// Whether testing for depth is enabled
        bool depth_test = true;

        /// Whether "animating" (continuous drawing) is enabled
        bool is_animating;
        /// Max fps of animation loop (e.g. 30fps or 60fps)
        double animation_max_fps;

        /// Caches the two-norm between the min/max point of the bounding box
        float object_scale;

        /// Base zoom of camera
        float camera_base_zoom;
        /// Current zoom of camera
        float camera_zoom;
        /// Whether camera is orthographic (or perspective)
        bool orthographic = false;
        /// Base translation of camera
        Eigen::Vector3f camera_base_translation;
        /// Current translation of camera
        Eigen::Vector3f camera_translation;
        /// Current "eye" / origin position of camera
        Eigen::Vector3f camera_eye;
        /// Current "up" vector of camera
        Eigen::Vector3f camera_up;
        /// Current "look at" position of camera
        Eigen::Vector3f camera_center;
        /// Current view angle of camera
        float camera_view_angle;
        /// Near plane of camera
        float camera_dnear;
        /// Far plane of camera
        float camera_dfar;

        /// OpenGL view transformation matrix on last render pass
        Eigen::Matrix4f view;
        /// OpenGL proj transformation matrix on last render pass
        Eigen::Matrix4f proj;
        /// OpenGL norm transformation matrix on last render pass
        Eigen::Matrix4f norm;

        /// OpenGL shadow_view transformation matrix on last render pass
        Eigen::Matrix4f shadow_view;
        /// OpenGL shadow_proj transformation matrix on last render pass
        Eigen::Matrix4f shadow_proj;

    private:
        /// scene data
        static constexpr float arc_line_width = 5.0f;
        static constexpr float seg_line_width = 5.0f;
        static constexpr float point_width = 10.0f;
        std::shared_ptr<PCBScene> pcb_scene = nullptr;

    public:
        /// Constructors
        Viewer() = default;

        /**
         *
         * @param window_width
         * @param window_height
         */
        Viewer(int window_width, int window_height);

        ~Viewer();

        void set_scene(const std::shared_ptr<PCBScene> &_pcb_scene) { pcb_scene = _pcb_scene; }

    public:
        /*template<typename UserFn>
        void run(UserFn &&user_fn) const {

            init_scene();
        }*/

        void run_cp();

        void run_cd();

    private:
        /// Init necessitate context
        ERROR_CODE init_context(int window_width, int window_height);

        void initialize_shadow_pass();

        void setup_shaders();

        void setup_buffers();

        /// PCB Data rendering functions
        Eigen::Matrix4f scene_mvp, dp_mvp, db_mvp;
        void set_seg_data(const PCBSeg &seg, const Eigen::Vector3f &color);

        void set_arc_data(const PCBArc &arc, const Eigen::Vector3f &color);

        void set_bbox_data(const BBox2 &bbox);

        void set_scene_data(Eigen::Matrix4f &MVP, const float scale_factor = 1.0);

        void set_dp_data(Eigen::Matrix4f &MVP, const int num_pts, const float scale_factor = 1.0);

        void set_db_data(Eigen::Matrix4f &MVP, const int num_db, const float scale_factor = 1.0);

        void update_dp(int num_dp);

        void update_db(int num_bbox, bool& scene_collision);

        void draw_dp(const Eigen::Vector2f &p);

        void draw_db(const std::array<Eigen::Vector2f, 4> &bbox_pos);

        /// Widgets
        void render_widgets();

    private:
        /// Utilities
        Vec2 windowToArcCoordinates(double xpos, double ypos, int window_width, int window_height) {
            Vec2 arcCoords;
            arcCoords[0] = (xpos / window_width) * 2.0f - 1.0f; // X 方向转换
            arcCoords[1] = 1.0f - (ypos / window_height) * 2.0f; // Y 方向转换 注意Y轴翻转
            return arcCoords;
        }

        /// Callbacks
//        static double highdpiw = 1; // High DPI width
//        static double highdpih = 1; // High DPI height
//        static double scroll_x = 0;
//        static double scroll_y = 0;

        static void glfw_error_callback(int error, const char *description) {
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
        }

        static void glfw_mouse_press(GLFWwindow * /*window*/ , int button, int action, int modifier) {
            MouseButton mb;

            if (button == GLFW_MOUSE_BUTTON_1)
                mb = MouseButton::Left;
            else if (button == GLFW_MOUSE_BUTTON_2)
                mb = MouseButton::Right;
            else //if (button == GLFW_MOUSE_BUTTON_3)
                mb = MouseButton::Middle;

            /*if (action == GLFW_PRESS)
                mouse_down(mb, modifier);
            else
                mouse_up(mb, modifier);*/
        }

        static void glfw_char_mods_callback(GLFWwindow * /*window*/ , unsigned int codepoint, int modifier) {
//            __viewer->key_pressed(codepoint, modifier);
        }

        static void glfw_key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int modifier) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GL_TRUE);

            /*if (action == GLFW_PRESS)
                key_down(key, modifier);
            else if (action == GLFW_RELEASE)
                key_up(key, modifier);*/
        }

        static void glfw_window_size(GLFWwindow * /*window*/ , int width, int height) {
//            int w = width * highdpiw;
//            int h = height * highdpih;

            // post_resize(w, h);
        }

        static void glfw_mouse_move(GLFWwindow * /*window*/ , double x, double y) {
            // mouse_move(x * highdpiw, y * highdpih);
        }

        static void glfw_mouse_scroll(GLFWwindow * /*window*/ , double x, double y) {
            using namespace std;
//            scroll_x += x;
//            scroll_y += y;

            // mouse_scroll(y);
        }

        static void glfw_drop_callback(GLFWwindow * /*window*/, int /*count*/, const char ** /*filenames*/) {
        }
    };
}

#endif //PCB_OFFSET_SCENE_H
