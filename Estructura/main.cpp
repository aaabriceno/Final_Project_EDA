#include "GeoCluster.cpp"
#include <vector>
#include <algorithm>

bool compararPorLatitudYLongitud(const Punto& p1, const Punto& p2) {
    // Primero comparar por latitud
    if (p1.latitud == p2.latitud) {
        // Si las latitudes son iguales, comparar por longitud
        return p1.longitud < p2.longitud;
    }
    // Si las latitudes son diferentes, ordenar por latitud
    return p1.latitud < p2.latitud;
}

int main() {
    // Crear un objeto de la clase GeoCluster
    GeoCluster geoCluster;

    // Crear algunos puntos de ejemplo
    vector<Punto> puntos = {
    {1, 19.5, -70.5, {1.0, 2.0, 3.0}},
    {2, 20.0, -70.2, {1.5, 2.5, 3.5}},
    {3, 21.0, -71.0, {2.0, 3.0, 4.0}},
    {4, 22.0, -69.5, {3.0, 4.0, 5.0}},
    {5, 23.0, -68.0, {4.0, 5.0, 6.0}},
    {6, 20.5, -70.0, {1.2, 2.2, 3.2}},
    {7, 21.5, -69.5, {2.5, 3.5, 4.5}},
    {8, 19.5, -70.3, {2.2, 3.2, 4.2}},
    {9, 19.8, -70.4, {1.6, 2.6, 3.6}},
    {10, 21.2, -70.1, {2.1, 3.1, 4.1}},
    {11, 22.1, -69.6, {3.2, 4.2, 5.2}},
    {12, 22.8, -68.3, {4.1, 5.1, 6.1}},
    {13, 20.6, -69.9, {1.7, 2.7, 3.7}},
    {14, 21.7, -70.8, {2.4, 3.4, 4.4}},
    {15, 23.3, -67.9, {3.3, 4.3, 5.3}},
    {16, 20.1, -70.3, {1.8, 2.8, 3.8}},
    {17, 19.9, -69.4, {1.9, 2.9, 3.9}},
    {18, 20.8, -71.1, {2.6, 3.6, 4.6}},
    {19, 21.4, -69.7, {3.4, 4.4, 5.4}},
    {20, 22.5, -68.7, {4.2, 5.2, 6.2}},
    {21, 22.2, -70.0, {4.5, 5.5, 6.5}},
    {22, 21.0, -69.4, {2.7, 3.7, 4.7}},
    {23, 23.1, -67.8, {5.1, 6.1, 7.1}},
    {24, 19.7, -70.2, {1.0, 1.5, 2.0}},
    {25, 20.4, -70.6, {2.2, 3.2, 4.2}},
    {26, 21.9, -71.2, {2.8, 3.8, 4.8}},
    {27, 22.3, -69.1, {3.7, 4.7, 5.7}},
    {28, 20.9, -69.3, {1.3, 2.3, 3.3}},
    {29, 22.6, -68.4, {4.3, 5.3, 6.3}},
    {30, 23.5, -67.5, {5.2, 6.2, 7.2}},
    {31, 21.8, -70.3, {2.9, 3.9, 4.9}},
    {32, 20.3, -69.7, {2.0, 3.0, 4.0}},
    {33, 19.6, -70.9, {1.4, 2.4, 3.4}},
    {34, 21.3, -70.0, {2.5, 3.5, 4.5}},
    {35, 22.4, -68.6, {3.6, 4.6, 5.6}},
    {36, 23.6, -67.4, {4.4, 5.4, 6.4}},
    {37, 20.7, -69.0, {2.7, 3.7, 4.7}},
    {38, 19.8, -70.7, {1.5, 2.5, 3.5}},
    {39, 21.2, -70.5, {2.8, 3.8, 4.8}},
    {40, 23.7, -67.3, {5.3, 6.3, 7.3}},
    {41, 20.2, -69.8, {2.3, 3.3, 4.3}},
    {42, 21.4, -69.9, {2.7, 3.7, 4.7}},
    {43, 22.0, -68.2, {3.2, 4.2, 5.2}},
    {44, 19.9, -70.6, {1.9, 2.9, 3.9}},
    {45, 20.7, -70.0, {2.0, 3.0, 4.0}},
    {46, 21.3, -70.4, {2.1, 3.1, 4.1}},
    {47, 22.9, -68.3, {3.4, 4.4, 5.4}},
    {48, 20.8, -70.1, {2.5, 3.5, 4.5}},
    {49, 21.6, -69.8, {2.8, 3.8, 4.8}},
    {50, 22.1, -69.9, {3.1, 4.1, 5.1}},
    {51, 20.4, -70.3, {2.2, 3.2, 4.2}},
    {52, 23.0, -67.9, {4.3, 5.3, 6.3}},
    {53, 21.8, -69.5, {2.6, 3.6, 4.6}},
    {54, 23.1, -68.5, {5.4, 6.4, 7.4}},
    {55, 22.7, -69.2, {3.3, 4.3, 5.3}},
    {56, 20.3, -69.6, {2.1, 3.1, 4.1}},
    {57, 21.7, -69.7, {2.4, 3.4, 4.4}},
    {58, 22.8, -68.8, {3.6, 4.6, 5.6}},
    {59, 23.6, -67.2, {5.5, 6.5, 7.5}},
    {60, 21.9, -69.4, {3.7, 4.7, 5.7}},
    };


    /*
    sort(puntos.begin(), puntos.end(), compararPorLatitudYLongitud);
    cout << "Puntos ordenados por latitud y longitud:" << endl;
    for (const auto& punto : puntos) {
        cout << "ID: " << punto.id << ", Latitud: " << punto.latitud << ", Longitud: " << punto.longitud << endl;
    }
    */
    
    
    
    // Insertamos los puntos en el árbol
    for (const auto& punto : puntos) {
        geoCluster.insertarPunto(punto);
        cout << "Insertando punto: " << punto.latitud << " "<< punto.longitud << endl;
    }

    //lat_min,long_min, lat_max,long_max
    MBR rango(19.4,-72,23.5,-70);
    Nodo* raiz = geoCluster.getRaiz();

    vector<Punto> puntosEncontradosEnRango = geoCluster.buscarPuntosDentroInterseccion(rango,raiz);
    
    cout << "\nPuntos encontrados dentro de la intersección del MBR de búsqueda:" << endl;
    for (const auto& punto : puntosEncontradosEnRango) {
        cout << "ID: " << punto.id << ", Latitud: " << punto.latitud << ", Longitud: " << punto.longitud << endl;
    }
    
    
    // Calcular y mostrar el MBR de todos los puntos
    //MBR mbr = geoCluster.calcularMBR(puntos);
    /*
    // Imprimir el MBR
    cout << "\nMBR de los  puntos:" << endl;
    cout << "Latitud mínima: " << mbr.m_minp[0] << ", Longitud mínima: " << mbr.m_minp[1] << endl;
    cout << "Latitud máxima: " << mbr.m_maxp[0] << ", Longitud máxima: " << mbr.m_maxp[1] << endl;
    */
    
    return 0;
}