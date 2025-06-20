#include "GeoCluster.cpp"
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
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
        cerr << "Error al abrir el archivo: " << archivo << endl;
        return puntos;  // Si no se puede abrir, devolver un vector vacío
    }

    // Configurar precisión para los mensajes de debug
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(10);

    string linea;
    int numeroLinea = 0;
    bool primeraLinea = true;
    
    // Leer cada línea del archivo
    while (getline(archivoCSV, linea)) {
        numeroLinea++;
        
        // Saltar líneas vacías
        if (linea.empty()) {
            continue;
        }
        
        // Saltar la primera línea (encabezado)
        if (primeraLinea) {
            cout << "Saltando línea de encabezado: " << linea << endl;
            primeraLinea = false;
            continue;
        }
        
        stringstream ss(linea);
        string campo;

        int id;
        double lat, lon;
        vector<double> atributos;

        try {
            // Leer tripID (ID del punto)
            if (!getline(ss, campo, ',')) {
                cerr << "Error en línea " << numeroLinea << ": No se pudo leer el tripID" << endl;
                continue;
            }
            if (campo.empty()) {
                cerr << "Error en línea " << numeroLinea << ": tripID vacío" << endl;
                continue;
            }
            id = stoi(campo);

            // Leer pickup_latitude (latitud)
            if (!getline(ss, campo, ',')) {
                cerr << "Error en línea " << numeroLinea << ": No se pudo leer la latitud" << endl;
                continue;
            }
            if (campo.empty()) {
                cerr << "Error en línea " << numeroLinea << ": Latitud vacía" << endl;
                continue;
            }
            lat = stod(campo);

            // Leer pickup_longitude (longitud)
            if (!getline(ss, campo, ',')) {
                cerr << "Error en línea " << numeroLinea << ": No se pudo leer la longitud" << endl;
                continue;
            }
            if (campo.empty()) {
                cerr << "Error en línea " << numeroLinea << ": Longitud vacía" << endl;
                continue;
            }
            lon = stod(campo);

            // Leer los atributos restantes (passenger_count, trip_distance, payment_type, etc.)
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) {
                    atributos.push_back(stod(campo));
                }
            }

            // Crear el punto y agregarlo al vector de puntos
            puntos.push_back(Punto(id, lat, lon, atributos));
            cout << "Punto leído: ID=" << id << ", Lat=" << lat << ", Lon=" << lon << ", Atributos=" << atributos.size() << endl;
            
        } catch (const std::invalid_argument& e) {
            cerr << "Error en línea " << numeroLinea << ": No se pudo convertir un valor numérico: " << e.what() << endl;
            cerr << "Línea: " << linea << endl;
            continue;
        } catch (const std::out_of_range& e) {
            cerr << "Error en línea " << numeroLinea << ": Valor fuera de rango: " << e.what() << endl;
            cerr << "Línea: " << linea << endl;
            continue;
        }
    }

    archivoCSV.close();  // Cerrar el archivo
    cout << "Se leyeron " << puntos.size() << " puntos del archivo " << archivo << endl;
    return puntos;
}

bool archivoExiste(const std::string& archivo) {
    std::ifstream file(archivo);
    bool existe = file.good();
    file.close();
    
    if (!existe) {
        std::cerr << "DEBUG: No se puede abrir el archivo: " << archivo << std::endl;
    } else {
        std::cout << "DEBUG: Archivo encontrado: " << archivo << std::endl;
    }
    
    return existe;
}

// Estructura para el formato binario
struct PuntoBinario {
    int id;
    double latitud;
    double longitud;
    double atributos[12];  // Asumiendo 12 atributos como máximo
    int num_atributos;
};

// Función para convertir CSV a binario
void convertirCSVaBinario(const string& archivoCSV, const string& archivoBinario) {
    cout << "Convirtiendo " << archivoCSV << " a formato binario..." << endl;
    
    vector<Punto> puntos = leerCSV(archivoCSV);
    
    ofstream archivoBin(archivoBinario, ios::binary);
    if (!archivoBin.is_open()) {
        cerr << "Error al crear archivo binario: " << archivoBinario << endl;
        return;
    }
    
    // Escribir número total de puntos
    int numPuntos = puntos.size();
    archivoBin.write(reinterpret_cast<const char*>(&numPuntos), sizeof(int));
    
    // Escribir cada punto
    for (const auto& punto : puntos) {
        PuntoBinario puntoBin;
        puntoBin.id = punto.id;
        puntoBin.latitud = punto.latitud;
        puntoBin.longitud = punto.longitud;
        puntoBin.num_atributos = punto.atributos.size();
        
        // Copiar atributos
        for (int i = 0; i < 12; i++) {
            if (i < punto.atributos.size()) {
                puntoBin.atributos[i] = punto.atributos[i];
            } else {
                puntoBin.atributos[i] = 0.0;
            }
        }
        
        archivoBin.write(reinterpret_cast<const char*>(&puntoBin), sizeof(PuntoBinario));
    }
    
    archivoBin.close();
    cout << "Conversión completada. Archivo binario: " << archivoBinario << endl;
    cout << "Tamaño original: " << puntos.size() << " puntos" << endl;
}

