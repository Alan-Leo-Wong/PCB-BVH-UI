//
// Created by Lei on 10/6/2024.
//

#ifndef PCB_OFFSET_VIEWERDATA_H
#define PCB_OFFSET_VIEWERDATA_H

#include <Eigen/Dense>
#include <glad/glad.h>

#include <vector>

namespace ui {

    struct Vertex {
        Eigen::Vector2f position;  // 顶点位置
        Eigen::Vector3f color;     // 顶点颜色

        Vertex() = default;

        Vertex(const Eigen::Vector2f &_pos, const Eigen::Vector3f &_color)
                : position(_pos), color(_color) {}
    };

    struct DynamicPoint {
        Eigen::Vector2f position;  // 动态点的当前坐标
        Eigen::Vector2f velocity;  // 动态点的速度，用于控制点的运动
        Eigen::Vector2f closest_point;  // 最近点查询结果

        DynamicPoint() = default;

        DynamicPoint(const Eigen::Vector2f &_pos,
                     const Eigen::Vector2f &_velocity, const Eigen::Vector2f &_closest_point
        )
                : position(_pos), velocity(_velocity), closest_point(_closest_point) {}
    };

    struct DynamicBBox {
        std::array<Eigen::Vector2f, 4> position;
        Eigen::Vector2f velocity;
        bool is_collision = false;

        DynamicBBox() = default;

        DynamicBBox(const std::array<Eigen::Vector2f, 4> &_position,
                    const Eigen::Vector2f &_velocity,
                    bool _is_collision)
                : position(_position),
                  velocity(_velocity), is_collision(_is_collision) {}
    };

    class ViewerData {
    public:
        GLuint seg_VAO, seg_VBO, seg_EBO;
        GLuint arc_VAO, arc_VBO, arc_EBO;
        GLuint db_VAO, db_VBO, db_EBO;
        GLuint dp_VAO, dp_VBO;
        GLuint dp_line_VAO, dp_line_VBO;
        GLuint scene_shader_program;
        GLuint dp_shader_program;
        GLuint db_shader_program;
        GLuint dp_line_shader_program;

        GLuint seg_beg_indice = 0;
        GLuint arc_beg_indice = 0;
        std::vector<GLuint> seg_indices;
        std::vector<Vertex> seg_vertices;
        std::vector<GLuint> arc_indices;
        std::vector<Vertex> arc_vertices;
        static constexpr std::array<GLuint, 4> db_indices = {0, 1, 2, 3};
        std::vector<DynamicPoint> dynamic_points;
        std::vector<DynamicBBox> dynamic_bbox;

        bool is_initialized = false;
        GLuint vao_mesh;
        GLuint vao_overlay_lines;
        GLuint vao_overlay_points;
        GLuint shader_mesh;
        GLuint shader_overlay_lines;
        GLuint shader_overlay_points;
        GLuint shader_text;

        /// Shape material shininess
        float shininess;

        /// Vertices of the current mesh (#V x 3)
        GLuint vbo_V;
        /// UV coordinates for the current mesh (#V x 2)
        GLuint vbo_V_uv;
        /// Vertices of the current mesh (#V x 3)
        GLuint vbo_V_normals;
        /// Ambient material  (#V x 3)
        GLuint vbo_V_ambient;
        /// Diffuse material  (#V x 3)
        GLuint vbo_V_diffuse;
        /// Specular material  (#V x 3)
        GLuint vbo_V_specular;

        /// Faces of the mesh (#F x 3)
        GLuint vbo_F;
        /// Texture
        GLuint vbo_tex;

        /// Indices of the line overlay
        GLuint vbo_lines_F;
        /// Vertices of the line overlay
        GLuint vbo_lines_V;
        /// Color values of the line overlay
        GLuint vbo_lines_V_colors;
        /// Indices of the point overlay
        GLuint vbo_points_F;
        /// Vertices of the point overlay
        GLuint vbo_points_V;
        /// Color values of the point overlay
        GLuint vbo_points_V_colors;

    };
}

#endif //PCB_OFFSET_VIEWERDATA_H
