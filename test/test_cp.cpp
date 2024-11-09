//
// Created by Lei on 10/6/2024.
//
#include <string>
#include <fstream>
#include <random>
#include <chrono>

#include <Core/pcb_scene.h>
#include <UI/Viewer.h>

using namespace core;
using namespace ui;
using Scalar = double;
using Vec2 = bvh::v2::Vec<Scalar, 2>;
using BBox2_ = bvh::v2::BBox<Scalar, 2>;
using PCBData2 = bvh::v2::PCBData<Scalar, 2>;
using PCBArc2 = bvh::v2::PCBArc<Scalar, 2>;

double generateRandomDouble(double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}

Vec2 genPoint(const Vec2 &scene_min, const Vec2 &scene_max) {
    double random_x = generateRandomDouble(scene_min[0], scene_max[0]);
    double random_y = generateRandomDouble(scene_min[1], scene_max[1]);
    Vec2 point = Vec2(random_x, random_y);

    return point;
}

void test_cp(const std::shared_ptr<PCBScene> &pcb_scene) {
    using namespace std;
    using namespace chrono;

    const auto bvh = pcb_scene->get_bvh();
    const BBox2_ scene_bbox = pcb_scene->get_bounding_box();
    const Vec2 scene_min = scene_bbox.min;
    const Vec2 scene_max = scene_bbox.max;

    std::vector<Vec2> test_points;
    test_points.reserve(100000);
    for (int i = 0; i < 1000; ++i) {
        test_points.emplace_back(genPoint(scene_min, scene_max));
    }

    auto start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 1000; ++i) {
        double dis;
        Vec2 closest;
        pcb_scene->get_closest(test_points[i], dis, closest);
    }
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "#1K closest queries spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 4000; ++i) {
        test_points.emplace_back(genPoint(scene_min, scene_max));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 5000; ++i) {
        double dis;
        Vec2 closest;
        pcb_scene->get_closest(test_points[i], dis, closest);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#5K closest queries spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 5000; ++i) {
        test_points.emplace_back(genPoint(scene_min, scene_max));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 10000; ++i) {
        double dis;
        Vec2 closest;
        pcb_scene->get_closest(test_points[i], dis, closest);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#10K closest queries spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 40000; ++i) {
        test_points.emplace_back(genPoint(scene_min, scene_max));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 50000; ++i) {
        double dis;
        Vec2 closest;
        pcb_scene->get_closest(test_points[i], dis, closest);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#50K closest queries spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;

    for (int i = 0; i < 50000; ++i) {
        test_points.emplace_back(genPoint(scene_min, scene_max));
    }
    start = system_clock::now();
#pragma omp parallel for
    for (int i = 0; i < 100000; ++i) {
        double dis;
        Vec2 closest;
        pcb_scene->get_closest(test_points[i], dis, closest);
    }
    end = system_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "#100K closest queries spent"
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " s" << endl;
}

int main(int argc, char **argv) {
    const std::string pcb_in = "initial_normal.txt";
    std::shared_ptr<PCBScene> pcb_scene = std::make_shared<PCBScene>(pcb_in);

//    test_cp(pcb_scene);
    Viewer viewer(1920, 1920);
    viewer.set_scene(pcb_scene);

    viewer.run_cp();

    return 0;
}