// Función para leer archivo binario
vector<Punto> leerBinario(const string& archivoBinario) {
    vector<Punto> puntos;
    ifstream archivoBin(archivoBinario, ios::binary);
    
    if (!archivoBin.is_open()) {
        cerr << "Error al abrir archivo binario: " << archivoBinario << endl;
        return puntos;
    }
    
    // Leer número total de puntos
    int numPuntos;
    archivoBin.read(reinterpret_cast<char*>(&numPuntos), sizeof(int));
    cout << "Leyendo " << numPuntos << " puntos del archivo binario..." << endl;
    
    // Leer cada punto
    for (int i = 0; i < numPuntos; i++) {
        PuntoBinario puntoBin;
        archivoBin.read(reinterpret_cast<char*>(&puntoBin), sizeof(PuntoBinario));
        
        // Convertir a estructura Punto
        vector<double> atributos;
        for (int j = 0; j < puntoBin.num_atributos; j++) {
            atributos.push_back(puntoBin.atributos[j]);
        }
        
        puntos.push_back(Punto(puntoBin.id, puntoBin.latitud, puntoBin.longitud, atributos));
        
        if (i < 5) {  // Mostrar solo los primeros 5 puntos como ejemplo
            cout << "Punto " << i+1 << ": ID=" << puntoBin.id 
                 << ", Lat=" << puntoBin.latitud << ", Lon=" << puntoBin.longitud 
                 << ", Atributos=" << puntoBin.num_atributos << endl;
        }
    }
    
    archivoBin.close();
    cout << "Lectura completada. Total de puntos: " << puntos.size() << endl;
    return puntos;
}

int main() {
    // Configurar la precisión para mostrar más decimales
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(10);
    
    // Crear un objeto de la clase GeoCluster
    GeoCluster geoCluster;

    // Configuración de archivos
    string archivoCSV = "C:/Users/anthony/Final_Project_EDA/Database/puntos500k.csv";
    string archivoBinario = "C:/Users/anthony/Final_Project_EDA/Database/puntos500k.bin";
    
    vector<Punto> puntos;
    
    // Verificar si existe el archivo binario
    if (archivoExiste(archivoBinario)) {
        cout << "Archivo binario encontrado. Leyendo desde binario..." << endl;
        puntos = leerBinario(archivoBinario);
    } else {
        cout << "Archivo binario no encontrado. Verificando archivo CSV..." << endl;
        
        // Intentar diferentes rutas posibles para el CSV
        vector<string> rutasPosibles = {
            archivoCSV,
            "Database/puntos500k.csv",
            "./Database/puntos500k.csv",
            "../Database/puntos500k.csv",
            "Database\\puntos500k.csv",
            "puntos500k.csv"
        };
        
        string rutaCSV = "";
        
        cout << "Buscando archivo puntos500k.csv..." << endl;
        
        for (const string& ruta : rutasPosibles) {
            cout << "Probando: " << ruta << " - ";
            if (archivoExiste(ruta)) {
                cout << "¡ENCONTRADO!" << endl;
                rutaCSV = ruta;
                break;
            } else {
                cout << "No encontrado" << endl;
            }
        }
        
        if (rutaCSV.empty()) {
            std::cerr << "ERROR: No se pudo encontrar el archivo puntos500k.csv en ninguna ruta" << std::endl;
            std::cerr << "Asegúrate de que el archivo existe en la carpeta Database/" << std::endl;
            return 1;
        }
        
        cout << "Usando archivo CSV: " << rutaCSV << endl;
        puntos = leerCSV(rutaCSV);
        
        // Convertir a binario para uso futuro
        cout << "\n¿Deseas convertir el CSV a formato binario para uso futuro? (s/n): ";
        char respuesta;
        cin >> respuesta;
        
        if (respuesta == 's' || respuesta == 'S') {
            convertirCSVaBinario(rutaCSV, archivoBinario);
        }
    }
    
    if (puntos.empty()) {
        cerr << "ERROR: No se pudieron leer puntos del archivo" << endl;
        return 1;
    }
    
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
    
    // Calcular y mostrar el MBR de todos los puntos
    MBR mbr = geoCluster.calcularMBR(puntos);
    
    // Imprimir el MBR
    cout << "\nMBR de los  puntos:" << endl;
    cout << "Latitud mínima: " << mbr.m_minp[0] << ", Longitud mínima: " << mbr.m_minp[1] << endl;
    cout << "Latitud máxima: " << mbr.m_maxp[0] << ", Longitud máxima: " << mbr.m_maxp[1] << endl;
    
    return 0;
}