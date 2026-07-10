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
