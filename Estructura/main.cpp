#include "GeoCluster.cpp"
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

bool compararPorLatitudYLongitud(const Punto& p1, const Punto& p2) {
    // Primero comparar por latitud
    if (p1.latitud == p2.latitud) {
        // Si las latitudes son iguales, comparar por longitud
        return p1.longitud < p2.longitud;
    }
    // Si las latitudes son diferentes, ordenar por latitud
    return p1.latitud < p2.latitud;
}

vector<Punto> leerCSV(const string& archivo) {
    vector<Punto> puntos;
    ifstream archivoCSV(archivo);  // Abrir el archivo CSV

    if (!archivoCSV.is_open()) {
        cerr << "Error al abrir el archivo." << endl;
        return puntos;  // Si no se puede abrir, devolver un vector vacío
    }

    string linea;
    // Leer cada línea del archivo
    while (getline(archivoCSV, linea)) {
        stringstream ss(linea);
        string campo;

        int id;
        double lat, lon;
        vector<double> atributos;

        // Leer los datos de cada columna
        getline(ss, campo, ',');  // ID
        id = stoi(campo);

        getline(ss, campo, ',');  // Latitud
        lat = stod(campo);

        getline(ss, campo, ',');  // Longitud
        lon = stod(campo);

        // Leer los atributos (valores después de la longitud)
        while (getline(ss, campo, ',')) {
            atributos.push_back(stod(campo));
        }

        // Crear el punto y agregarlo al vector de puntos
        puntos.push_back(Punto(id, lat, lon, atributos));
    }

    archivoCSV.close();  // Cerrar el archivo
    return puntos;
}

bool archivoExiste(const std::string& archivo) {
    std::ifstream file(archivo);
    return file.good();  // Retorna true si el archivo se puede abrir
}


