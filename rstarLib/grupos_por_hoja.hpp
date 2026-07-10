#pragma once
// Precomputacion por hoja (DISENO.md secciones 2 y 4): dentro de cada hoja
// del arbol, los puntos se agrupan por etiqueta (cajones) y se calcula el
// centroide de caracteristicas por cajon. Se construye UNA vez tras la carga;
// las hojas mutadas despues se rearman perezosamente (por version).
// Los extractores (etiquetaDe, caracteristicasDe) permiten usar cualquier T.
#include "rstartree.hpp"
#include <map>
#include <unordered_map>

template <typename T, typename Etiqueta>   // Etiqueta necesita operator<
class GruposPorHoja {
public:
    using Res = typename RStarTree2D<T>::Resultado;

    struct Grupo {
        Etiqueta etiqueta{};
        std::vector<Res> miembros;
        std::vector<double> centroide;
    };

    GruposPorHoja(const RStarTree2D<T>& arbol,
                  std::function<Etiqueta(const T&)> etiquetaDe,
                  std::function<std::vector<double>(const T&)> caracteristicasDe)
        : arbol_(arbol), etiquetaDe_(std::move(etiquetaDe)),
          caracteristicasDe_(std::move(caracteristicasDe)) {}

    // Pasada completa post-carga: arma los cajones de todas las hojas
    void construir() {
        cache_.clear();
        arbol_.visitarHojas([&](const typename RStarTree2D<T>::HojaVista& h) {
            cache_[h.clave] = armarHoja(h);
        });
    }

    // Consulta 2 del proyecto: grupos (>= 2 miembros) dentro del bbox,
    // fusionados por etiqueta entre hojas. Devuelve indices a la arena.
    std::vector<std::vector<uint32_t>> gruposEnRango(const Caja& bbox) {
        std::map<Etiqueta, std::vector<uint32_t>> fusion;
        arbol_.visitarHojasEnRango(bbox, [&](const typename RStarTree2D<T>::HojaVista& h) {
            const CacheHoja& c = obtener(h);
            for (const Grupo& g : c.grupos)
                for (const Res& m : g.miembros)
                    if (bbox.contiene(m.x, m.y))
                        fusion[g.etiqueta].push_back(m.idx);
        });
        std::vector<std::vector<uint32_t>> res;
        for (auto& [et, v] : fusion)
            if (v.size() >= 2) res.push_back(std::move(v));
        return res;
    }

    // Consulta 1 del proyecto: los n mas parecidos al punto idxReferencia,
    // buscando SOLO dentro del bbox (el referente puede estar fuera de el).
    // Prioridad: (1) miembros de la misma etiqueta, ordenados por distancia
    // de caracteristicas al referente; (2) demas grupos ordenados por
    // distancia de su centroide, con sus miembros tambien ordenados.
    std::vector<uint32_t> nSimilares(const Caja& bbox, uint32_t idxReferencia, int n) {
        std::vector<uint32_t> res;
        if (n <= 0) return res;
        const T& refDato = arbol_.dato(idxReferencia);
        Etiqueta refEt = etiquetaDe_(refDato);
        std::vector<double> refCar = caracteristicasDe_(refDato);

        // grupos del bbox fusionados por etiqueta (excluyendo al referente)
        std::map<Etiqueta, std::vector<Res>> fusion;
        arbol_.visitarHojasEnRango(bbox, [&](const typename RStarTree2D<T>::HojaVista& h) {
            const CacheHoja& c = obtener(h);
            for (const Grupo& g : c.grupos)
                for (const Res& m : g.miembros)
                    if (bbox.contiene(m.x, m.y) && m.idx != idxReferencia)
                        fusion[g.etiqueta].push_back(m);
        });

        auto dist2Car = [](const std::vector<double>& a, const std::vector<double>& b) {
            double s = 0;
            size_t k = std::min(a.size(), b.size());
            for (size_t i = 0; i < k; i++) { double d = a[i] - b[i]; s += d * d; }
            return s;
        };
        auto agregarOrdenado = [&](std::vector<Res>& miembros) {
            std::sort(miembros.begin(), miembros.end(), [&](const Res& p, const Res& q) {
                return dist2Car(caracteristicasDe_(arbol_.dato(p.idx)), refCar) <
                       dist2Car(caracteristicasDe_(arbol_.dato(q.idx)), refCar);
            });
            for (const Res& m : miembros) {
                if ((int)res.size() >= n) return;
                res.push_back(m.idx);
            }
        };

        // prioridad 1: misma etiqueta que el referente
        auto mismo = fusion.find(refEt);
        if (mismo != fusion.end()) {
            agregarOrdenado(mismo->second);
            fusion.erase(mismo);
        }

        // prioridad 2: demas grupos por distancia de centroide al referente
        std::vector<std::pair<double, Etiqueta>> orden;
        for (auto& [et, miembros] : fusion) {
            std::vector<double> suma;
            for (const Res& m : miembros) {
                std::vector<double> v = caracteristicasDe_(arbol_.dato(m.idx));
                if (suma.empty()) suma.assign(v.size(), 0.0);
                for (size_t i = 0; i < v.size() && i < suma.size(); i++) suma[i] += v[i];
            }
            for (double& s : suma) s /= (double)miembros.size();
            orden.push_back({dist2Car(suma, refCar), et});
        }
        std::sort(orden.begin(), orden.end());
        for (auto& [d, et] : orden) {
            if ((int)res.size() >= n) break;
            agregarOrdenado(fusion[et]);
        }
        return res;
    }

private:
    struct CacheHoja {
        uint64_t version = 0;
        std::vector<Grupo> grupos;
    };

    CacheHoja armarHoja(const typename RStarTree2D<T>::HojaVista& h) {
        std::map<Etiqueta, Grupo> cajones;
        for (const Res& e : h.entradas) {
            const T& d = arbol_.dato(e.idx);
            Etiqueta et = etiquetaDe_(d);
            Grupo& g = cajones[et];
            g.etiqueta = et;
            g.miembros.push_back(e);
        }
        CacheHoja c;
        c.version = h.version;
        for (auto& [et, g] : cajones) {
            std::vector<double> suma;
            for (const Res& m : g.miembros) {
                std::vector<double> v = caracteristicasDe_(arbol_.dato(m.idx));
                if (suma.empty()) suma.assign(v.size(), 0.0);
                for (size_t i = 0; i < v.size() && i < suma.size(); i++) suma[i] += v[i];
            }
            for (double& s : suma) s /= (double)g.miembros.size();
            g.centroide = std::move(suma);
            c.grupos.push_back(std::move(g));
        }
        return c;
    }

    // Invalidacion perezosa: si la version de la hoja cambio, se rearma solo esa
    const CacheHoja& obtener(const typename RStarTree2D<T>::HojaVista& h) {
        auto it = cache_.find(h.clave);
        if (it == cache_.end() || it->second.version != h.version)
            it = cache_.insert_or_assign(h.clave, armarHoja(h)).first;
        return it->second;
    }

    const RStarTree2D<T>& arbol_;
    std::function<Etiqueta(const T&)> etiquetaDe_;
    std::function<std::vector<double>(const T&)> caracteristicasDe_;
    std::unordered_map<uintptr_t, CacheHoja> cache_;
};
