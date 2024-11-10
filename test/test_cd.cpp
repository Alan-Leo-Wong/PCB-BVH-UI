//
// Created by Lei on 10/4/2024.
//
#include <omp.h>
#include <string>
#include <fstream>
#include <chrono>
#include <random>

#include <bvh/v2/thread_pool.h>
#include <bvh/v2/executor.h>
#include <bvh/v2/default_builder.h>


#include <UI/Viewer.h>
#include <Core/pcb_scene.h>

using namespace core;
using namespace ui;
using Vec2 = bvh::v2::Vec<double, 2>;
using BBox2 = bvh::v2::BBox<double, 2>;
using PCBData2 = bvh::v2::PCBData<double, 2>;
using BvhNode = bvh::v2::Node<double, 2>;
using BVH = bvh::v2::Bvh<BvhNode>;

void output_scene_bbox(const std::string &out_file, const BBox2 &bbox) {
    std::ofstream out(out_file);
    size_t cnt = 1;
    {
        Vec2 min = bbox.min;
        Vec2 max = bbox.max;
        Vec2 top_left_corner = {min[0], max[1]};
        Vec2 bottom_right_corner = {max[0], min[1]};

        out << "v " << min[0] << " " << min[1] << " 0\n";
        out << "v " << top_left_corner[0] << " " << top_left_corner[1] << " 0\n";
        out << "v " << max[0] << " " << max[1] << " 0\n";
        out << "v " << bottom_right_corner[0] << " " << bottom_right_corner[1] << " 0\n";

        out << "l " << cnt << " " << cnt + 1 << "\n";
        out << "l " << cnt + 1 << " " << cnt + 2 << "\n";
        out << "l " << cnt + 2 << " " << cnt + 3 << "\n";
        out << "l " << cnt + 3 << " " << cnt << "\n";

        cnt += 4;
    }
}

void output_bvh(const std::string &out_file, const std::vector<std::unique_ptr<PCBData2>> &pcb_data) {
    std::ofstream out(out_file);
    size_t cnt = 1;
    for (const std::unique_ptr<PCBData2> &data: pcb_data) {
        BBox2 bbox = data->get_bbox();

        Vec2 min = bbox.min;
        Vec2 max = bbox.max;
        Vec2 top_left_corner = {min[0], max[1]};
        Vec2 bottom_right_corner = {max[0], min[1]};

        out << "v " << min[0] << " " << min[1] << " 0\n";
        out << "v " << top_left_corner[0] << " " << top_left_corner[1] << " 0\n";
        out << "v " << max[0] << " " << max[1] << " 0\n";
        out << "v " << bottom_right_corner[0] << " " << bottom_right_corner[1] << " 0\n";

        out << "l " << cnt << " " << cnt + 1 << "\n";
        out << "l " << cnt + 1 << " " << cnt + 2 << "\n";
        out << "l " << cnt + 2 << " " << cnt + 3 << "\n";
        out << "l " << cnt + 3 << " " << cnt << "\n";

        cnt += 4;
    }
}

double generateRandomDouble(double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}

BBox2 genBBox(const Vec2 &scene_min, const Vec2 &scene_max, const Vec2 &scene_width) {
    double random_x = generateRandomDouble(scene_min[0], scene_max[0]);
    double random_y = generateRandomDouble(scene_min[1], scene_max[1]);
    Vec2 bbox_min = Vec2(random_x, random_y);
    double width = generateRandomDouble(scene_width[0] * 0.1, scene_width[0] * 0.3);
    double height = generateRandomDouble(scene_width[1] * 0.1, scene_width[1] * 0.3);
    Vec2 bbox_max = bbox_min + Vec2(width, height);

    return BBox2(bbox_min, bbox_max);
}

