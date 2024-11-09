#include "pcb_scene.h"

#include <string>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <regex>

#include <bvh/v2/stack.h>
#include <bvh/v2/executor.h>
#include <bvh/v2/thread_pool.h>
#include <bvh/v2/default_builder.h>

namespace core {

    ////////////////////////
    //    Constructors    //
    ////////////////////////
    PCBScene::PCBScene(const std::string &in_file) {
        read_data(in_file);
        create_bvh();
    }

    ////////////////////////
    //        Input       //
    ////////////////////////
    ERROR_CODE
    PCBScene::read_pcb_points(const std::string &line) {
        index_t left_paren_pos = line.find('(');
        index_t right_paren_pos = line.find(')');
        index_t comma_pos = line.find(',');
        if (left_paren_pos == std::string::npos ||
            right_paren_pos == std::string::npos ||
            comma_pos == std::string::npos)
            return ERROR_CODE::ERROR_IO_FAILURE;

        double x = std::stod(line.substr(left_paren_pos + 1, comma_pos - left_paren_pos - 1));
        double y = std::stod(line.substr(comma_pos + 1, right_paren_pos - comma_pos - 1));

        index_t equal_pos = line.find('=');
        if (line[0] == 'P') {
            index_t p_index = std::stoull(line.substr(1, equal_pos - 1));
            P_coord[p_index] = Point(x, y);
        } else {
            index_t c_index = std::stoull(line.substr(1, equal_pos - 1));
            C_coord[c_index] = Point(x, y);
        }

        return ERROR_CODE::SUCCESS;
    }

    ERROR_CODE
    PCBScene::read_pcb_segs(const std::vector<std::wstring> &token) {
        if (token.size() != 2) {
            std::cerr << "token.size() != 2\n";
            return ERROR_CODE::ERROR_IO_FAILURE;
        }

        index_t pos_0 = std::stoull(token[0].substr(1));
        index_t pos_1 = std::stoull(token[1].substr(1));

        if (!P_coord.count(pos_0) || !P_coord.count(pos_1))
            return ERROR_CODE::ERROR_IO_FAILURE;

        Point p0 = P_coord.at(pos_0);
        Point p1 = P_coord.at(pos_1);

        pcb_data.push_back(std::make_shared<PCBSeg>(p0, p1));

        return ERROR_CODE::SUCCESS;
    }

    ERROR_CODE
    PCBScene::read_pcb_arcs(const std::vector<std::wstring> &token) {
        if (token.size() != 3)
            return ERROR_CODE::ERROR_IO_FAILURE;

        index_t pos_c = std::stoull(token[0].substr(1));
        index_t pos_0 = std::stoull(token[1].substr(1));
        index_t pos_1 = std::stoull(token[2].substr(1));

        if (!C_coord.count(pos_c) || !P_coord.count(pos_0) || !P_coord.count(pos_1))
            return ERROR_CODE::ERROR_IO_FAILURE;

        Point center = C_coord.at(pos_c);
        Point p0 = P_coord.at(pos_0);
        Point p1 = P_coord.at(pos_1);

        pcb_data.push_back(std::make_shared<PCBArc>(center, p0, p1));
        pcb_data.back()->is_arc = true;

        return ERROR_CODE::SUCCESS;
    }

    ERROR_CODE
    PCBScene::read_data(const std::string &in_file) {
        std::ifstream in(in_file);
        if (!in.is_open()) return ERROR_CODE::ERROR_IO_FAILURE;

        std::string line;

        // preprocessing
        std::string _seg_name = "线段";
        std::string _arc_name = "圆弧";
        std::wstringstream wss1;
        wss1 << _seg_name.c_str();
        std::wstring seg_name = wss1.str();
        std::wstringstream wss2;
        wss2 << _arc_name.c_str();
        std::wstring arc_name = wss2.str();

        std::regex line_st_pattern(R"(^P\d+=|C\d+=|l\d+=)");
        while (getline(in, line)) {
            if (!std::regex_search(line, line_st_pattern)) continue;
            if (line[0] == 'P' || line[0] == 'C') { /// points
                if (read_pcb_points(line) != ERROR_CODE::SUCCESS)
                    return ERROR_CODE::ERROR_IO_FAILURE;
            } else {
                std::wstringstream wss;
                wss << line.c_str();
                std::wstring c_line = wss.str();

                index_t equal_pos = c_line.find(L'=');
                index_t left_paren_pos = c_line.find(L'(');
                index_t right_paren_pos = c_line.find(L')');
                if (equal_pos == std::wstring::npos ||
                    left_paren_pos == std::wstring::npos ||
                    right_paren_pos == std::wstring::npos)
                    return ERROR_CODE::ERROR_IO_FAILURE;

                std::wstring type = c_line.substr(equal_pos + 2, left_paren_pos - equal_pos - 2);
                std::wstring data = c_line.substr(left_paren_pos + 1, right_paren_pos - left_paren_pos - 1);

                index_t comma_pos = 0;
                std::vector<std::wstring> token;
                while ((comma_pos = data.find(L',')) != std::wstring::npos) {
                    token.emplace_back(data.substr(0, comma_pos));
                    data.erase(0, comma_pos + 1);
                }
                token.emplace_back(data);

                if (type.compare(seg_name) == 0) { /// segments
                    if (read_pcb_segs(token) != ERROR_CODE::SUCCESS)
                        return ERROR_CODE::ERROR_IO_FAILURE;
                } else if (type.compare(arc_name) == 0) { /// arcs
                    if (read_pcb_arcs(token) != ERROR_CODE::SUCCESS)
                        return ERROR_CODE::ERROR_IO_FAILURE;
                } else {
                    return ERROR_CODE::ERROR_IO_FAILURE;
                }
            }
        }

        in.close();
        return ERROR_CODE::SUCCESS;
    }

