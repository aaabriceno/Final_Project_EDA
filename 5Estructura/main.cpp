#include "GeoCluster.cpp"
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
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

// Función para verificar si un archivo existe
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

// Función para contar líneas de un archivo de forma eficiente
long long contarLineasArchivo(const string& archivo) {
    ifstream archivoCSV(archivo);
    if (!archivoCSV.is_open()) {
        cerr << "Error al abrir el archivo para contar líneas: " << archivo << endl;
        return 0;
    }
    
    long long lineas = 0;
    string linea;
    while (getline(archivoCSV, linea)) {
        lineas++;
    }
    archivoCSV.close();
    return lineas - 1; // Restar 1 por el header
}

// Función para leer CSV con streaming (sin cargar todo en memoria)
vector<Punto> leerCSVStreaming(const string& archivo, int maxPuntos = -1) {
    vector<Punto> puntos;
    ifstream archivoCSV(archivo);
    if (!archivoCSV.is_open()) {
        cerr << "Error al abrir el archivo: " << archivo << endl;
        return puntos;
    }
    
    // Contar líneas totales para mostrar progreso
    cout << "Contando líneas del archivo..." << endl;
    long long totalLineas = contarLineasArchivo(archivo);
    cout << "Total de líneas en el archivo: " << totalLineas << endl;
    
    if (maxPuntos > 0 && maxPuntos < totalLineas) {
        cout << "Limitando lectura a " << maxPuntos << " puntos..." << endl;
        totalLineas = maxPuntos;
    }
    
    string linea;
    int numeroLinea = 0;
    bool primeraLinea = true;
    int puntosLeidos = 0;
    
    cout << "Leyendo archivo en streaming..." << endl;
    
    while (getline(archivoCSV, linea) && (maxPuntos == -1 || puntosLeidos < maxPuntos)) {
        numeroLinea++;
        if (linea.empty()) continue;
        if (primeraLinea) { primeraLinea = false; continue; }
        
        stringstream ss(linea);
        string campo;

        int cluster_espacial, cluster_atributivo;
        int id;
        double lat, lon;
        vector<double> atributos;

        try {
            // cluster_espacial
            if (!getline(ss, campo, ',')) continue;
            cluster_espacial = stoi(campo);
            
            // cluster_atributivo
            if (!getline(ss, campo, ',')) continue;
            cluster_atributivo = stoi(campo);
            
            // tripID
            if (!getline(ss, campo, ',')) continue;
            id = stoi(campo);

            // pickup_latitude
            if (!getline(ss, campo, ',')) continue;
            lat = stod(campo);

            // pickup_longitude
            if (!getline(ss, campo, ',')) continue;
            lon = stod(campo);

            // atributos restantes
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) atributos.push_back(stod(campo));
            }
            
            Punto punto(id, lat, lon, atributos);
            punto.id_cluster_geografico = cluster_espacial;
            punto.id_subcluster_atributivo = cluster_atributivo;
            puntos.push_back(punto);
            puntosLeidos++;
            
            // Mostrar progreso cada 10000 puntos
            if (puntosLeidos % 200000 == 0) {
                cout << "Progreso: " << puntosLeidos << "/" << totalLineas << " puntos leídos" << endl;
            }
            
        } catch (...) {
            cerr << "Error en línea " << numeroLinea << ": conversión fallida" << endl;
            continue;
        }
    }
    archivoCSV.close();
    cout << "Se leyeron " << puntos.size() << " puntos del archivo " << archivo << endl;
    return puntos;
}

// Función para leer CSV con columnas: cluster_espacial, cluster_atributivo, tripID, pickup_latitude, pickup_longitude, ...atributos...
vector<Punto> leerCSV(const string& archivo) {
    return leerCSVStreaming(archivo, -1); // Leer todos los puntos
}

// Función para leer CSV limitado a N puntos
vector<Punto> leerCSVLimitado(const string& archivo, int maxPuntos) {
    return leerCSVStreaming(archivo, maxPuntos);
}

