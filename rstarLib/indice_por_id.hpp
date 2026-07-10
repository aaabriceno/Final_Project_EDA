#pragma once
// Indice opcional: id externo -> posicion en la arena. Ver DISENO.md seccion 2.
// Su unica funcion es el punto de entrada por id (O(1)); no almacena datos.
#include "rstartree.hpp"
#include <unordered_map>
#include <optional>

template <typename T, typename Id>
class IndicePorId {
public:
    IndicePorId(const RStarTree2D<T>& arbol, std::function<Id(const T&)> idDe)
        : idDe_(std::move(idDe)) {
        arbol.recorrer([&](const typename RStarTree2D<T>::Resultado& r) {
            mapa_[idDe_(arbol.dato(r.idx))] = r.idx;
        });
    }
    std::optional<uint32_t> buscar(const Id& id) const {
        auto it = mapa_.find(id);
        if (it == mapa_.end()) return std::nullopt;
        return it->second;
    }
    // Para mantener el indice al insertar despues de construirlo
    void agregar(const T& dato, uint32_t idx) { mapa_[idDe_(dato)] = idx; }
    size_t tamano() const { return mapa_.size(); }

private:
    std::function<Id(const T&)> idDe_;
    std::unordered_map<Id, uint32_t> mapa_;
};
