#pragma once
// rstarLib — R*-tree 2D reutilizable (Beckmann et al. 1990)
// Ver DISENO.md. Header-only, C++17, sin dependencias.
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <stdexcept>
#include <cmath>

struct Caja {
    double lo[2], hi[2];
    Caja() { reset(); }
    Caja(double xmin, double ymin, double xmax, double ymax) {
        lo[0] = xmin; lo[1] = ymin; hi[0] = xmax; hi[1] = ymax;
    }
    void reset() {
        lo[0] = lo[1] = std::numeric_limits<double>::max();
        hi[0] = hi[1] = std::numeric_limits<double>::lowest();
    }
    void estirar(const Caja& o) {
        for (int d = 0; d < 2; d++) {
            lo[d] = std::min(lo[d], o.lo[d]);
            hi[d] = std::max(hi[d], o.hi[d]);
        }
    }
    void estirar(double x, double y) {
        lo[0] = std::min(lo[0], x); hi[0] = std::max(hi[0], x);
        lo[1] = std::min(lo[1], y); hi[1] = std::max(hi[1], y);
    }
    double area() const { return (hi[0] - lo[0]) * (hi[1] - lo[1]); }
    double margen() const { return 2.0 * ((hi[0] - lo[0]) + (hi[1] - lo[1])); }
    double overlap(const Caja& o) const {
        double a = std::max(0.0, std::min(hi[0], o.hi[0]) - std::max(lo[0], o.lo[0]));
        double b = std::max(0.0, std::min(hi[1], o.hi[1]) - std::max(lo[1], o.lo[1]));
        return a * b;
    }
    bool interseca(const Caja& o) const {
        return !(hi[0] < o.lo[0] || o.hi[0] < lo[0] || hi[1] < o.lo[1] || o.hi[1] < lo[1]);
    }
    bool contiene(double x, double y) const {
        return x >= lo[0] && x <= hi[0] && y >= lo[1] && y <= hi[1];
    }
    // distancia al cuadrado del punto a la caja (0 si esta dentro)
    double dist2A(double x, double y) const {
        double dx = std::max({lo[0] - x, 0.0, x - hi[0]});
        double dy = std::max({lo[1] - y, 0.0, y - hi[1]});
        return dx * dx + dy * dy;
    }
};
