#pragma once
#include <optional>
#include <vector>
#include "vec3.hpp"

struct Ray {
    Vec3 origin, dir;
    Vec3 at(double t) const { return origin + dir * t; }
};

struct Material {
    Vec3 color{1, 1, 1};
    double specular = 0.5;    // intensidade do brilho
    double shininess = 50.0;  // concentração do brilho
    double reflectivity = 0;  // 0 = fosco, 1 = espelho
    bool checker = false;     // padrão xadrez (usado no piso)
};

struct Hit {
    double t;
    Vec3 point, normal;
    Material mat;
};

struct Sphere {
    Vec3 center;
    double radius;
    Material mat;

    std::optional<Hit> intersect(const Ray& r, double tmin, double tmax) const {
        Vec3 oc = r.origin - center;
        double b = dot(oc, r.dir);
        double c = dot(oc, oc) - radius * radius;
        double disc = b * b - c;
        if (disc < 0) return std::nullopt;
        double sq = std::sqrt(disc);
        double t = -b - sq;
        if (t < tmin || t > tmax) {
            t = -b + sq;
            if (t < tmin || t > tmax) return std::nullopt;
        }
        Vec3 p = r.at(t);
        return Hit{t, p, (p - center) / radius, mat};
    }
};

struct Plane {  // plano horizontal y = height
    double height;
    Material mat;

    std::optional<Hit> intersect(const Ray& r, double tmin, double tmax) const {
        if (std::abs(r.dir.y) < 1e-9) return std::nullopt;
        double t = (height - r.origin.y) / r.dir.y;
        if (t < tmin || t > tmax) return std::nullopt;
        Vec3 p = r.at(t);
        Material m = mat;
        if (m.checker) {
            bool on = (static_cast<long>(std::floor(p.x)) + static_cast<long>(std::floor(p.z))) & 1;
            m.color = on ? Vec3{0.9, 0.9, 0.9} : Vec3{0.1, 0.1, 0.12};
        }
        return Hit{t, p, {0, 1, 0}, m};
    }
};

struct Light {
    Vec3 position;
    double intensity;
};

struct Scene {
    std::vector<Sphere> spheres;
    Plane floor;
    std::vector<Light> lights;

    std::optional<Hit> closestHit(const Ray& r, double tmin = 1e-4, double tmax = 1e9) const {
        std::optional<Hit> best;
        double closest = tmax;
        for (const auto& s : spheres)
            if (auto h = s.intersect(r, tmin, closest)) { best = h; closest = h->t; }
        if (auto h = floor.intersect(r, tmin, closest)) best = h;
        return best;
    }
};