// Función para convertir CSV a binario con streaming
void convertirCSVaBinarioStreaming(const string& archivoCSV, const string& archivoBinario, int maxPuntos = -1) {
    cout << "Convirtiendo CSV a binario con streaming..." << endl;
    
    ifstream archivoCSV_in(archivoCSV);
    if (!archivoCSV_in.is_open()) {
        cerr << "Error al abrir el archivo CSV: " << archivoCSV << endl;
        return;
    }
    
    ofstream archivoBin(archivoBinario, ios::binary);
    if (!archivoBin.is_open()) {
        cerr << "No se pudo crear el archivo binario." << endl;
        return;
    }
    
    // Contar líneas totales
    long long totalLineas = contarLineasArchivo(archivoCSV);
    if (maxPuntos > 0 && maxPuntos < totalLineas) {
        totalLineas = maxPuntos;
    }
    
    // Escribir número de puntos (placeholder, se actualizará al final)
    int numPuntos = 0;
    archivoBin.write(reinterpret_cast<char*>(&numPuntos), sizeof(int));
    
    string linea;
    bool primeraLinea = true;
    int puntosProcesados = 0;
    
    while (getline(archivoCSV_in, linea) && (maxPuntos == -1 || puntosProcesados < maxPuntos)) {
        if (linea.empty()) continue;
        if (primeraLinea) { primeraLinea = false; continue; }
        
        stringstream ss(linea);
        string campo;

        int cluster_espacial, cluster_atributivo;
        int id;
        double lat, lon;
        vector<double> atributos;

        try {
            // cluster_espacial
            if (!getline(ss, campo, ',')) continue;
            cluster_espacial = stoi(campo);
            
            // cluster_atributivo
            if (!getline(ss, campo, ',')) continue;
            cluster_atributivo = stoi(campo);
            
            // tripID
            if (!getline(ss, campo, ',')) continue;
            id = stoi(campo);

            // pickup_latitude
            if (!getline(ss, campo, ',')) continue;
            lat = stod(campo);

            // pickup_longitude
            if (!getline(ss, campo, ',')) continue;
            lon = stod(campo);

            // atributos restantes
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) atributos.push_back(stod(campo));
            }
            
            // Escribir punto al archivo binario
            archivoBin.write(reinterpret_cast<const char*>(&id), sizeof(int));
            archivoBin.write(reinterpret_cast<const char*>(&lat), sizeof(double));
            archivoBin.write(reinterpret_cast<const char*>(&lon), sizeof(double));
            archivoBin.write(reinterpret_cast<const char*>(&cluster_espacial), sizeof(int));
            archivoBin.write(reinterpret_cast<const char*>(&cluster_atributivo), sizeof(int));
            int num_atributos = atributos.size();
            archivoBin.write(reinterpret_cast<char*>(&num_atributos), sizeof(int));
            for (int i = 0; i < 12; ++i) {
                double valor = (i < num_atributos) ? atributos[i] : 0.0;
                archivoBin.write(reinterpret_cast<char*>(&valor), sizeof(double));
            }
            
            puntosProcesados++;
            
            // Mostrar progreso cada 10000 puntos
            if (puntosProcesados % 10000 == 0) {
                cout << "Progreso: " << puntosProcesados << "/" << totalLineas << " puntos procesados" << endl;
            }
            
        } catch (...) {
            cerr << "Error en línea: conversión fallida" << endl;
            continue;
        }
    }
    
    // Volver al inicio y escribir el número real de puntos
    archivoBin.seekp(0);
    archivoBin.write(reinterpret_cast<char*>(&puntosProcesados), sizeof(int));
    
    archivoCSV_in.close();
    archivoBin.close();
    cout << "Archivo binario creado: " << archivoBinario << " con " << puntosProcesados << " puntos" << endl;
}

// Función para convertir CSV a binario
void convertirCSVaBinario(const string& archivoCSV, const string& archivoBinario) {
    convertirCSVaBinarioStreaming(archivoCSV, archivoBinario, -1);
}