int main() {
    // Crear un objeto de la clase GeoCluster
    GeoCluster geoCluster;

    string rutaCSV = "puntos10.csv";  // Ruta correcta desde Estructura hacia Database

    if (!archivoExiste(rutaCSV)) {
        std::cerr << "No se pudo encontrar el archivo CSV en la ruta especificada: " << rutaCSV << std::endl;
        return 1;  // Salir del programa si el archivo no existe
    }
    vector<Punto> puntos = leerCSV(rutaCSV);
    // Crear algunos puntos de ejemplo
    /*
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
    {11, 62.85, -129.43, {2.99, 3.95, 4.89}},
    {12, 25.83, -78.57, {4.86, 4.90, 3.44}},
    {13, 0.87, 77.23, {1.40, 3.70, 4.95}},
    {14, -9.46, 135.51, {2.85, 3.21, 2.90}},
    {15, 24.92, -125.83, {3.58, 3.19, 4.61}},
    {16, 21.67, -74.35, {4.55, 2.50, 3.30}},
    {17, 60.41, -82.15, {1.90, 2.30, 3.10}},
    {18, -10.35, 160.56, {2.34, 4.32, 3.71}},
    {19, 5.79, -39.45, {4.12, 3.30, 2.51}},
    {20, -15.83, 135.56, {3.92, 4.11, 2.20}},
    {21, 30.92, -119.55, {4.62, 2.76, 3.59}},
    {22, 42.71, -73.76, {1.45, 3.34, 3.98}},
    {23, 21.11, -70.18, {3.59, 4.24, 2.87}},
    {24, -0.36, 178.63, {4.79, 3.82, 2.61}},
    {25, 41.56, -70.19, {3.20, 3.78, 1.92}},
    {26, -3.70, 173.42, {2.59, 2.87, 4.16}},
    {27, 59.02, -65.14, {4.12, 4.43, 1.85}},
    {28, 8.23, -85.51, {3.50, 4.10, 4.45}},
    {29, -2.56, 126.40, {4.65, 3.60, 2.95}},
    {30, 21.60, -70.33, {2.84, 4.03, 4.72}},
    {31, 60.37, -142.51, {4.88, 1.56, 3.91}},
    {32, -22.61, 144.62, {3.89, 2.32, 4.43}},
    {33, -31.77, 134.44, {4.60, 3.18, 2.49}},
    {34, 16.35, -117.56, {3.55, 3.83, 2.79}},
    {35, -9.61, 145.50, {1.91, 4.22, 3.71}},
    {36, 72.48, -89.33, {3.77, 4.61, 1.93}},
    {37, -5.84, 118.02, {4.10, 2.98, 3.47}},
    {38, 56.33, -73.74, {2.23, 3.65, 4.40}},
    {39, -17.49, 129.88, {3.79, 2.94, 1.89}},
    {40, 47.12, -89.32, {3.52, 2.63, 3.39}},
    {41, -9.91, 116.27, {3.21, 2.47, 4.61}},
    {42, 11.39, -68.53, {4.00, 3.88, 3.05}},
    {43, -44.92, 106.55, {2.51, 4.39, 3.12}},
    {44, -33.77, 178.33, {1.68, 3.72, 2.77}},
    {45, 29.99, -98.68, {3.32, 3.60, 1.88}},
    {46, -19.55, 113.11, {2.84, 4.02, 2.68}},
    {47, 6.94, -73.62, {3.77, 3.12, 4.07}},
    {48, 24.89, -138.61, {4.34, 3.90, 3.14}},
    {49, 51.43, -102.35, {3.61, 3.22, 4.33}},
    {50, 19.21, -64.49, {2.98, 3.68, 4.13}},
    {51, -40.43, 158.22, {3.80, 4.16, 2.71}},
    {52, 60.81, -77.56, {4.50, 3.71, 2.32}},
    {53, 32.91, -64.71, {1.98, 4.44, 3.09}},
    {54, -7.52, 125.38, {4.22, 3.98, 2.55}},
    {55, -8.92, 116.43, {3.74, 4.10, 3.65}},
    {56, 21.37, -129.43, {2.83, 4.20, 3.56}},
    {57, 58.72, -136.93, {3.44, 2.87, 4.26}},
    {58, 8.34, -97.82, {4.10, 3.89, 4.31}},
    {59, 30.42, -157.77, {2.93, 3.12, 3.27}},
    {60, 12.16, -43.29, {1.84, 4.50, 2.76}},
    {61, 68.12, -124.67, {3.25, 3.92, 4.48}},
    {62, 47.64, -66.29, {3.52, 2.80, 4.12}},
    {63, 12.89, -49.90, {2.68, 4.28, 1.75}},
    {64, 25.37, -88.17, {3.11, 2.97, 4.59}},
    {65, 18.95, -105.12, {4.38, 2.47, 3.62}},
    {66, 22.57, -117.43, {4.09, 3.71, 4.50}},
    {67, 48.06, -68.51, {2.47, 4.05, 3.88}},
    {68, 62.38, -120.33, {1.99, 2.74, 3.52}},
    {69, -5.74, 150.74, {2.38, 3.69, 2.96}},
    {70, 20.62, -118.53, {4.41, 2.79, 3.08}},
    {71, 16.14, -123.61, {3.63, 4.02, 2.93}},
    {72, 54.79, -135.49, {2.83, 3.55, 3.79}},
    {73, 35.91, -85.44, {4.51, 3.22, 2.48}},
    {74, 13.46, -80.72, {3.59, 2.90, 4.18}},
    {75, 44.88, -100.63, {3.23, 4.02, 3.61}},
    {76, 24.73, -123.99, {4.61, 2.71, 3.54}},
    {77, 51.91, -108.79, {2.97, 3.65, 4.43}},
    {78, -23.62, 141.36, {3.89, 4.06, 3.80}},
    {79, -50.53, 158.68, {3.27, 2.83, 2.91}},
    {80, -16.73, 173.71, {1.94, 3.47, 4.18}},
    {81, 7.35, -73.88, {4.56, 2.94, 3.61}},
    {82, -35.98, 137.13, {2.84, 4.27, 1.97}},
    {83, -13.72, 179.04, {3.01, 4.10, 3.88}},
    {84, 60.64, -138.95, {1.92, 2.90, 4.05}},
    {85, 11.23, -116.61, {4.19, 2.91, 3.32}},
    {86, 40.51, -115.94, {4.61, 3.18, 4.56}},
    {87, 28.01, -154.67, {2.47, 3.40, 2.87}},
    {88, 32.83, -75.26, {3.62, 4.29, 3.91}},
    {89, 54.56, -145.72, {4.38, 3.12, 2.83}},
    {90, 41.25, -163.31, {2.81, 3.64, 4.42}},
    {91, -22.67, 139.92, {3.91, 4.06, 2.61}},
    {92, -16.97, 115.44, {2.96, 4.32, 3.35}},
    {93, 15.82, -142.47, {4.18, 2.70, 3.95}},
    {94, 24.60, -151.52, {4.50, 2.61, 3.74}},
    {95, 3.37, -129.68, {1.91, 4.07, 3.69}},
    {96, 18.24, -67.72, {3.46, 3.56, 2.58}},
    {97, -30.52, 115.98, {3.90, 4.25, 4.02}},
    {98, -41.87, 120.62, {2.86, 3.42, 3.73}},
    {99, -4.20, 137.17, {2.95, 3.31, 4.13}},
    {100, 16.41, -110.46, {3.71, 4.15, 2.90}}
};
    */
    cout << "\n=== INSERCION DE PUNTOS ===" << endl;
    for (const auto& punto : puntos) {
        geoCluster.inserData(punto);
        cout << "Insertando punto " << punto.id << ": (" << punto.latitud << ", " << punto.longitud << ")" << endl;
        
        // Verificar puntos en el árbol después de cada inserción
        int puntosActuales = geoCluster.contarPuntosEnArbol();
        cout << "  Puntos en árbol después de insertar " << punto.id << ": " << puntosActuales << endl;
    }
    
    cout << "\n=== ESTRUCTURA DEL GEOCLUSTER-TREE ===" << endl;
    geoCluster.imprimirArbol();
    cout << "=====================================" << endl;
    
    // Verificar duplicados
    geoCluster.verificarDuplicados();
    
    int puntosEnArbol = geoCluster.contarPuntosEnArbol();
    cout << "\nTotal de puntos en el árbol: " << puntosEnArbol << " (esperados: " << puntos.size() << ")" << endl;
    
    if (puntosEnArbol != puntos.size()) {
        cout << "¡ADVERTENCIA! Faltan " << (puntos.size() - puntosEnArbol) << " puntos en el árbol" << endl;
    }
    /*
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
    */
    
    
    
    // Calcular y mostrar el MBR de todos los puntos
    MBR mbr = geoCluster.calcularMBR(puntos);
    
    // Imprimir el MBR
    cout << "\nMBR de los  puntos:" << endl;
    cout << "Latitud mínima: " << mbr.m_minp[0] << ", Longitud mínima: " << mbr.m_minp[1] << endl;
    cout << "Latitud máxima: " << mbr.m_maxp[0] << ", Longitud máxima: " << mbr.m_maxp[1] << endl;
    
    
    return 0;
}