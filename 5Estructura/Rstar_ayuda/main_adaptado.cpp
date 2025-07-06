#include <iostream>
#include <iosfwd>
#include <string>
#include <ctime>
#include <fstream>
#include <sstream>
#include <chrono>

#include <stdio.h>
#include "rstartree_seguro.h"
#include "profile.h"

using namespace std;

// Estructura para puntos geográficos
struct PuntoGeo {
    int id;
    double latitud, longitud;
    int cluster_espacial, cluster_atributivo;
    vector<double> atributos;
    
    PuntoGeo(int _id, double _lat, double _lon, int _cluster_esp, int _cluster_attr, const vector<double>& _atributos)
        : id(_id), latitud(_lat), longitud(_lon), cluster_espacial(_cluster_esp), cluster_atributivo(_cluster_attr), atributos(_atributos) {}
};

// Función para crear bounding box desde coordenadas geográficas
RStarBoundingBox<2> createGeoBox(double lat, double lon, double radio = 0.001) {
    RStarBoundingBox<2> box;
    // Convertir coordenadas geográficas a enteros (escalar por 1000000 para precisión)
    int lat_int = static_cast<int>(lat * 1000000);
    int lon_int = static_cast<int>(lon * 1000000);
    int radio_int = static_cast<int>(radio * 1000000);
    
    box.min_edges[0] = lat_int - radio_int;
    box.min_edges[1] = lon_int - radio_int;
    box.max_edges[0] = lat_int + radio_int;
    box.max_edges[1] = lon_int + radio_int;
    return box;
}

// Función para cargar datos desde CSV
vector<PuntoGeo> cargarDatosCSV(const string& archivo, int maxPuntos = -1) {
    vector<PuntoGeo> puntos;
    ifstream archivoCSV(archivo);
    
    if (!archivoCSV.is_open()) {
        cerr << "Error al abrir el archivo: " << archivo << endl;
        return puntos;
    }
    
    string linea;
    bool primeraLinea = true;
    int puntosLeidos = 0;
    
    cout << "Cargando datos desde " << archivo << "..." << endl;
    
    while (getline(archivoCSV, linea) && (maxPuntos == -1 || puntosLeidos < maxPuntos)) {
        if (linea.empty() || primeraLinea) { 
            primeraLinea = false; 
            continue; 
        }
        
        stringstream ss(linea);
        string campo;
        
        try {
            // cluster_espacial
            if (!getline(ss, campo, ',')) continue;
            int cluster_espacial = stoi(campo);
            
            // cluster_atributivo
            if (!getline(ss, campo, ',')) continue;
            int cluster_atributivo = stoi(campo);
            
            // tripID
            if (!getline(ss, campo, ',')) continue;
            int id = stoi(campo);
            
            // pickup_latitude
            if (!getline(ss, campo, ',')) continue;
            double lat = stod(campo);
            
            // pickup_longitude
            if (!getline(ss, campo, ',')) continue;
            double lon = stod(campo);
            
            // atributos restantes
            vector<double> atributos;
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) {
                    atributos.push_back(stod(campo));
                }
            }
            
            // Limitar atributos a 8 para eficiencia
            if (atributos.size() > 8) {
                atributos.resize(8);
            }
            
            PuntoGeo punto(id, lat, lon, cluster_espacial, cluster_atributivo, atributos);
            puntos.push_back(punto);
            puntosLeidos++;
            
            if (puntosLeidos % 100000 == 0) {
                cout << "Progreso: " << puntosLeidos << " puntos cargados" << endl;
            }
            
        } catch (...) {
            continue;
        }
    }
    
    archivoCSV.close();
    cout << "Se cargaron " << puntos.size() << " puntos desde " << archivo << endl;
    return puntos;
}

// Función para medir tiempo
template<typename Func>
double medirTiempo(Func func) {
    auto inicio = chrono::high_resolution_clock::now();
    func();
    auto fin = chrono::high_resolution_clock::now();
    return chrono::duration_cast<chrono::milliseconds>(fin - inicio).count();
}

