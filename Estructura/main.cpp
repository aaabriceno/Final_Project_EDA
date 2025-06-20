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
    };
    cout << "\n=== INSERCIÓN DE PUNTOS ===" << endl;
    for (const auto& punto : puntos) {
        geoCluster.inserData(punto);
        cout << "Insertando punto " << punto.id << ": (" << punto.latitud << ", " << punto.longitud << ")" << endl;
        
        // Verificar puntos en el árbol después de cada inserción
        int puntosActuales = geoCluster.contarPuntosEnArbol();
        cout << "  Puntos en árbol después de insertar " << punto.id << ": " << puntosActuales << endl;
    }
    
    cout << "\n=== ESTRUCTURA DEL ÁRBOL R*-TREE ===" << endl;
    geoCluster.imprimirArbol();
    cout << "=====================================" << endl;
    
    // Verificar duplicados
    geoCluster.verificarDuplicados();
    
    int puntosEnArbol = geoCluster.contarPuntosEnArbol();
    cout << "\nTotal de puntos en el árbol: " << puntosEnArbol << " (esperados: " << puntos.size() << ")" << endl;
    
    if (puntosEnArbol != puntos.size()) {
        cout << "¡ADVERTENCIA! Faltan " << (puntos.size() - puntosEnArbol) << " puntos en el árbol" << endl;
    }
    
    MBR rango(19,-72,20.5,-69);
    Nodo* raiz = geoCluster.getRaiz();

    cout << "\nBuscando puntos en rango: (" << rango.m_minp[0] << "," << rango.m_minp[1] << ") a (" 
         << rango.m_maxp[0] << "," << rango.m_maxp[1] << ")" << endl;

    vector<Punto> puntosEncontradosEnRango = geoCluster.buscarPuntosDentroInterseccion(rango, raiz);
    
    cout << "\nPuntos encontrados dentro de la intersección del MBR de búsqueda: " 
         << puntosEncontradosEnRango.size() << " puntos" << endl;
    for (const auto& punto : puntosEncontradosEnRango) {
        cout << "ID: " << punto.id << ", Latitud: " << punto.latitud << ", Longitud: " << punto.longitud << endl;
    }
    
    
    // Calcular y mostrar el MBR de todos los puntos
    MBR mbr = geoCluster.calcularMBR(puntos);
    
    // Imprimir el MBR
    cout << "\nMBR de los  puntos:" << endl;
    cout << "Latitud mínima: " << mbr.m_minp[0] << ", Longitud mínima: " << mbr.m_minp[1] << endl;
    cout << "Latitud máxima: " << mbr.m_maxp[0] << ", Longitud máxima: " << mbr.m_maxp[1] << endl;
    
    
    return 0;
}