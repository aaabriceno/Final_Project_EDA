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
        // Leer ID
        int id;
        archivoBin.read(reinterpret_cast<char*>(&id), sizeof(int));
        
        // Leer latitud y longitud
        double lat, lon;
        archivoBin.read(reinterpret_cast<char*>(&lat), sizeof(double));
        archivoBin.read(reinterpret_cast<char*>(&lon), sizeof(double));
        
        // Leer número de atributos
        int num_atributos;
        archivoBin.read(reinterpret_cast<char*>(&num_atributos), sizeof(int));
        
        // Leer atributos (máximo 12)
        vector<double> atributos;
        for (int j = 0; j < 12; j++) {
            double atributo;
            archivoBin.read(reinterpret_cast<char*>(&atributo), sizeof(double));
            if (j < num_atributos) {
                atributos.push_back(atributo);
            }
        }
        
        puntos.push_back(Punto(id, lat, lon, atributos));
        
        if (i < 5) {  // Mostrar solo los primeros 5 puntos como ejemplo
            cout << "Punto " << i+1 << ": ID=" << id 
                 << ", Lat=" << lat << ", Lon=" << lon 
                 << ", Atributos=" << num_atributos << endl;
        }
        
        if (i % 10000 == 0 && i > 0) {
            cout << "Procesados " << i << " puntos..." << endl;
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
    string archivoCSV = "C:/Users/anthony/Final_Project_EDA/Database/puntos10k.csv";
    string archivoBinario = "C:/Users/anthony/Final_Project_EDA/Database/puntos10kk.bin";
    
    vector<Punto> puntos;
    
    cout << "==========================================" << endl;
    cout << "        GEOCLUSTER-TREE LOADER" << endl;
    cout << "==========================================" << endl;
    cout << endl;
    
    // Verificar si existe el archivo binario
    bool existeBinario = archivoExiste(archivoBinario);
    bool existeCSV = false;
    
    // Verificar si existe el archivo CSV
    vector<string> rutasPosibles = {
        archivoCSV,
        "Database/puntos10k.csv",
        "./Database/puntos10k.csv",
        "../Database/puntos10k.csv",
        "Database\\puntos10k.csv",
        "puntos10k.csv"
    };
    
    string rutaCSV = "";
    for (const string& ruta : rutasPosibles) {
        if (archivoExiste(ruta)) {
            rutaCSV = ruta;
            existeCSV = true;
            break;
        }
    }
    
    // Mostrar opciones disponibles
    cout << "Archivos disponibles:" << endl;
    if (existeBinario) {
        cout << "✓ Archivo binario encontrado: " << archivoBinario << endl;
    } else {
        cout << "✗ Archivo binario no encontrado" << endl;
    }
    
    if (existeCSV) {
        cout << "✓ Archivo CSV encontrado: " << rutaCSV << endl;
    } else {
        cout << "✗ Archivo CSV no encontrado" << endl;
    }
    cout << endl;
    
    // Menú de opciones
    int opcion = 0;
    if (existeBinario && existeCSV) {
        cout << "Opciones disponibles:" << endl;
        cout << "1. Leer archivo binario (más rápido)" << endl;
        cout << "2. Leer archivo CSV directamente" << endl;
        cout << "3. Convertir CSV a binario y luego leer binario" << endl;
        cout << "4. Salir" << endl;
        cout << endl;
        cout << "Selecciona una opción (1-4): ";
        cin >> opcion;
    } else if (existeBinario) {
        cout << "Solo archivo binario disponible. Leyendo binario..." << endl;
        opcion = 1;
    } else if (existeCSV) {
        cout << "Solo archivo CSV disponible. Leyendo CSV..." << endl;
        opcion = 2;
    } else {
        cout << "ERROR: No se encontraron archivos de datos" << endl;
        cout << "Asegúrate de que existe puntos500k.csv en la carpeta Database/" << endl;
        return 1;
    }
    
    // Procesar la opción seleccionada
    switch (opcion) {
        case 1: // Leer binario
            if (existeBinario) {
                cout << "Leyendo archivo binario..." << endl;
                puntos = leerBinario(archivoBinario);
            } else {
                cout << "ERROR: Archivo binario no encontrado" << endl;
                return 1;
            }
            break;
            
        case 2: // Leer CSV directamente
            if (existeCSV) {
                cout << "Leyendo archivo CSV directamente..." << endl;
                puntos = leerCSV(rutaCSV);
            } else {
                cout << "ERROR: Archivo CSV no encontrado" << endl;
                return 1;
            }
            break;
            
        case 3: // Convertir CSV a binario y leer
            if (existeCSV) {
                cout << "Convirtiendo CSV a binario..." << endl;
                convertirCSVaBinario(rutaCSV, archivoBinario);
                cout << "Leyendo archivo binario recién creado..." << endl;
                puntos = leerBinario(archivoBinario);
            } else {
                cout << "ERROR: Archivo CSV no encontrado para conversión" << endl;
                return 1;
            }
            break;
            
        case 4: // Salir
            cout << "Saliendo..." << endl;
            return 0;
            
        default:
            cout << "Opción inválida. Saliendo..." << endl;
            return 1;
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