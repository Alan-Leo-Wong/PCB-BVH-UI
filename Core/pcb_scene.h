#ifndef PCB_OFFSET_PCB_SCENE_H
#define PCB_OFFSET_PCB_SCENE_H

#include "error.h"

#include <unordered_map>

#include <bvh/v2/Node.h>
#include <bvh/v2/Bvh.h>
#include <bvh/v2/pcb_data.h>

namespace core {

    class PCBScene {
        using Scalar = double;
        using index_t = uint64_t;
        using Vec2 = bvh::v2::Vec<Scalar, 2>;
        using BBox2 = bvh::v2::BBox<Scalar, 2>;
        using PCBData = bvh::v2::PCBData<Scalar, 2>;
        using Point = typename PCBData::Point;
        using PCBSeg = typename bvh::v2::PCBSeg<Scalar, 2>;
        using PCBArc = typename bvh::v2::PCBArc<Scalar, 2>;
        using BvhNode = bvh::v2::Node<Scalar, 2>;
        using Bvh = bvh::v2::Bvh<BvhNode>;

    private:
        /// input data
        std::unordered_map<index_t, Point> P_coord; //
        std::unordered_map<index_t, Point> C_coord; //
        std::vector<std::shared_ptr<PCBData>> pcb_data; //

        /// Bounding-box
        BBox2 bounding_box = {Vec2(std::numeric_limits<Scalar>::max(), std::numeric_limits<Scalar>::max()),
                              Vec2(std::numeric_limits<Scalar>::min(), std::numeric_limits<Scalar>::min())};

        /// BVH data
        std::shared_ptr<Bvh> bvh;

    private:
        /// functions for input
        /**
         *
         * @param line
         * @return
         */
        ERROR_CODE
        read_pcb_points(const std::string &line);

        /**
         *
         * @param token
         * @return
         */
        ERROR_CODE
        read_pcb_segs(const std::vector<std::wstring> &token);

        /**
         *
         * @param token
         * @return
         */
        ERROR_CODE
        read_pcb_arcs(const std::vector<std::wstring> &token);

    public:
        /// Constructors
        PCBScene() = default;

        PCBScene(const std::string &in_file);

        /// Getters
        /**
         *
         * @return
         */
        [[nodiscard]] const BBox2 &get_bounding_box() const { return bounding_box; }

        /**
         *
         * @return
         */
        [[nodiscard]] const std::vector<std::shared_ptr<PCBData>> &get_data() const { return pcb_data; }

        /**
         *
         * @return
         */
        [[nodiscard]] const std::shared_ptr<Bvh> &get_bvh() const { return bvh; }

    public:
        /// core functions
        /**
         *
         * @param in_file
         * @return
         */
        ERROR_CODE
        read_data(const std::string &in_file);

        /**
         *
         * @return
         */
        ERROR_CODE
        create_bvh();

    public:
        /// utility functions for distance query and ray intersection via bvh
        /**
         *
         * @param q
         * @param dis
         * @param closest
         * @return
         */
        ERROR_CODE
        get_closest(const Point &q, double &dis, Point &closest);

        /**
         *
         * @param bbox
         * @param inter_pris
         * @return
         */
        ERROR_CODE
        collision_detection(const BBox2 &bbox, std::vector<PCBData*> &inter_pris);

        /**
         *
         * @param pcb_pri
         * @param inter_pris
         * @return
         */
        ERROR_CODE
        collision_detection(const PCBData &pcb_pri, std::vector<PCBData*> &inter_pris);

    public:
        /// functions for visualization
        /**
         *
         * @param out_file
         * @return
         */
        ERROR_CODE
        output_scene(const std::string &out_file);
    };

}

#endif //PCB_OFFSET_PCB_SCENE_H