void test_cd(const std::shared_ptr<PCBScene> &pcb_scene) {
    using namespace std;
    using namespace chrono;

    const auto bvh = pcb_scene->get_bvh();
    const BBox2 scene_bbox = pcb_scene->get_bounding_box();
    Vec2 scene_min = scene_bbox.min;
    Vec2 scene_max = scene_bbox.max;
    Vec2 scene_width = scene_bbox.max - scene_bbox.min;

    std::vector<BBox2> test_bbox;
    test_bbox.reserve(100000);
    for (int i = 0; i < 1000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
    }

    auto start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 1000; ++i) {
        std::vector<PCBData2 *> inter_pris;
        pcb_scene->collision_detection(test_bbox[i], inter_pris);
    }
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "#1K collision detection spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 4000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 5000; ++i) {
        std::vector<PCBData2 *> inter_pris;
        pcb_scene->collision_detection(test_bbox[i], inter_pris);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#5K collision detection spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 5000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 10000; ++i) {
        std::vector<PCBData2 *> inter_pris;
        pcb_scene->collision_detection(test_bbox[i], inter_pris);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#10K collision detection spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 40000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 50000; ++i) {
        std::vector<PCBData2 *> inter_pris;
        pcb_scene->collision_detection(test_bbox[i], inter_pris);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#50K collision detection spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 50000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 100000; ++i) {
        std::vector<PCBData2 *> inter_pris;
        pcb_scene->collision_detection(test_bbox[i], inter_pris);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#100K collision detection spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;
}

void test_bvh(const std::shared_ptr<PCBScene> &pcb_scene) {
    using namespace std;
    using namespace chrono;

    bvh::v2::ThreadPool thread_pool;
    bvh::v2::ParallelExecutor executor(thread_pool);

    typename bvh::v2::DefaultBuilder<BvhNode>::Config config;
    config.quality = bvh::v2::DefaultBuilder<BvhNode>::Quality::High;

    const BBox2 scene_bbox = pcb_scene->get_bounding_box();
    Vec2 scene_min = scene_bbox.min;
    Vec2 scene_max = scene_bbox.max;
    Vec2 scene_width = scene_bbox.max - scene_bbox.min;

    std::vector<BBox2> test_bbox;
    std::vector<Vec2> test_bbox_centers;
    test_bbox.reserve(100000);
    test_bbox_centers.reserve(100000);
    for (int i = 0; i < 1000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
        test_bbox_centers.emplace_back(test_bbox.back().get_center());
    }
    auto start = system_clock::now();
    {
        auto bvh = std::make_shared<BVH>(
                bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, test_bbox, test_bbox_centers, config));
    }
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "#1K bvh construction spent "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 4000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
        test_bbox_centers.emplace_back(test_bbox.back().get_center());
    }
    start = system_clock::now();
    {
        auto bvh = std::make_shared<BVH>(
                bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, test_bbox, test_bbox_centers, config));
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#5K bvh construction spent "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 5000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
        test_bbox_centers.emplace_back(test_bbox.back().get_center());
    }
    start = system_clock::now();
    {
        auto bvh = std::make_shared<BVH>(
                bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, test_bbox, test_bbox_centers, config));
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#10K bvh construction spent "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 40000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
        test_bbox_centers.emplace_back(test_bbox.back().get_center());
    }
    start = system_clock::now();
    {
        auto bvh = std::make_shared<BVH>(
                bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, test_bbox, test_bbox_centers, config));
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#50K bvh construction spent "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 50000; ++i) {
        test_bbox.emplace_back(genBBox(scene_min, scene_max, scene_width));
        test_bbox_centers.emplace_back(test_bbox.back().get_center());
    }
    start = system_clock::now();
    {
        auto bvh = std::make_shared<BVH>(
                bvh::v2::DefaultBuilder<BvhNode>::build(thread_pool, test_bbox, test_bbox_centers, config));
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#100K bvh construction spent "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;
}

int main(int argc, char **argv) {
    const std::string pcb_in = argc > 1 ? argv[1] : "initial_hard.txt";
    std::shared_ptr<PCBScene> pcb_scene = std::make_shared<PCBScene>(pcb_in);

    Vec2 bbox_min = {1.19139e+06, -3.99955e+06};
    Vec2 bbox_max = {1.2069e+06, -3.98404e+06};
    BBox2 bbox = BBox2(bbox_min, bbox_max);
    std::vector<PCBData2 *> inter_pris;
    pcb_scene->collision_detection(bbox, inter_pris);
    std::cout << std::boolalpha << (inter_pris.size() > 0) << std::endl;

//    test_cd(pcb_scene);
//    test_bvh(pcb_scene);

    Viewer viewer(1920, 1920);
    viewer.set_scene(pcb_scene);

    viewer.run_cd();

    return 0;
}
