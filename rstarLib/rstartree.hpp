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

// R*-tree 2D con arena: las hojas guardan {x, y, idx} y el dato T completo
// vive una sola vez en la arena (vector<T>). Ver DISENO.md seccion 2.
template <typename T>
class RStarTree2D {
public:
    struct Resultado { double x, y; uint32_t idx; };

    explicit RStarTree2D(int M = 1200, int m = 480) : M_(M), m_(m) {
        if (m < 2 || m > M / 2) throw std::invalid_argument("m debe cumplir 2 <= m <= M/2");
    }
    ~RStarTree2D() { delete raiz_; }
    RStarTree2D(const RStarTree2D&) = delete;
    RStarTree2D& operator=(const RStarTree2D&) = delete;

    uint32_t insertar(double x, double y, T dato) {
        arena_.push_back(std::move(dato));
        uint32_t idx = (uint32_t)(arena_.size() - 1);
        insertarEntrada({x, y, idx});
        n_puntos_++;
        return idx;
    }
    const T& dato(uint32_t idx) const { return arena_[idx]; }
    T& dato(uint32_t idx) { return arena_[idx]; }
    size_t tamano() const { return n_puntos_; }

private:
    struct Nodo {
        bool esHoja;
        int nivel = 0;                   // 0 = hoja
        Caja mbr;
        std::vector<Nodo*> hijos;        // solo internos
        std::vector<Resultado> entradas; // solo hojas
        Nodo* padre = nullptr;
        uint64_t version = 0;            // para caches externos (grupos)
        explicit Nodo(bool hoja) : esHoja(hoja) {}
        ~Nodo() { for (Nodo* h : hijos) delete h; }
        Nodo(const Nodo&) = delete;
        Nodo& operator=(const Nodo&) = delete;
    };

    int M_, m_;
    std::vector<T> arena_;
    Nodo* raiz_ = nullptr;
    size_t n_puntos_ = 0;
    uint64_t contadorVersion_ = 0;
    std::vector<bool> nivelReinsertado_; // OT1: un reinsert por nivel por operacion

    void tocar(Nodo* hoja) { hoja->version = ++contadorVersion_; }

    // Version minima sin split (la maquinaria completa llega en la Task 4)
    void insertarEntrada(const Resultado& e) {
        if (raiz_ == nullptr) raiz_ = new Nodo(true);
        raiz_->entradas.push_back(e);
        raiz_->mbr.estirar(e.x, e.y);
        tocar(raiz_);
    }
};
