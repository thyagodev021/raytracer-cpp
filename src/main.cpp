#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "scene.hpp"

constexpr int MAX_DEPTH = 5;

Vec3 skyColor(const Vec3& dir) {
    double t = 0.5 * (dir.y + 1.0);
    return Vec3{1.0, 1.0, 1.0} * (1.0 - t) + Vec3{0.35, 0.55, 0.95} * t;
}

Vec3 trace(const Scene& scene, const Ray& ray, int depth) {
    auto hit = scene.closestHit(ray);
    if (!hit) return skyColor(ray.dir);

    const Material& m = hit->mat;
    Vec3 viewDir = -ray.dir;
    double diffuse = 0, spec = 0;

    for (const auto& light : scene.lights) {
        Vec3 toLight = light.position - hit->point;
        double dist = toLight.length();
        Vec3 l = toLight / dist;

        // sombra: algo bloqueia o caminho até a luz?
        if (scene.closestHit({hit->point, l}, 1e-4, dist)) continue;

        double ndl = std::max(0.0, dot(hit->normal, l));
        diffuse += light.intensity * ndl;

        Vec3 h = (l + viewDir).normalized();  // Blinn-Phong
        spec += light.intensity * m.specular * std::pow(std::max(0.0, dot(hit->normal, h)), m.shininess);
    }

    Vec3 local = m.color * (0.12 + diffuse) + Vec3{1, 1, 1} * spec;

    if (m.reflectivity > 0 && depth < MAX_DEPTH) {
        Ray rr{hit->point, reflect(ray.dir, hit->normal).normalized()};
        Vec3 refl = trace(scene, rr, depth + 1);
        return local * (1.0 - m.reflectivity) + refl * m.reflectivity;
    }
    return local;
}

Scene buildScene() {
    Scene s;
    s.floor = {0.0, Material{{1, 1, 1}, 0.0, 10, 0.25, true}};
    s.spheres = {
        {{0, 1.0, 0},       1.0, {{0.9, 0.15, 0.2}, 0.8, 80, 0.15}},
        {{-2.2, 0.8, 0.6},  0.8, {{0.95, 0.95, 0.95}, 1.0, 200, 0.85}},  // espelho
        {{2.1, 0.7, 0.4},   0.7, {{0.2, 0.8, 0.3}, 0.6, 60, 0.2}},
        {{0.9, 0.35, 2.0},  0.35, {{0.95, 0.8, 0.1}, 0.9, 120, 0.3}},
        {{-0.9, 0.3, 2.3},  0.3, {{0.3, 0.4, 0.95}, 0.9, 120, 0.3}},
        {{0.2, 0.5, -2.5},  0.5, {{0.8, 0.4, 0.9}, 0.7, 70, 0.5}},
    };
    s.lights = {{{-5, 8, 4}, 0.75}, {{6, 5, 2}, 0.35}};
    return s;
}

void writePPM(const std::string& path, int w, int h, const std::vector<Vec3>& img) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << " " << h << "\n255\n";
    for (const auto& c : img) {
        auto to8 = [](double v) {
            v = std::pow(std::clamp(v, 0.0, 1.0), 1.0 / 2.2);  // correção gama
            return static_cast<unsigned char>(v * 255.999);
        };
        f.put(to8(c.x)); f.put(to8(c.y)); f.put(to8(c.z));
    }
}

int main(int argc, char** argv) {
    int width = argc > 1 ? std::atoi(argv[1]) : 960;
    int height = argc > 2 ? std::atoi(argv[2]) : 540;
    int samples = argc > 3 ? std::atoi(argv[3]) : 16;  // anti-aliasing
    std::string out = argc > 4 ? argv[4] : "render.ppm";

    Scene scene = buildScene();
    Vec3 camPos{0, 2.2, 7}, target{0, 0.8, 0};
    Vec3 fwd = (target - camPos).normalized();
    Vec3 right = cross(fwd, {0, 1, 0}).normalized();
    Vec3 up = cross(right, fwd);
    double aspect = static_cast<double>(width) / height;
    double fov = std::tan(35.0 * M_PI / 180.0);

    std::vector<Vec3> image(static_cast<size_t>(width) * height);
    std::atomic<int> nextRow{0}, done{0};

    auto worker = [&](unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> jitter(0.0, 1.0);
        for (int y; (y = nextRow++) < height;) {
            for (int x = 0; x < width; ++x) {
                Vec3 col;
                for (int s = 0; s < samples; ++s) {
                    double u = ((x + jitter(rng)) / width * 2 - 1) * aspect * fov;
                    double v = (1 - (y + jitter(rng)) / height * 2) * fov;
                    Ray r{camPos, (fwd + right * u + up * v).normalized()};
                    col += trace(scene, r, 0);
                }
                image[static_cast<size_t>(y) * width + x] = col / samples;
            }
            int d = ++done;
            if (d % 20 == 0 || d == height)
                std::cerr << "\rRenderizando: " << d * 100 / height << "%" << std::flush;
        }
    };

    unsigned n = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    for (unsigned i = 0; i < n; ++i) pool.emplace_back(worker, 1234 + i);
    for (auto& t : pool) t.join();

    writePPM(out, width, height, image);
    std::cerr << "\nPronto! Salvo em " << out << " (" << n << " threads)\n";
}
