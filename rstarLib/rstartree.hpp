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
        nivelReinsertado_.assign(64, false);   // OT1: un reinsert por nivel por operacion
        insertarEntrada({x, y, idx});
        n_puntos_++;
        return idx;
    }
    const T& dato(uint32_t idx) const { return arena_[idx]; }
    T& dato(uint32_t idx) { return arena_[idx]; }
    size_t tamano() const { return n_puntos_; }

    std::vector<Resultado> buscarRango(const Caja& bbox) const {
        std::vector<Resultado> res;
        rangoRec(raiz_, bbox, res);
        return res;
    }
    void recorrer(const std::function<void(const Resultado&)>& visita) const {
        recorrerRec(raiz_, visita);
    }

    // Inspeccion estructural (tests, estadisticas):
    // f(esHoja, nivel, profundidad, mbr, nEntradas, nHijos, esRaiz)
    void inspeccionar(const std::function<void(bool, int, int, const Caja&, size_t, size_t, bool)>& f) const {
        inspeccionarRec(raiz_, 0, true, f);
    }

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

    // ========================================================================
    // INSERCION R*-TREE (Beckmann et al. 1990, seccion 4)
    // Portada de struct/GeoCluster.cpp (probada contra el paper) y
    // generalizada: entradas {x, y, idx}, M/m por constructor.
    // ========================================================================

    // I1-I4: inserta una entrada de datos en el nivel hoja
    void insertarEntrada(const Resultado& e) {
        if (raiz_ == nullptr) raiz_ = new Nodo(true);
        Caja ce(e.x, e.y, e.x, e.y);
        Nodo* hoja = chooseSubTree(ce, 0);                  // I1
        hoja->entradas.push_back(e);                        // I2
        tocar(hoja);
        ajustarHaciaArriba(hoja);                           // I4
        if ((int)hoja->entradas.size() > M_)                // I2/I3
            overflowTreatment(hoja);
    }

    // Reinsercion de una entrada de nodo interno: cuelga el subarbol completo
    // en un nodo del nivel que le corresponde (paper 4.3)
    void insertarSubarbol(Nodo* sub) {
        Nodo* n = chooseSubTree(sub->mbr, sub->nivel + 1);
        sub->padre = n;
        n->hijos.push_back(sub);
        ajustarHaciaArriba(n);
        if ((int)n->hijos.size() > M_)
            overflowTreatment(n);
    }

    // 4.1 ChooseSubtree: desciende hasta un nodo del nivel destino.
    // Hijos-hoja: minima ampliacion de overlap; hijos internos: de area.
    Nodo* chooseSubTree(const Caja& entrada, int nivelDestino) {
        Nodo* n = raiz_;                                    // CS1
        while (n != nullptr && n->nivel > nivelDestino) {   // CS2/CS3
            Nodo* mejor = (n->nivel == 1)
                ? hijoConMenorAmpliacionOverlap(n, entrada)
                : hijoConMenorAmpliacionArea(n, entrada);
            if (mejor == nullptr) return n;                 // no deberia ocurrir
            n = mejor;
        }
        return n;
    }

    static double ampliacionArea(const Caja& c, const Caja& e) {
        Caja ampliada = c;
        ampliada.estirar(e);
        return ampliada.area() - c.area();
    }

    Nodo* hijoConMenorAmpliacionArea(Nodo* n, const Caja& entrada) const {
        Nodo* mejor = nullptr;
        double mejorCosto = std::numeric_limits<double>::infinity();
        double mejorArea = std::numeric_limits<double>::infinity();
        for (Nodo* h : n->hijos) {
            double costo = ampliacionArea(h->mbr, entrada);
            double area = h->mbr.area();
            if (costo < mejorCosto || (costo == mejorCosto && area < mejorArea)) {
                mejorCosto = costo;
                mejorArea = area;
                mejor = h;
            }
        }
        return mejor;
    }

    // Overlap del paper: overlap(E_k) = sum_{i!=k} area(E_k ∩ E_i).
    // Optimizacion del paper (4.1): evaluar solo los 32 hijos con menor
    // ampliacion de area — el costo exacto es cuadratico en M.
    Nodo* hijoConMenorAmpliacionOverlap(Nodo* n, const Caja& entrada) const {
        const int CANDIDATOS = 32;
        int total = (int)n->hijos.size();

        std::vector<int> orden(total);
        for (int i = 0; i < total; i++) orden[i] = i;
        std::sort(orden.begin(), orden.end(), [&](int a, int b) {
            return ampliacionArea(n->hijos[a]->mbr, entrada) <
                   ampliacionArea(n->hijos[b]->mbr, entrada);
        });

        int evaluar = std::min(total, CANDIDATOS);
        Nodo* mejor = nullptr;
        double mejorOverlap = std::numeric_limits<double>::infinity();
        double mejorCostoArea = std::numeric_limits<double>::infinity();
        double mejorArea = std::numeric_limits<double>::infinity();

        for (int c = 0; c < evaluar; c++) {
            Nodo* hijoK = n->hijos[orden[c]];
            Caja ampliado = hijoK->mbr;
            ampliado.estirar(entrada);

            double antes = 0.0, despues = 0.0;
            for (int i = 0; i < total; i++) {
                if (n->hijos[i] == hijoK) continue;
                antes   += hijoK->mbr.overlap(n->hijos[i]->mbr);
                despues += ampliado.overlap(n->hijos[i]->mbr);
            }
            double costoOverlap = despues - antes;
            double costoArea = ampliacionArea(hijoK->mbr, entrada);
            double area = hijoK->mbr.area();

            if (costoOverlap < mejorOverlap ||
                (costoOverlap == mejorOverlap &&
                 (costoArea < mejorCostoArea ||
                  (costoArea == mejorCostoArea && area < mejorArea)))) {
                mejorOverlap = costoOverlap;
                mejorCostoArea = costoArea;
                mejorArea = area;
                mejor = hijoK;
            }
        }
        return mejor;
    }

    // I4: los MBR de todo el camino hasta la raiz deben cubrir la entrada
    void ajustarHaciaArriba(Nodo* n) {
        while (n != nullptr) {
            actualizarMBR(n);
            n = n->padre;
        }
    }

    void actualizarMBR(Nodo* n) {
        n->mbr.reset();
        if (n->esHoja) {
            for (const auto& e : n->entradas) n->mbr.estirar(e.x, e.y);
        } else {
            for (const Nodo* h : n->hijos) n->mbr.estirar(h->mbr);
        }
    }

    // OT1: primera vez en el nivel (y no raiz) => reinsertar; si no => split
    void overflowTreatment(Nodo* n) {
        int nivel = n->nivel < (int)nivelReinsertado_.size() ? n->nivel : 0;
        if (n != raiz_ && !nivelReinsertado_[nivel]) {
            nivelReinsertado_[nivel] = true;
            reinsertar(n);
        } else {
            split(n);
        }
    }

    // 4.3 ReInsert (RI1-RI4), variante close reinsert (la mejor del paper):
    // quitar las p=30% de M entradas mas lejanas al centro del MBR y
    // reinsertarlas empezando por la de distancia MINIMA.
    void reinsertar(Nodo* n) {
        double cx = (n->mbr.lo[0] + n->mbr.hi[0]) / 2.0;
        double cy = (n->mbr.lo[1] + n->mbr.hi[1]) / 2.0;

        int total = n->esHoja ? (int)n->entradas.size() : (int)n->hijos.size();
        int p = (int)(M_ * 0.3);                    // p = 30% de M (paper 4.3)
        if (p < 1) p = 1;
        if (p > total - 1) p = total - 1;
        if (p < 1) return;

        std::vector<std::pair<double, int>> dist(total);   // {distancia^2, indice}
        for (int i = 0; i < total; i++) {
            double x, y;
            if (n->esHoja) { x = n->entradas[i].x; y = n->entradas[i].y; }
            else {
                x = (n->hijos[i]->mbr.lo[0] + n->hijos[i]->mbr.hi[0]) / 2.0;
                y = (n->hijos[i]->mbr.lo[1] + n->hijos[i]->mbr.hi[1]) / 2.0;
            }
            double dx = x - cx, dy = y - cy;
            dist[i] = {dx * dx + dy * dy, i};
        }
        // RI2: orden decreciente de distancia
        std::sort(dist.begin(), dist.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        // RI3: quitar las p mas lejanas
        std::vector<bool> quitar(total, false);
        for (int c = 0; c < p; c++) quitar[dist[c].second] = true;

        std::vector<Resultado> entradasQuitadas;
        std::vector<Nodo*> subarbolesQuitados;
        // guardadas en orden de distancia CRECIENTE (close reinsert)
        for (int c = p - 1; c >= 0; c--) {
            int i = dist[c].second;
            if (n->esHoja) entradasQuitadas.push_back(n->entradas[i]);
            else           subarbolesQuitados.push_back(n->hijos[i]);
        }

        if (n->esHoja) {
            std::vector<Resultado> resto;
            resto.reserve(total - p);
            for (int i = 0; i < total; i++) if (!quitar[i]) resto.push_back(n->entradas[i]);
            n->entradas = std::move(resto);
            tocar(n);
        } else {
            std::vector<Nodo*> resto;
            resto.reserve(total - p);
            for (int i = 0; i < total; i++) if (!quitar[i]) resto.push_back(n->hijos[i]);
            n->hijos = std::move(resto);
        }
        ajustarHaciaArriba(n);

        // RI4: reinsertar empezando por la distancia minima
        for (const auto& e : entradasQuitadas) insertarEntrada(e);
        for (Nodo* s : subarbolesQuitados) insertarSubarbol(s);
    }

    // Resultado de la seleccion de split (4.2): permutacion y tamano de grupo1
    struct SeleccionSplit {
        int eje;
        std::vector<int> orden;
        int tamGrupo1;
    };

    // S1 ChooseSplitAxis (suma S de margenes de todas las distribuciones,
    // ambos ordenes lower/upper) + S2 ChooseSplitIndex (minimo overlap,
    // empate por area). MBRs prefijo/sufijo => O(E) por orden.
    SeleccionSplit elegirSplit(const std::vector<Caja>& ent) const {
        int E = (int)ent.size();
        int m = m_;
        if (m > E / 2) m = E / 2;   // salvaguarda: >= 1 distribucion
        if (m < 1) m = 1;
        int numDistribuciones = E - 2 * m + 1;

        std::vector<int> ordenes[2][2];  // [eje][clave: 0=inferior, 1=superior]
        double S[2] = {0.0, 0.0};

        auto prefSuf = [&](const std::vector<int>& orden, std::vector<Caja>& pref, std::vector<Caja>& suf) {
            pref.resize(E); suf.resize(E);
            pref[0] = ent[orden[0]];
            for (int i = 1; i < E; i++) { pref[i] = pref[i - 1]; pref[i].estirar(ent[orden[i]]); }
            suf[E - 1] = ent[orden[E - 1]];
            for (int i = E - 2; i >= 0; i--) { suf[i] = suf[i + 1]; suf[i].estirar(ent[orden[i]]); }
        };

        for (int eje = 0; eje < 2; eje++) {
            for (int clave = 0; clave < 2; clave++) {
                std::vector<int>& orden = ordenes[eje][clave];
                orden.resize(E);
                for (int i = 0; i < E; i++) orden[i] = i;
                std::sort(orden.begin(), orden.end(), [&](int a, int b) {
                    double ka = (clave == 0) ? ent[a].lo[eje] : ent[a].hi[eje];
                    double kb = (clave == 0) ? ent[b].lo[eje] : ent[b].hi[eje];
                    return ka < kb;
                });
                std::vector<Caja> pref, suf;
                prefSuf(orden, pref, suf);
                for (int k = 0; k < numDistribuciones; k++) {
                    int g1 = m + k;
                    S[eje] += pref[g1 - 1].margen() + suf[g1].margen();
                }
            }
        }

        SeleccionSplit sel;
        sel.eje = (S[0] <= S[1]) ? 0 : 1;   // CSA2
        sel.tamGrupo1 = m;

        double mejorOverlap = std::numeric_limits<double>::infinity();
        double mejorArea = std::numeric_limits<double>::infinity();
        for (int clave = 0; clave < 2; clave++) {
            const std::vector<int>& orden = ordenes[sel.eje][clave];
            std::vector<Caja> pref, suf;
            prefSuf(orden, pref, suf);
            for (int k = 0; k < numDistribuciones; k++) {
                int g1 = m + k;
                double ov = pref[g1 - 1].overlap(suf[g1]);
                double ar = pref[g1 - 1].area() + suf[g1].area();
                if (ov < mejorOverlap || (ov == mejorOverlap && ar < mejorArea)) {
                    mejorOverlap = ov;
                    mejorArea = ar;
                    sel.orden = orden;
                    sel.tamGrupo1 = g1;
                }
            }
        }
        return sel;
    }

    // 4.2 Split (S1-S3) + I3 (propagacion). Generico: hojas e internos.
    void split(Nodo* n) {
        std::vector<Caja> entradas;
        if (n->esHoja) {
            entradas.reserve(n->entradas.size());
            for (const auto& e : n->entradas) entradas.push_back(Caja(e.x, e.y, e.x, e.y));
        } else {
            entradas.reserve(n->hijos.size());
            for (const Nodo* h : n->hijos) entradas.push_back(h->mbr);
        }
        if (entradas.size() < 2) return;

        SeleccionSplit sel = elegirSplit(entradas);

        Nodo* nuevo = new Nodo(n->esHoja);
        nuevo->nivel = n->nivel;

        if (n->esHoja) {
            std::vector<Resultado> g1, g2;
            g1.reserve(sel.tamGrupo1);
            g2.reserve(entradas.size() - sel.tamGrupo1);
            for (int i = 0; i < (int)sel.orden.size(); i++)
                (i < sel.tamGrupo1 ? g1 : g2).push_back(n->entradas[sel.orden[i]]);
            n->entradas = std::move(g1);
            nuevo->entradas = std::move(g2);
            tocar(n);
            tocar(nuevo);
        } else {
            std::vector<Nodo*> g1, g2;
            g1.reserve(sel.tamGrupo1);
            g2.reserve(entradas.size() - sel.tamGrupo1);
            for (int i = 0; i < (int)sel.orden.size(); i++)
                (i < sel.tamGrupo1 ? g1 : g2).push_back(n->hijos[sel.orden[i]]);
            n->hijos = std::move(g1);
            nuevo->hijos = std::move(g2);
            for (Nodo* h : nuevo->hijos) h->padre = nuevo;
        }

        actualizarMBR(n);
        actualizarMBR(nuevo);

        // I3: propagar el split hacia arriba
        if (n == raiz_) {
            Nodo* nuevaRaiz = new Nodo(false);
            nuevaRaiz->nivel = n->nivel + 1;
            nuevaRaiz->hijos.push_back(n);
            nuevaRaiz->hijos.push_back(nuevo);
            n->padre = nuevaRaiz;
            nuevo->padre = nuevaRaiz;
            actualizarMBR(nuevaRaiz);
            raiz_ = nuevaRaiz;
        } else {
            Nodo* padre = n->padre;
            nuevo->padre = padre;
            padre->hijos.push_back(nuevo);
            ajustarHaciaArriba(padre);
            if ((int)padre->hijos.size() > M_)
                overflowTreatment(padre);   // el overflow puede cascadear
        }
    }

    void rangoRec(const Nodo* n, const Caja& bbox, std::vector<Resultado>& res) const {
        if (n == nullptr || !n->mbr.interseca(bbox)) return;
        if (n->esHoja) {
            for (const auto& e : n->entradas)
                if (bbox.contiene(e.x, e.y)) res.push_back(e);
        } else {
            for (const Nodo* h : n->hijos) rangoRec(h, bbox, res);
        }
    }
    void recorrerRec(const Nodo* n, const std::function<void(const Resultado&)>& v) const {
        if (n == nullptr) return;
        if (n->esHoja) { for (const auto& e : n->entradas) v(e); }
        else for (const Nodo* h : n->hijos) recorrerRec(h, v);
    }
    void inspeccionarRec(const Nodo* n, int prof, bool esRaiz,
                         const std::function<void(bool, int, int, const Caja&, size_t, size_t, bool)>& f) const {
        if (n == nullptr) return;
        f(n->esHoja, n->nivel, prof, n->mbr, n->entradas.size(), n->hijos.size(), esRaiz);
        for (const Nodo* h : n->hijos) inspeccionarRec(h, prof + 1, false, f);
    }
};
