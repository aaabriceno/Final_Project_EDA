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
    cout.precision(14);

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
            //cout << "Punto leído: ID=" << id << ", Lat=" << lat << ", Lon=" << lon << ", Atributos=" << atributos.size() << endl;
            
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
    double atributos[10];  // 10 atributos como máximo sin aplicar preprocesamiento o con este
    int num_atributos;
};

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
    cout.precision(14);
    
    // Crear un objeto de la clase GeoCluster
    GeoCluster geoCluster;

    // Configuración de archivos
    string archivoCSV = "C:/Users/anthony/Final_Project_EDA/2Database/puntos10k.csv";
    string archivoBinario = "C:/Users/anthony/Final_Project_EDA/2Database/puntos500k.bin";
    
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
        "2Database/puntos500k.csv",
        "./2Database/puntos500k.csv",
        "../2Database/puntos500k.csv",
        "2Database\\puntos500k.csv",
        "puntos500k.csv"
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
        cout << "Archivo binario encontrado: " << archivoBinario << endl;
    } else {
        cout << "Archivo binario no encontrado" << endl;
    }
    
    if (existeCSV) {
        cout << "Archivo CSV encontrado: " << rutaCSV << endl;
    } else {
        cout << "Archivo CSV no encontrado" << endl;
    }
    cout << endl;
    
    // Menú de opciones
    menu_principal:
    cout << "=== MENÚ PRINCIPAL ===" << endl;
    cout << "1. Cargar datos desde archivo binario" << endl;
    cout << "2. Cargar datos desde archivo CSV" << endl;
    cout << "3. Insertar puntos en R*-Tree" << endl;
    cout << "4. Crear microclusters en hojas" << endl;
    cout << "5. Consulta 1: N puntos más similares a un punto en rango" << endl;
    cout << "6. Consulta 2: Grupos de puntos similares en rango" << endl;
    cout << "7. Mostrar información del árbol" << endl;
    cout << "8. Verificar duplicados" << endl;
    cout << "9. Salir" << endl;
    cout << endl;
    
    int opcion;
    cout << "Selecciona una opción: ";
    cin >> opcion;
    
    switch (opcion) {
        case 1:
            if (existeBinario) {
                cout << "Leyendo archivo binario..." << endl;
                puntos = leerBinario(archivoBinario);
            } else {
                cout << "ERROR: Archivo binario no encontrado" << endl;
            }
            break;
            
        case 2:
            if (existeCSV) {
                cout << "Convirtiendo CSV a binario..." << endl;
                //convertirCSVaBinario(rutaCSV, archivoBinario);
                cout << "Leyendo archivo binario recién creado..." << endl;
                puntos = leerBinario(archivoBinario);
            } else {
                cout << "ERROR: Archivo CSV no encontrado para conversión" << endl;
            }
            break;
            
        case 3:
            if (puntos.empty()) {
                cout << "ERROR: No hay datos cargados. Carga datos primero (opción 1 o 2)" << endl;
            } else {
                cout << "Insertando " << puntos.size() << " puntos en R*-Tree..." << endl;
                for (const auto& punto : puntos) {
                    geoCluster.inserData(punto);
                }
                cout << "✅ Inserción completada" << endl;
            }
            break;
            
        case 4:
            cout << "Creando microclusters en hojas del R*-Tree..." << endl;
            geoCluster.crearMicroclustersEnHojas();
            break;
            
        case 5: {
            if (puntos.empty()) {
                cout << "ERROR: No hay datos cargados" << endl;
                break;
            }
            
            cout << "\n=== CONSULTA 1: N PUNTOS MÁS SIMILARES ===" << endl;
            
            // Seleccionar punto de búsqueda
            int id_punto_busqueda;
            cout << "Ingresa el ID del punto de búsqueda: ";
            cin >> id_punto_busqueda;
            
            // Buscar el punto en los datos
            Punto punto_busqueda;
            bool encontrado = false;
            for (const auto& p : puntos) {
                if (p.id == id_punto_busqueda) {
                    punto_busqueda = p;
                    encontrado = true;
                    break;
                }
            }
            
            if (!encontrado) {
                cout << "ERROR: Punto con ID " << id_punto_busqueda << " no encontrado" << endl;
                break;
            }
            
            // Definir rango de búsqueda
            double lat_min, lat_max, lon_min, lon_max;
            cout << "Ingresa rango de búsqueda:" << endl;
            cout << "Latitud mínima: ";
            cin >> lat_min;
            cout << "Latitud máxima: ";
            cin >> lat_max;
            cout << "Longitud mínima: ";
            cin >> lon_min;
            cout << "Longitud máxima: ";
            cin >> lon_max;
            
            MBR rango_busqueda(lat_min, lon_min, lat_max, lon_max);
            
            // Número de puntos similares
            int n_puntos;
            cout << "Número de puntos similares a buscar: ";
            cin >> n_puntos;
            
            // Ejecutar consulta
            vector<Punto> puntos_similares = geoCluster.n_puntos_similiares_a_punto(
                punto_busqueda, rango_busqueda, n_puntos);
            
            cout << "\n=== RESULTADOS ===" << endl;
            cout << "Punto de búsqueda: ID=" << punto_busqueda.id 
                 << " Lat=" << punto_busqueda.latitud 
                 << " Lon=" << punto_busqueda.longitud << endl;
            cout << "Puntos similares encontrados: " << puntos_similares.size() << endl;
            break;
        }
        
        case 6: {
            if (puntos.empty()) {
                cout << "ERROR: No hay datos cargados" << endl;
                break;
            }
            
            cout << "\n=== CONSULTA 2: GRUPOS DE PUNTOS SIMILARES ===" << endl;
            
            // Definir rango de búsqueda
            double lat_min, lat_max, lon_min, lon_max;
            cout << "Ingresa rango de búsqueda:" << endl;
            cout << "Latitud mínima: ";
            cin >> lat_min;
            cout << "Latitud máxima: ";
            cin >> lat_max;
            cout << "Longitud mínima: ";
            cin >> lon_min;
            cout << "Longitud máxima: ";
            cin >> lon_max;
            
            MBR rango_busqueda(lat_min, lon_min, lat_max, lon_max);
            
            // Ejecutar consulta
            vector<vector<Punto>> grupos_similares = geoCluster.grupos_similares_de_puntos(rango_busqueda);
            
            cout << "\n=== RESULTADOS ===" << endl;
            cout << "Grupos de puntos similares encontrados: " << grupos_similares.size() << endl;
            
            for (size_t i = 0; i < grupos_similares.size(); i++) {
                cout << "Grupo " << i + 1 << ": " << grupos_similares[i].size() << " puntos" << endl;
                if (i < 3) {  // Mostrar solo los primeros 3 grupos
                    for (const auto& punto : grupos_similares[i]) {
                        cout << "  - ID: " << punto.id << " (Lat: " << punto.latitud 
                             << ", Lon: " << punto.longitud << ")" << endl;
                    }
                }
            }
            break;
        }
        
        case 7:
            cout << "\n=== INFORMACIÓN DEL R*-TREE ===" << endl;
            cout << "Total de puntos en el árbol: " << geoCluster.contarPuntosEnArbol() << endl;
            cout << "\nEstructura del árbol:" << endl;
            geoCluster.imprimirArbol();
            break;
            
        case 8:
            cout << "\n=== VERIFICACIÓN DE DUPLICADOS ===" << endl;
            geoCluster.verificarDuplicados();
            break;
            
        case 9:
            cout << "¡Hasta luego!" << endl;
            return 0;
            
        default:
            cout << "Opción no válida" << endl;
            break;
    }
    
    cout << endl;
    cout << "Presiona Enter para continuar...";
    cin.ignore();
    cin.get();
    
    // Limpiar pantalla (Windows)
    system("cls");
    
    goto menu_principal;
}