    ////////////////////////
    //         BVH        //
    ////////////////////////
    ERROR_CODE
    PCBScene::create_bvh() {
        using namespace std;
        using namespace chrono;

        bvh::v2::ThreadPool thread_pool;
        bvh::v2::ParallelExecutor executor(thread_pool);

        size_t num_pris = pcb_data.size();
        std::vector<BBox2> bboxes(num_pris);
        std::vector<Vec2> centers(num_pris);

        executor.for_each(0, num_pris, [&](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                bboxes[i] = pcb_data[i]->get_bbox();
                centers[i] = pcb_data[i]->get_bbox_center();
            }
        });

        typename bvh::v2::DefaultBuilder<BvhNode>::Config config;
        config.quality = bvh::v2::DefaultBuilder<BvhNode>::Quality::High;

        auto start = system_clock::now();
        bvh = std::make_shared<Bvh>(bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, bboxes, centers, config));
        auto end = system_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        cout << "bvh construction spent "
             << double(duration.count()) * microseconds::period::num / microseconds::period::den
             << " s" << endl;

        bounding_box = bvh->get_root().get_bbox();
        // scale to a square for constructing octree correctly
        // TODO: optimize
        {
            double w = bounding_box.max[0] - bounding_box.min[0];
            double h = bounding_box.max[1] - bounding_box.min[1];
            double max_side = std::max(w, h);

            Vec2 center = bounding_box.get_center();

            double x_min = center[0] - max_side / 2.0;
            double x_max = center[0] + max_side / 2.0;
            double y_min = center[1] - max_side / 2.0;
            double y_max = center[1] + max_side / 2.0;

            bounding_box = BBox2(Vec2(x_min, y_min), Vec2(x_max, y_max));
        }

        return ERROR_CODE::SUCCESS;
    }

    ERROR_CODE
    PCBScene::get_closest(const Point &q, double &dis, Point &closest) {
        static constexpr size_t stack_size = 64;
        bvh::v2::SmallStack<Bvh::Index, stack_size> stack;

        static constexpr size_t invalid_id = std::numeric_limits<size_t>::max();
        size_t prim_id = invalid_id;

        static constexpr bool should_permute = true;
        dis = std::numeric_limits<double>::max();
        bvh->closest_point(q, bvh->get_root().index, stack,
                           [&](size_t begin, size_t end) {
                               double min_pri_res = std::numeric_limits<double>::max();
                               for (size_t i = begin; i < end; ++i) {
                                   size_t j = should_permute ? i : bvh->prim_ids[i];
                                   auto res = pcb_data[j]->get_closest_dis(q);
                                   if (dis > res.first) {
                                       prim_id = i;
                                       std::tie(dis, closest) = res;
                                   }
                                   min_pri_res = std::min(min_pri_res, res.first);
                               }
                               return min_pri_res;
                           });

        dis = std::sqrt(dis);
        if (prim_id != invalid_id) return ERROR_CODE::SUCCESS;
        else return ERROR_CODE::ERROR_DATA_CORRUPTION;
    }

    ERROR_CODE
    PCBScene::collision_detection(const BBox2 &bbox, std::vector<PCBData *> &inter_pris) {
        inter_pris.clear();
        inter_pris.shrink_to_fit();
        inter_pris.reserve(pcb_data.size()); // might not work

        static constexpr size_t stack_size = 64;
        bvh::v2::SmallStack<Bvh::Index, stack_size> stack;

        static constexpr bool should_permute = false;
        bvh->intersect(bbox, bvh->get_root().index, stack,
                       [&](size_t begin, size_t end) {
                           for (size_t i = begin; i < end; ++i) {
                               size_t j = should_permute ? i : bvh->prim_ids[i];
                               auto res = pcb_data[j]->is_intersect(bbox);
                               if (res) inter_pris.push_back(pcb_data[j].get());
                           }
                       });

        if (!inter_pris.empty()) return ERROR_CODE::SUCCESS;
        else return ERROR_CODE::WARNING_UNEXPECTED_BEHAVIOR;
    }

    ERROR_CODE
    PCBScene::collision_detection(const PCBData &pcb_pri, std::vector<PCBData *> &inter_pris) {
        const BBox2 &bbox = pcb_pri.get_bbox();
        return collision_detection(bbox, inter_pris);
    }

    ////////////////////////
    //    Visualization   //
    ////////////////////////
    ERROR_CODE
    PCBScene::output_scene(const std::string &out_file) {
        std::ofstream out(out_file);
        if (!out) return ERROR_CODE::ERROR_IO_FAILURE;
        out << std::setprecision(15);

        index_t cnt = 1;
        for (const auto &pri: pcb_data) {
            if (pri->is_arc) {
                auto arc = dynamic_cast<PCBArc *>(pri.get());

                std::vector<Point> sample_points = arc->adaptive_sample();
                assert(sample_points.size() > 1);

                for (index_t i = 0; i < sample_points.size(); ++i) {
                    const Point &p = sample_points.at(i);
                    out << "v " << p[0] << " " << p[1] << " 0 " << " 0.71 0.49 0.86" << std::endl;
                    if (i >= 1) {
                        out << "l " << cnt - 1 << " " << cnt << std::endl;
                    }
                    ++cnt;
                }
            } else {
                auto [p0, p1] = pri->get_ed();
                out << "v " << p0[0] << " " << p0[1] << " 0 " << " 0.53 0.81 0.98" << std::endl;
                out << "v " << p1[0] << " " << p1[1] << " 0 " << " 0.53 0.81 0.98" << std::endl;

                out << "l " << cnt << " " << cnt + 1 << std::endl;

                cnt += 2;
            }
        }

        return ERROR_CODE::SUCCESS;
    }
}