int main() {
    cout << "=== R-STAR TREE ADAPTADO PARA 1.5M DE DATOS ===" << endl;
    
    // Parámetros optimizados para grandes volúmenes
    const int MIN_CHILD_ITEMS = 50;
    const int MAX_CHILD_ITEMS = 100;
    
    cout << "Parámetros optimizados:" << endl;
    cout << "MIN_CHILD_ITEMS: " << MIN_CHILD_ITEMS << endl;
    cout << "MAX_CHILD_ITEMS: " << MAX_CHILD_ITEMS << endl;
    cout << endl;
    
    // Opciones de prueba
    cout << "Opciones de prueba:" << endl;
    cout << "1. Probar con 100,000 puntos" << endl;
    cout << "2. Probar con 500,000 puntos" << endl;
    cout << "3. Probar con 1,500,000 puntos" << endl;
    cout << "4. Cargar desde archivo CSV" << endl;
    cout << "Selecciona una opción: ";
    
    int opcion;
    cin >> opcion;
    
    vector<PuntoGeo> datos;
    int cantidad_puntos = 0;
    
    switch (opcion) {
        case 1:
            cantidad_puntos = 100000;
            cout << "Generando " << cantidad_puntos << " puntos aleatorios..." << endl;
            break;
        case 2:
            cantidad_puntos = 500000;
            cout << "Generando " << cantidad_puntos << " puntos aleatorios..." << endl;
            break;
        case 3:
            cantidad_puntos = 1500000;
            cout << "Generando " << cantidad_puntos << " puntos aleatorios..." << endl;
            break;
        case 4: {
            string archivo;
            cout << "Ingresa el nombre del archivo CSV: ";
            cin >> archivo;
            datos = cargarDatosCSV(archivo, 1500000);
            break;
        }
        default:
            cout << "Opción no válida, usando 100,000 puntos..." << endl;
            cantidad_puntos = 100000;
            break;
    }
    
    // Si no se cargaron datos desde CSV, generar aleatorios
    if (datos.empty() && cantidad_puntos > 0) {
        srand(time(0));
        for (int i = 0; i < cantidad_puntos; i++) {
            // Coordenadas de NYC
            double lat = 40.7 + (rand() % 1000) / 10000.0;  // 40.7-40.8
            double lon = -74.0 + (rand() % 1000) / 10000.0; // -74.0 a -73.9
            int cluster_esp = rand() % 10;
            int cluster_attr = rand() % 5;
            
            vector<double> atributos;
            for (int j = 0; j < 8; j++) {
                atributos.push_back(rand() % 1000 / 1000.0);
            }
            
            PuntoGeo punto(i, lat, lon, cluster_esp, cluster_attr, atributos);
            datos.push_back(punto);
        }
    }
    
    if (datos.empty()) {
        cout << "No se pudieron cargar datos. Saliendo..." << endl;
        return 1;
    }
    
    cout << "\n=== INSERTANDO " << datos.size() << " PUNTOS EN R-STAR TREE ===" << endl;
    
    // Crear R-Star Tree optimizado
    RStarTree<int, 2, MIN_CHILD_ITEMS, MAX_CHILD_ITEMS> tree;
    
    // Medir tiempo de inserción
    double tiempoInsercion = medirTiempo([&]() {
        for (const auto& punto : datos) {
            auto box = createGeoBox(punto.latitud, punto.longitud, 0.001);
            tree.insert(punto.id, box);
        }
    });
    
    cout << "Tiempo de inserción: " << tiempoInsercion << " ms" << endl;
    cout << "Tiempo promedio por punto: " << tiempoInsercion / datos.size() << " ms" << endl;
    cout << "Rendimiento: " << (datos.size() / (tiempoInsercion / 1000.0)) << " puntos/segundo" << endl;
    
    // Prueba de búsqueda en rango
    cout << "\n=== PRUEBA DE BÚSQUEDA EN RANGO ===" << endl;
    
    // Crear rango de búsqueda (centro de NYC)
    auto rango_busqueda = createGeoBox(40.75, -73.95, 0.01);
    
    double tiempoBusqueda = medirTiempo([&]() {
        auto resultados = tree.find_objects_in_area(rango_busqueda);
        cout << "Puntos encontrados en rango: " << resultados.size() << endl;
    });
    
    cout << "Tiempo de búsqueda en rango: " << tiempoBusqueda << " ms" << endl;
    
    // Prueba de múltiples búsquedas
    cout << "\n=== PRUEBA DE MÚLTIPLES BÚSQUEDAS ===" << endl;
    
    double tiempoMultiplesBusquedas = medirTiempo([&]() {
        for (int i = 0; i < 1000; i++) {
            double lat = 40.7 + (rand() % 1000) / 10000.0;
            double lon = -74.0 + (rand() % 1000) / 10000.0;
            auto rango = createGeoBox(lat, lon, 0.005);
            auto resultados = tree.find_objects_in_area(rango);
        }
    });
    
    cout << "Tiempo para 1000 búsquedas: " << tiempoMultiplesBusquedas << " ms" << endl;
    cout << "Tiempo promedio por búsqueda: " << tiempoMultiplesBusquedas / 1000.0 << " ms" << endl;
    
    // Prueba de escritura/lectura binaria
    cout << "\n=== PRUEBA DE ESCRITURA/LECTURA BINARIA ===" << endl;
    
    double tiempoEscritura = medirTiempo([&]() {
        fstream archivo("arbol_rtree.bin", ios::out | ios::binary);
        tree.write_in_binary_file(archivo);
        archivo.close();
    });
    
    cout << "Tiempo de escritura binaria: " << tiempoEscritura << " ms" << endl;
    
    // Crear nuevo árbol y leer desde archivo
    RStarTree<int, 2, MIN_CHILD_ITEMS, MAX_CHILD_ITEMS> tree_leido;
    
    double tiempoLectura = medirTiempo([&]() {
        fstream archivo("arbol_rtree.bin", ios::in | ios::binary);
        tree_leido.read_from_binary_file(archivo);
        archivo.close();
    });
    
    cout << "Tiempo de lectura binaria: " << tiempoLectura << " ms" << endl;
    
    // Verificar que los árboles son iguales
    auto resultados_original = tree.find_objects_in_area(rango_busqueda);
    auto resultados_leido = tree_leido.find_objects_in_area(rango_busqueda);
    
    cout << "Verificación: " << (resultados_original.size() == resultados_leido.size() ? "✅ Correcto" : "❌ Error") << endl;
    
    cout << "\n=== RESUMEN DE RENDIMIENTO ===" << endl;
    cout << "Total de puntos procesados: " << datos.size() << endl;
    cout << "Tiempo total de inserción: " << tiempoInsercion << " ms" << endl;
    cout << "Tiempo total de búsquedas: " << tiempoBusqueda + tiempoMultiplesBusquedas << " ms" << endl;
    cout << "Tiempo total de I/O: " << tiempoEscritura + tiempoLectura << " ms" << endl;
    cout << "Rendimiento general: " << (datos.size() / ((tiempoInsercion + tiempoBusqueda) / 1000.0)) << " puntos/segundo" << endl;
    
    cout << "\n🎯 ¡R-Star Tree optimizado listo para 1.5M+ puntos!" << endl;
    return 0;
} 