// Función para leer archivo binario con streaming
vector<Punto> leerBinarioStreaming(const string& archivoBinario, int maxPuntos = -1) {
    vector<Punto> puntos;
    ifstream archivoBin(archivoBinario, ios::binary);
    if (!archivoBin.is_open()) {
        cerr << "Error al abrir archivo binario: " << archivoBinario << endl;
        return puntos;
    }
    int numPuntos;
    archivoBin.read(reinterpret_cast<char*>(&numPuntos), sizeof(int));
    cout << "Leyendo " << numPuntos << " puntos del archivo binario..." << endl;
    
    if (maxPuntos > 0 && maxPuntos < numPuntos) {
        cout << "Limitando lectura a " << maxPuntos << " puntos..." << endl;
        numPuntos = maxPuntos;
    }
    
    for (int i = 0; i < numPuntos; i++) {
        int id;
        archivoBin.read(reinterpret_cast<char*>(&id), sizeof(int));
        double lat, lon;
        archivoBin.read(reinterpret_cast<char*>(&lat), sizeof(double));
        archivoBin.read(reinterpret_cast<char*>(&lon), sizeof(double));
        int cluster_espacial, cluster_atributivo;
        archivoBin.read(reinterpret_cast<char*>(&cluster_espacial), sizeof(int));
        archivoBin.read(reinterpret_cast<char*>(&cluster_atributivo), sizeof(int));
        int num_atributos;
        archivoBin.read(reinterpret_cast<char*>(&num_atributos), sizeof(int));
        vector<double> atributos;
        for (int j = 0; j < 12; j++) {
            double atributo;
            archivoBin.read(reinterpret_cast<char*>(&atributo), sizeof(double));
            if (j < num_atributos) atributos.push_back(atributo);
        }
        Punto punto(id, lat, lon, atributos);
        punto.id_cluster_geografico = cluster_espacial;
        punto.id_subcluster_atributivo = cluster_atributivo;
        puntos.push_back(punto);
        if (i < 5) {
            cout << "Punto " << i+1 << ": ID=" << id << ", Lat=" << lat << ", Lon=" << lon 
                 << ", Cluster=" << cluster_espacial << ", Subcluster=" << cluster_atributivo 
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

// Función para leer archivo binario
vector<Punto> leerBinario(const string& archivoBinario) {
    return leerBinarioStreaming(archivoBinario, -1);
}

// Función para insertar puntos directamente desde CSV sin cargar en memoria
void insertarPuntosDesdeCSVStreaming(const string& archivoCSV, GeoCluster& geoCluster) {
    cout << "Insertando puntos desde CSV usando streaming..." << endl;
    
    ifstream archivoCSV_in(archivoCSV);
    if (!archivoCSV_in.is_open()) {
        cerr << "Error al abrir el archivo CSV: " << archivoCSV << endl;
        return;
    }
    
    // Contar líneas totales para mostrar progreso
    cout << "Contando líneas del archivo..." << endl;
    long long totalLineas = contarLineasArchivo(archivoCSV);
    cout << "Total de líneas en el archivo: " << totalLineas << endl;
    
    string linea;
    bool primeraLinea = true;
    int puntosInsertados = 0;
    
    cout << "Insertando puntos en GeoCluster-Tree..." << endl;
    
    while (getline(archivoCSV_in, linea)) {
        if (linea.empty()) continue;
        if (primeraLinea) { primeraLinea = false; continue; }
        
        stringstream ss(linea);
        string campo;

        int cluster_espacial, cluster_atributivo;
        int id;
        double lat, lon;
        vector<double> atributos;

        try {
            // cluster_espacial
            if (!getline(ss, campo, ',')) continue;
            cluster_espacial = stoi(campo);
            
            // cluster_atributivo
            if (!getline(ss, campo, ',')) continue;
            cluster_atributivo = stoi(campo);
            
            // tripID
            if (!getline(ss, campo, ',')) continue;
            id = stoi(campo);

            // pickup_latitude
            if (!getline(ss, campo, ',')) continue;
            lat = stod(campo);

            // pickup_longitude
            if (!getline(ss, campo, ',')) continue;
            lon = stod(campo);

            // atributos restantes
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) atributos.push_back(stod(campo));
            }
            
            // Crear punto e insertarlo directamente
            Punto punto(id, lat, lon, atributos);
            punto.id_cluster_geografico = cluster_espacial;
            punto.id_subcluster_atributivo = cluster_atributivo;
            
            geoCluster.inserData(punto);
            puntosInsertados++;
            
            // Liberar memoria del vector de atributos
            atributos.clear();
            atributos.shrink_to_fit();
            
            // Mostrar progreso cada 100000 puntos (más frecuente)
            if (puntosInsertados % 100000 == 0) {
                cout << "Progreso: " << puntosInsertados << "/" << totalLineas << " puntos insertados" << endl;
                // Forzar liberación de memoria
                cout.flush();
            }
            
            // Pausa cada 500000 puntos para dar respiro al sistema
            if (puntosInsertados % 500000 == 0) {
                cout << "Pausa de 2 segundos para estabilizar memoria..." << endl;
                this_thread::sleep_for(chrono::seconds(2));
            }
            
        } catch (...) {
            cerr << "Error en línea: conversión fallida" << endl;
            continue;
        }
    }
    
    archivoCSV_in.close();
    cout << "Inserción completada. Total de puntos insertados: " << puntosInsertados << endl;
    
    // Mostrar información de optimización
    cout << "✅ Optimización de memoria activa:" << endl;
    cout << "   - Puntos almacenados como PointID en nodos hoja" << endl;
    cout << "   - Puntos completos en hash table: " << geoCluster.obtenerNumeroPuntosCompletos() << endl;
    cout << "   - Ahorro estimado: " << ((double)(sizeof(Punto) - sizeof(PointID)) / sizeof(Punto) * 100) << "% de memoria" << endl;
    // ✅ Optimización completada - matrices de similitud eliminadas
    cout << "✅ Optimización completada - matrices de similitud eliminadas" << endl;
    // Verificar que los puntos se insertaron correctamente
    cout << "[DEBUG] Estructura del arbol:" << endl;
    geoCluster.imprimirArbol();
}

// Función para insertar puntos directamente desde archivo binario sin cargar en memoria
void insertarPuntosDesdeBinarioStreaming(const string& archivoBinario, GeoCluster& geoCluster) {
    cout << "Insertando puntos desde archivo binario usando streaming..." << endl;
    
    ifstream archivoBin(archivoBinario, ios::binary);
    if (!archivoBin.is_open()) {
        cerr << "Error al abrir archivo binario: " << archivoBinario << endl;
        return;
    }
    
    int numPuntos;
    archivoBin.read(reinterpret_cast<char*>(&numPuntos), sizeof(int));
    cout << "Total de puntos en archivo binario: " << numPuntos << endl;
    
    int puntosInsertados = 0;
    cout << "Insertando puntos en GeoCluster-Tree..." << endl;
    
    for (int i = 0; i < numPuntos; i++) {
        int id;
        archivoBin.read(reinterpret_cast<char*>(&id), sizeof(int));
        double lat, lon;
        archivoBin.read(reinterpret_cast<char*>(&lat), sizeof(double));
        archivoBin.read(reinterpret_cast<char*>(&lon), sizeof(double));
        int cluster_espacial, cluster_atributivo;
        archivoBin.read(reinterpret_cast<char*>(&cluster_espacial), sizeof(int));
        archivoBin.read(reinterpret_cast<char*>(&cluster_atributivo), sizeof(int));
        int num_atributos;
        archivoBin.read(reinterpret_cast<char*>(&num_atributos), sizeof(int));
        vector<double> atributos;
        for (int j = 0; j < 12; j++) {
            double atributo;
            archivoBin.read(reinterpret_cast<char*>(&atributo), sizeof(double));
            if (j < num_atributos) atributos.push_back(atributo);
        }
        
        // Crear punto e insertarlo directamente
        Punto punto(id, lat, lon, atributos);
        punto.id_cluster_geografico = cluster_espacial;
        punto.id_subcluster_atributivo = cluster_atributivo;
        
        geoCluster.inserData(punto);
        puntosInsertados++;
        
        // Liberar memoria del vector de atributos
        atributos.clear();
        atributos.shrink_to_fit();
        
        // Mostrar progreso cada 100000 puntos (más frecuente)
        if (puntosInsertados % 100000 == 0) {
            cout << "Progreso: " << puntosInsertados << "/" << numPuntos << " puntos insertados" << endl;
            // Forzar liberación de memoria
            cout.flush();
        }
        
        // Pausa cada 500000 puntos para dar respiro al sistema
        if (puntosInsertados % 500000 == 0) {
            cout << "Pausa de 2 segundos para estabilizar memoria..." << endl;
            this_thread::sleep_for(chrono::seconds(2));
        }
    }
    
    archivoBin.close();
    cout << "Inserción completada. Total de puntos insertados: " << puntosInsertados << endl;
    
    // Mostrar información de optimización
    cout << "✅ Optimización de memoria activa:" << endl;
    cout << "   - Puntos almacenados como PointID en nodos hoja" << endl;
    cout << "   - Puntos completos en hash table: " << geoCluster.obtenerNumeroPuntosCompletos() << endl;
    cout << "   - Ahorro estimado: " << ((double)(sizeof(Punto) - sizeof(PointID)) / sizeof(Punto) * 100) << "% de memoria" << endl;
    // ✅ Optimización completada - matrices de similitud eliminadas
    cout << "✅ Optimización completada - matrices de similitud eliminadas" << endl;
    // Verificar que los puntos se insertaron correctamente
    cout << "[DEBUG] Estructura del arbol:" << endl;
    geoCluster.imprimirArbol();
}

// Función para insertar puntos en lotes (para archivos muy grandes)
void insertarPuntosEnLotes(const string& archivoCSV, GeoCluster& geoCluster, int tamanoLote = 500000) {
    cout << "Insertando puntos en lotes de " << tamanoLote << " puntos..." << endl;
    
    ifstream archivoCSV_in(archivoCSV);
    if (!archivoCSV_in.is_open()) {
        cerr << "Error al abrir el archivo CSV: " << archivoCSV << endl;
        return;
    }
    
    // Contar líneas totales para mostrar progreso
    cout << "Contando líneas del archivo..." << endl;
    long long totalLineas = contarLineasArchivo(archivoCSV);
    cout << "Total de líneas en el archivo: " << totalLineas << endl;
    
    string linea;
    bool primeraLinea = true;
    int puntosInsertados = 0;
    int puntosEnLote = 0;
    
    cout << "Insertando puntos en GeoCluster-Tree..." << endl;
    
    while (getline(archivoCSV_in, linea)) {
        if (linea.empty()) continue;
        if (primeraLinea) { primeraLinea = false; continue; }
        
        stringstream ss(linea);
        string campo;

        int cluster_espacial, cluster_atributivo;
        int id;
        double lat, lon;
        vector<double> atributos;

        try {
            // cluster_espacial
            if (!getline(ss, campo, ',')) continue;
            cluster_espacial = stoi(campo);
            
            // cluster_atributivo
            if (!getline(ss, campo, ',')) continue;
            cluster_atributivo = stoi(campo);
            
            // tripID
            if (!getline(ss, campo, ',')) continue;
            id = stoi(campo);

            // pickup_latitude
            if (!getline(ss, campo, ',')) continue;
            lat = stod(campo);

            // pickup_longitude
            if (!getline(ss, campo, ',')) continue;
            lon = stod(campo);

            // atributos restantes
            while (getline(ss, campo, ',')) {
                if (!campo.empty()) atributos.push_back(stod(campo));
            }
            
            // Crear punto e insertarlo directamente
            Punto punto(id, lat, lon, atributos);
            punto.id_cluster_geografico = cluster_espacial;
            punto.id_subcluster_atributivo = cluster_atributivo;
            
            geoCluster.inserData(punto);
            puntosInsertados++;
            puntosEnLote++;
            
            // Liberar memoria del vector de atributos
            atributos.clear();
            atributos.shrink_to_fit();
            
            // Mostrar progreso cada 50000 puntos
            if (puntosInsertados % 50000 == 0) {
                cout << "Progreso: " << puntosInsertados << "/" << totalLineas << " puntos insertados" << endl;
                cout.flush();
            }
            
            // Pausa y estabilización cada lote
            if (puntosEnLote >= tamanoLote) {
                cout << "Lote completado: " << puntosEnLote << " puntos. Pausa de 5 segundos..." << endl;
                this_thread::sleep_for(chrono::seconds(5));
                puntosEnLote = 0;
            }
            
        } catch (...) {
            cerr << "Error en línea: conversión fallida" << endl;
            continue;
        }
    }
    
    archivoCSV_in.close();
    cout << "Inserción completada. Total de puntos insertados: " << puntosInsertados << endl;
    
    // Mostrar información de optimización
    cout << "Optimización de memoria activa:" << endl;
    cout << "   - Puntos almacenados como PointID en nodos hoja" << endl;
    cout << "   - Puntos completos en hash table: " << geoCluster.obtenerNumeroPuntosCompletos() << endl;
    cout << "   - Ahorro estimado: " << ((double)(sizeof(Punto) - sizeof(PointID)) / sizeof(Punto) * 100) << "% de memoria" << endl;
    //  Optimización completada - matrices de similitud eliminadas
    cout << "Optimización completada - matrices de similitud eliminadas" << endl;
    // Verificar que los puntos se insertaron correctamente
    cout << "[DEBUG] Estructura del arbol:" << endl;
    geoCluster.imprimirArbol();
}

int main() {
    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(14);
    GeoCluster geoCluster;
    
    // Archivos disponibles (priorizar el generado por Python)
    string archivoCSV = "puntos.csv";
    string archivoBinarioPython = "2Database/7datos_clusterizados_rtree.bin";
    string archivoBinarioCpp = "5clusterizacion_geografica_attr.bin";
    
    //Buscamos el archivo binario para poder usar
    string archivoBinario;
    if (archivoExiste(archivoBinarioPython)) {
        archivoBinario = archivoBinarioPython;
        //cout << "Detectado archivo binario - Python: " << archivoBinario << endl;
    } else if (archivoExiste(archivoBinarioCpp)) {
        archivoBinario = archivoBinarioCpp;
        //cout << "Detectado archivo binario - C++: " << archivoBinario << endl;
    } else {
        archivoBinario = archivoBinarioCpp; // Usar como default
        //cout << "No se encontraron archivos binarios." << endl;
    }
    
    vector<Punto> puntos;
    bool datosCargados = false;
    long long totalPuntosDisponibles = 0;
    
    while (true) {
    cout << "=============================================" << endl;
    cout << "        GEOCLUSTER-TREE PROYECTO EDA" << endl;
    cout << "=============================================" << endl;
    cout << "VERSIÓN OPTIMIZADA CON PointID" << endl;
    cout << "Ahorro de memoria: ~84% por punto" << endl;
    cout << "Mejor rendimiento y escalabilidad" << endl;
        cout << "\nArchivos disponibles:" << endl;
        cout << "- CSV: " << archivoCSV << (archivoExiste(archivoCSV) ? " (encontrado)" : " (no encontrado)") << endl;
        cout << "- Binario Python: " << archivoBinarioPython << (archivoExiste(archivoBinarioPython) ? " (encontrado)" : " (no encontrado)") << endl;
        cout << "- Binario C++: " << archivoBinarioCpp << (archivoExiste(archivoBinarioCpp) ? " (encontrado)" : " (no encontrado)") << endl;
        cout << "- Binario actual: " << archivoBinario << endl;
        cout << endl;
        cout << "=== MENU PRINCIPAL ===" << endl;
        cout << "1. Cargar datos desde archivo binario" << endl;
        cout << "2. Cargar CSV (streaming - sin límite de memoria)" << endl;
        cout << "3. Insertar puntos en GeoCluster-Tree (streaming)" << endl;
        cout << "4. Insertar puntos en lotes (para archivos muy grandes)" << endl;
        cout << "5. Generar microclusters en hojas" << endl;
        cout << "6. Consulta 1: N puntos mas similares a un punto en rango" << endl;
        cout << "7. Consulta 2: Grupos de puntos similares en rango" << endl;
        cout << "8. Mostrar estadisticas de microclusters" << endl;
        cout << "9. Mostrar informaciOn del Arbol" << endl;
        cout << "10. Mostrar estadisticas de memoria optimizada" << endl;
        cout << "11. Salir" << endl;
        cout << "Selecciona una opcion: ";
    int opcion;
        cin >> opcion;
        if (opcion == 1) {
                cout << "Contando puntos disponibles en archivo binario..." << endl;
                ifstream archivoBin_temp(archivoBinario, ios::binary);
                if (archivoBin_temp.is_open()) {
                    int numPuntos;
                    archivoBin_temp.read(reinterpret_cast<char*>(&numPuntos), sizeof(int));
                    cout << "Total de puntos disponibles: " << numPuntos << endl;
                    archivoBin_temp.close();
                }
                datosCargados = true;
        } 
        else if (opcion == 2) {
            cout << "Contando puntos disponibles en el CSV..." << endl;
            totalPuntosDisponibles = contarLineasArchivo(archivoCSV);
            cout << "Total de puntos disponibles: " << totalPuntosDisponibles << endl;
            datosCargados = true;
        } 
        else if (opcion == 3) {
            if (!datosCargados) {
                cout << "Primero debes cargar los datos (opción 1 o 2)." << endl;
            } else {
                cout << "¿Desde qué archivo quieres insertar?" << endl;
                cout << "1. CSV: " << archivoCSV << endl;
                cout << "2. Binario: " << archivoBinario << endl;
                cout << "Selecciona (1 o 2): ";
                int tipoArchivo;
                cin >> tipoArchivo;
                
                if (tipoArchivo == 1) {
                    cout << "Insertando puntos desde CSV usando streaming..." << endl;
                    insertarPuntosDesdeCSVStreaming(archivoCSV, geoCluster);
                } else if (tipoArchivo == 2) {
                    cout << "Insertando puntos desde archivo binario usando streaming..." << endl;
                    insertarPuntosDesdeBinarioStreaming(archivoBinario, geoCluster);
                } else {
                    cout << "Opción no válida." << endl;
                }
            }
        }
        else if (opcion == 4) {
            if (!datosCargados) {
                cout << "Primero debes cargar los datos (opción 1 o 2)." << endl;
            } else {
                cout << "Insertando puntos en lotes (recomendado para archivos muy grandes)..." << endl;
                int tamanoLote;
                cout << "Ingresa el tamaño del lote (recomendado: 300000): ";
                cin >> tamanoLote;
                insertarPuntosEnLotes(archivoCSV, geoCluster, tamanoLote);
            }
        } 
        else if (opcion == 5) {
            cout << "Los microclusters se crean automáticamente durante la inserción" << endl;
            cout << "No es necesario crearlos manualmente" << endl;
        } 
        else if (opcion == 6) {
            if (!datosCargados) { cout << "ERROR: No hay datos cargados" << endl; continue; }
            cout << "\n=== CONSULTA 1: N PUNTOS MAS SIMILARES (OPTIMIZADO) ===" << endl;
            int id_punto_busqueda;
            cout << "Ingresa el ID del punto de búsqueda: ";
            cin >> id_punto_busqueda;
            
            // Buscar el punto en el CSV
            Punto punto_busqueda;
            bool encontrado = false;
            ifstream archivoCSV_busqueda(archivoCSV);
            if (archivoCSV_busqueda.is_open()) {
                string linea;
                bool primeraLinea = true;
                while (getline(archivoCSV_busqueda, linea) && !encontrado) {
                    if (linea.empty() || primeraLinea) { primeraLinea = false; continue; }
                    stringstream ss(linea);
                    string campo;
                    int cluster_espacial, cluster_atributivo, id;
                    double lat, lon;
                    vector<double> atributos;
                    try {
                        if (!getline(ss, campo, ',')) continue;
                        cluster_espacial = stoi(campo);
                        if (!getline(ss, campo, ',')) continue;
                        cluster_atributivo = stoi(campo);
                        if (!getline(ss, campo, ',')) continue;
                        id = stoi(campo);
                        if (id == id_punto_busqueda) {
                            if (!getline(ss, campo, ',')) continue;
                            lat = stod(campo);
                            if (!getline(ss, campo, ',')) continue;
                            lon = stod(campo);
                            while (getline(ss, campo, ',')) {
                                if (!campo.empty()) atributos.push_back(stod(campo));
                            }
                            punto_busqueda = Punto(id, lat, lon, atributos);
                            punto_busqueda.id_cluster_geografico = cluster_espacial;
                            punto_busqueda.id_subcluster_atributivo = cluster_atributivo;
                            encontrado = true;
                        }
                    } catch (...) { continue; }
                }
                archivoCSV_busqueda.close();
            }
            if (!encontrado) { cout << "ERROR: Punto con ID " << id_punto_busqueda << " no encontrado" << endl; continue; }
            double lat_min, lat_max, lon_min, lon_max;
            cout << "Ingresa rango de búsqueda:" << endl;
            cout << "Latitud minima: "; cin >> lat_min;
            cout << "Latitud maxima: "; cin >> lat_max;
            cout << "Longitud minima: "; cin >> lon_min;
            cout << "Longitud maxima: "; cin >> lon_max;
            MBR rango_busqueda(lat_min, lon_min, lat_max, lon_max);
            int numero_de_puntos;
            cout << "Numero de puntos similares a buscar: ";
            cin >> numero_de_puntos;
            // USAR FUNCIÓN OPTIMIZADA
            vector<Punto> resultado = geoCluster.buscarPuntosPorSimilitudCluster(punto_busqueda, rango_busqueda, numero_de_puntos);
            cout << "\n=== RESULTADOS (OPTIMIZADO) ===" << endl;
            cout << "Punto de busqueda: ID=" << punto_busqueda.id 
                 << " Lat=" << punto_busqueda.latitud 
                 << " Lon=" << punto_busqueda.longitud << endl;
            cout << "Puntos similares encontrados: " << resultado.size() << endl;
            cout << "\nIDs de los puntos similares (ordenados por distancia euclidiana):" << endl;
            for (size_t i = 0; i < resultado.size(); ++i) {
                double dist = sqrt(pow(resultado[i].latitud - punto_busqueda.latitud, 2) + pow(resultado[i].longitud - punto_busqueda.longitud, 2));
                cout << "Punto " << i+1 << ": ID=" << resultado[i].id 
                     << " (Lat=" << resultado[i].latitud 
                     << ", Lon=" << resultado[i].longitud 
                     << ", Cluster=" << resultado[i].id_cluster_geografico
                     << ", Subcluster=" << resultado[i].id_subcluster_atributivo 
                     << ", Distancia=" << dist << ")" << endl;
            }
        } 
        else if (opcion == 7) {
            if (!datosCargados) { cout << "ERROR: No hay datos cargados" << endl; continue; }
            cout << "\n=== CONSULTA 2: GRUPOS DE PUNTOS SIMILARES (OPTIMIZADO) ===" << endl;
            double lat_min, lat_max, lon_min, lon_max;
            cout << "Ingresa rango de búsqueda:" << endl;
            cout << "Latitud minima: "; cin >> lat_min;
            cout << "Latitud maxima: "; cin >> lat_max;
            cout << "Longitud minima: "; cin >> lon_min;
            cout << "Longitud maxima: "; cin >> lon_max;
            MBR rango_busqueda(lat_min, lon_min, lat_max, lon_max);
            // USAR FUNCIÓN OPTIMIZADA
            vector<vector<Punto>> grupos_similares = geoCluster.grupos_similares_de_puntos(rango_busqueda);
            cout << "\n=== RESULTADOS (OPTIMIZADO) ===" << endl;
            cout << "Grupos de puntos similares encontrados: " << grupos_similares.size() << endl;
            for (size_t i = 0; i < grupos_similares.size(); i++) {
                cout << "Grupo " << i + 1 << ": " << grupos_similares[i].size() << " puntos" << endl;
                for (const auto& p : grupos_similares[i]) {
                    cout << "    ID=" << p.id << " (Lat=" << p.latitud << ", Lon=" << p.longitud << ", Cluster=" << p.id_cluster_geografico << ", Subcluster=" << p.id_subcluster_atributivo << ")" << endl;
                }
            }
        } 
        else if (opcion == 8) {
            geoCluster.mostrarEstadisticasMicroclusters();
        } 
        else if (opcion == 9) {
            cout << "\n=== INFORMACION DEL R*-TREE ===" << endl;
            cout << "Total de puntos en hash table: " << geoCluster.obtenerNumeroPuntosCompletos() << endl;
            cout << "Total de puntos completos en hash table: " << geoCluster.obtenerNumeroPuntosCompletos() << endl;
            cout << "\n=== OPTIMIZACIÓN DE MEMORIA ===" << endl;
            cout << "✓ Usando PointID en nodos hoja (optimizado)" << endl;
            cout << "✓ Puntos completos almacenados en hash table" << endl;
            cout << "✓ Ahorro de memoria: ~84% por punto" << endl;
            cout << "\nEstructura del arbol:" << endl;
            geoCluster.imprimirArbol();
        } 
        else if (opcion == 10) {
            cout << "\n=== ESTADÍSTICAS DE MEMORIA OPTIMIZADA ===" << endl;
            cout << "Tamaño de estructura Punto: " << sizeof(Punto) << " bytes" << endl;
            cout << "Tamaño de estructura PointID: " << sizeof(PointID) << " bytes" << endl;
            cout << "Ahorro por punto: " << (sizeof(Punto) - sizeof(PointID)) << " bytes" << endl;
            cout << "Porcentaje de ahorro: " << ((double)(sizeof(Punto) - sizeof(PointID)) / sizeof(Punto) * 100) << "%" << endl;
            
            int puntos_arbol = geoCluster.obtenerNumeroPuntosCompletos();
            int puntos_hash = geoCluster.obtenerNumeroPuntosCompletos();
            
            if (puntos_arbol > 0) {
                size_t memoria_original = sizeof(Punto) * puntos_arbol;
                size_t memoria_optimizada = (sizeof(PointID) * puntos_arbol) + (sizeof(Punto) * puntos_hash);
                
                cout << "\nMemoria estimada:" << endl;
                cout << "Con Punto original: " << memoria_original / (1024*1024) << " MB" << endl;
                cout << "Con PointID + Hash: " << memoria_optimizada / (1024*1024) << " MB" << endl;
                
                double ahorro_total = ((double)(memoria_original - memoria_optimizada) / memoria_original) * 100;
                cout << "Ahorro total estimado: " << ahorro_total << "%" << endl;
            }
            
            cout << "\nEstado actual:" << endl;
            cout << "✓ Puntos en árbol (PointID): " << puntos_arbol << endl;
            cout << "✓ Puntos en hash table: " << puntos_hash << endl;
            cout << "✓ Optimización activa" << endl;
        }
        else if (opcion == 11) {
            cout << "¡Hasta luego!" << endl;
            break;
        } 
        else {
            cout << "Opcion no valida" << endl;
    }
    cout << endl;
    cout << "Presiona Enter para continuar...";
    cin.ignore();
    cin.get();
    system("cls");
    }
    return 0;
}
