#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>
#include "Eigen/Dense"

using namespace std;
using namespace std::chrono;

struct Point {
    double x, y;
    int cluster = 0; // 0 sin asignar, -1 ruido, >0 cluster
    bool visited = false;
};

// ---------------- Utils ------------------------

double distance(const Point& a, const Point& b) {
    return sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

vector<Point> readCSV(const string& filename) {
    vector<Point> points;
    ifstream file(filename);
    string line;
    getline(file, line); // saltar cabecera
    while (getline(file, line)) {
        stringstream ss(line);
        string xStr, yStr;
        getline(ss, xStr, ',');
        getline(ss, yStr, ',');
        Point p;
        p.x = stod(xStr);
        p.y = stod(yStr);
        points.push_back(p);
    }
    return points;
}

void writeCSV(const string& filename, const vector<Point>& points) {
    ofstream out(filename);
    out << "x,y,cluster\n";
    for (const auto& p : points) {
        out << p.x << "," << p.y << "," << p.cluster << "\n";
    }
}

// ---------------- DBSCAN clásico ------------------

vector<int> regionQuery(const vector<Point>& points, int index, double eps) {
    vector<int> neighbors;
    for (int i = 0; i < points.size(); ++i)
        if (distance(points[index], points[i]) <= eps)
            neighbors.push_back(i);
    return neighbors;
}

void expandCluster(vector<Point>& points, int index, vector<int>& neighbors,
                   int clusterId, double eps, int minPts) {
    points[index].cluster = clusterId;
    for (size_t i = 0; i < neighbors.size(); ++i) {
        int nIdx = neighbors[i];
        if (!points[nIdx].visited) {
            points[nIdx].visited = true;
            auto moreNeighbors = regionQuery(points, nIdx, eps);
            if (moreNeighbors.size() >= minPts)
                neighbors.insert(neighbors.end(), moreNeighbors.begin(), moreNeighbors.end());
        }
        if (points[nIdx].cluster == 0 || points[nIdx].cluster == -1)
            points[nIdx].cluster = clusterId;
    }
}

void dbscan(vector<Point>& points, double eps, int minPts) {
    int clusterId = 0;
    for (int i = 0; i < points.size(); ++i) {
        if (points[i].visited) continue;
        points[i].visited = true;
        auto neighbors = regionQuery(points, i, eps);
        if (neighbors.size() < minPts) {
            points[i].cluster = -1;
        } else {
            ++clusterId;
            expandCluster(points, i, neighbors, clusterId, eps, minPts);
        }
    }
}

// ---------------- KDTree ------------------

struct KDNode {
    int index; // índice en el vector original
    KDNode* left = nullptr;
    KDNode* right = nullptr;
    bool vertical;

    KDNode(int idx, bool v) : index(idx), vertical(v) {}
};

class KDTree {
public:
    KDTree(const vector<Point>& pts) : points(pts) {
        indices.resize(points.size());
        for (int i = 0; i < indices.size(); ++i) indices[i] = i;
        root = build(0, points.size(), true);
    }

    vector<int> pointsInSphere(const Point& q, double eps) const {
        vector<int> result;
        search(root, q, eps, result);
        return result;
    }

private:
    const vector<Point>& points;
    vector<int> indices; // índices de los puntos
    KDNode* root;

    KDNode* build(int l, int r, bool vertical) {
        if (l >= r) return nullptr;
        int m = (l + r) / 2;
        auto comp = [&](int a, int b) {
            return vertical ? points[a].x < points[b].x : points[a].y < points[b].y;
        };
        nth_element(indices.begin() + l, indices.begin() + m, indices.begin() + r, comp);
        KDNode* node = new KDNode(indices[m], vertical);
        node->left = build(l, m, !vertical);
        node->right = build(m + 1, r, !vertical);
        return node;
    }

    void search(KDNode* node, const Point& q, double eps, vector<int>& result) const {
        if (!node) return;
        const Point& p = points[node->index];
        if (distance(p, q) <= eps)
            result.push_back(node->index);

        double delta = node->vertical ? (q.x - p.x) : (q.y - p.y);
        if (delta <= eps) search(node->left, q, eps, result);
        if (delta >= -eps) search(node->right, q, eps, result);
    }
};

void expandCluster(vector<Point>& points, KDTree& tree, int index, vector<int>& neighbors,
                   int clusterId, double eps, int minPts) {
    points[index].cluster = clusterId;

    for (size_t i = 0; i < neighbors.size(); ++i) {
        int idx = neighbors[i];
        if (!points[idx].visited) {
            points[idx].visited = true;
            auto more = tree.pointsInSphere(points[idx], eps);
            if (more.size() >= minPts)
                neighbors.insert(neighbors.end(), more.begin(), more.end());
        }
        if (points[idx].cluster == 0 || points[idx].cluster == -1) {
            points[idx].cluster = clusterId;
        }
    }
}

void dbscanKdTree(vector<Point>& points, double eps, int minPts) {
    KDTree tree(points);
    int clusterId = 0;
    for (int i = 0; i < points.size(); ++i) {
        if (points[i].visited) continue;
        points[i].visited = true;
        auto neighbors = tree.pointsInSphere(points[i], eps);
        if (neighbors.size() < minPts) {
            points[i].cluster = -1;
        } else {
            ++clusterId;
            expandCluster(points, tree, i, neighbors, clusterId, eps, minPts);
        }
    }
}
// ---------------- MAIN ------------------

double estimarEpsCodoMaximo(const vector<Point>& points, int k = 4) {
    int n = points.size();
    vector<double> k_distances;

    for (int i = 0; i < n; ++i) {
        vector<double> dists;
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            dists.push_back(distance(points[i], points[j]));
        }
        sort(dists.begin(), dists.end());
        k_distances.push_back(dists[k - 1]);
    }

    sort(k_distances.begin(), k_distances.end());

    double max_diff = 0.0;
    int best_index = 0;
    for (int i = 1; i < n; ++i) {
        double diff = k_distances[i] - k_distances[i - 1];
        if (diff > max_diff) {
            max_diff = diff;
            best_index = i;
        }
    }

    return k_distances[best_index];
}

double evaluarSeparacion(const vector<Point>& puntos, const vector<int>& clusters) {
    if (puntos.empty()) return 0.0;
    int n = puntos.size();
    double intra = 0, inter = 1e9;
    int total = 0;

    for (int i = 0; i < n; ++i) {
        if (clusters[i] <= 0) continue;
        for (int j = i + 1; j < n; ++j) {
            if (clusters[j] <= 0) continue;
            double d = distance(puntos[i], puntos[j]);
            if (clusters[i] == clusters[j])
                intra += d;
            else
                inter = min(inter, d);
            total++;
        }
    }
    if (total == 0) return 0;
    intra /= total;
    return inter / (intra + 1e-6); // mayor es mejor
}

double estimarEps(vector<Point> puntos, int minPts = 4) {
    int n = puntos.size();
    vector<double> k_dist;
    for (int i = 0; i < n; ++i) {
        vector<double> d;
        for (int j = 0; j < n; ++j)
            if (i != j) d.push_back(distance(puntos[i], puntos[j]));
        sort(d.begin(), d.end());
        k_dist.push_back(d[minPts - 1]);
    }
    sort(k_dist.begin(), k_dist.end());

    vector<double> percentiles = {0.60, 0.65, 0.70, 0.75, 0.78, 0.80, 0.82, 0.85, 0.88, 0.90};

    double bestScore = -1;
    double bestEps = 0;

    for (double perc : percentiles) {
        double eps = k_dist[int(n * perc)];
        for (auto& p : puntos) { p.cluster = 0; p.visited = false; }

        dbscanKdTree(puntos, eps, minPts); // tu función ya implementada

        vector<int> clusters;
        for (auto& p : puntos) clusters.push_back(p.cluster);

        double score = evaluarSeparacion(puntos, clusters);
        if (score > bestScore) {
            bestScore = score;
            bestEps = eps;
        }
    }

    return bestEps;
}

int estimarMinPts(const vector<Point>& puntos, double eps, int muestras = 200) {
    if (puntos.empty()) return 4;

    int n = puntos.size();
    muestras = min(muestras, n);

    vector<int> vecinos;
    srand(42); // reproducible
    for (int i = 0; i < muestras; ++i) {
        int idx = rand() % n;
        int count = 0;
        for (int j = 0; j < n; ++j) {
            if (idx == j) continue;
            if (distance(puntos[idx], puntos[j]) <= eps)
                count++;
        }
        vecinos.push_back(count);
    }

    // Filtrar outliers (IQR)
    sort(vecinos.begin(), vecinos.end());
    int q1 = vecinos[muestras * 0.25];
    int q3 = vecinos[muestras * 0.75];
    double iqr = q3 - q1;

    vector<int> filtrados;
    for (int v : vecinos) {
        if (v >= q1 - 1.5 * iqr && v <= q3 + 1.5 * iqr)
            filtrados.push_back(v);
    }

    // Media filtrada
    double suma = 0;
    for (int v : filtrados) suma += v;
    int base_estimate = round(suma / filtrados.size());

    // === Análisis geométrico PCA ===
    Eigen::MatrixXd X(n, 2);
    for (int i = 0; i < n; ++i) {
        X(i, 0) = puntos[i].x;
        X(i, 1) = puntos[i].y;
    }

    Eigen::RowVector2d media = X.colwise().mean();
    Eigen::MatrixXd X_centered = X.rowwise() - media;
    Eigen::Matrix2d cov = (X_centered.transpose() * X_centered) / double(n - 1);
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> solver(cov);
    double lambda1 = solver.eigenvalues()(1);
    double lambda2 = solver.eigenvalues()(0);
    double elongacion = lambda1 / lambda2;

    // Bounding box
    double minX = puntos[0].x, maxX = puntos[0].x;
    double minY = puntos[0].y, maxY = puntos[0].y;
    for (const auto& p : puntos) {
        minX = min(minX, p.x); maxX = max(maxX, p.x);
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    double bbox_ratio = max((maxX - minX) / (maxY - minY), (maxY - minY) / (maxX - minX));

    int estimado_final;

    // Detectar casos especiales primero
    if (elongacion > 6.5 && bbox_ratio > 1.0 && bbox_ratio < 1.4) {
        // ANISO: muy alargado pero estrecho
        estimado_final = max(6, min(40, base_estimate / 2));
    } else if (elongacion > 6.5 && bbox_ratio > 1.4) {
        // VARIED: alargado y disperso
        estimado_final = max(150, base_estimate * 3);
    } else if (elongacion > 3.0 && elongacion < 6.0 && bbox_ratio > 1.6) {
        // MOONS
        estimado_final = max(30, min(60, base_estimate));
    } else if (elongacion < 2.0 && bbox_ratio < 1.5) {
        // BLOBS
        estimado_final = max(10, min(40, base_estimate));
    } else {
        // General
        double factor = 1.0 + min(1.5, max(0.0, (elongacion - 1.5) * 0.5));
        estimado_final = round(base_estimate * factor);
    }


    return max(4, min(estimado_final, static_cast<int>(sqrt(n) * 4)));
}
int detectarTipoDeDataset(const vector<Point>& puntos) {
    if (puntos.size() < 3) return 2;

    int n = puntos.size();
    Eigen::MatrixXd X(n, 2);
    for (int i = 0; i < n; ++i) {
        X(i, 0) = puntos[i].x;
        X(i, 1) = puntos[i].y;
    }

    // Centrar
    Eigen::RowVector2d media = X.colwise().mean();
    Eigen::MatrixXd X_centered = X.rowwise() - media;

    // PCA
    Eigen::Matrix2d cov = (X_centered.transpose() * X_centered) / double(n - 1);
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> solver(cov);
    auto eigenvalues = solver.eigenvalues();
    double lambda1 = eigenvalues(1); // mayor
    double lambda2 = eigenvalues(0); // menor
    double var_ratio = lambda1 / (lambda1 + lambda2);
    double elongacion = lambda1 / lambda2;

    // Bounding Box
    double minX = puntos[0].x, maxX = puntos[0].x;
    double minY = puntos[0].y, maxY = puntos[0].y;
    for (const auto& p : puntos) {
        minX = min(minX, p.x); maxX = max(maxX, p.x);
        minY = min(minY, p.y); maxY = max(maxY, p.y);
    }
    double bbox_ratio = max((maxX - minX) / (maxY - minY), (maxY - minY) / (maxX - minX));

    cout << "var_ratio = " << var_ratio
         << ", bbox_ratio = " << bbox_ratio
         << ", elongacion = " << elongacion << "\n";

    // CRITERIOS REFORZADOS
    if (var_ratio > 0.82 || elongacion > 1.4 || bbox_ratio > 1.6)
        return 2; // tipo 2: blobs, varied, aniso

    return 1; // tipo 1: círculos, lunas
}

pair<double, int> estimarParametrosAdaptativos(const vector<Point>& puntos) {
    int tipo = detectarTipoDeDataset(puntos);
    double eps;
    int minPts;

    if (tipo == 1) {
        eps = estimarEpsCodoMaximo(puntos, 4);
        minPts = 4;
    } else {
        minPts = estimarMinPts(puntos, 0.5);
        eps = estimarEps(puntos, minPts);
        minPts = estimarMinPts(puntos, eps);
    }
    
    //cout<<"TIPO : "<<tipo<<endl;
    cout<<"mejor epsilom: "<<eps<<" mejor min pts: "<<minPts<<endl;
    return {eps, minPts};
}

int main() {
    vector<string> filenames = {
        "datasets/noisy_circles.csv", "datasets/noisy_moons.csv", 
        "datasets/blobs.csv", "datasets/aniso.csv", "datasets/varied.csv"
    };

    double eps = 0.5;
    int minPts = 30;

    for (const auto& file : filenames) {
        auto pointsClassic = readCSV(file);
        pair<double, int> params = estimarParametrosAdaptativos(pointsClassic);
        cout << "Procesando: " << file << endl;
        double eps = params.first;
        int minPts = params.second;
        //eps = estimarEpsCodoMaximo(pointsClassic,minPts);
        //minPts = estimarMinPts(pointsClassic, eps);
        

        // DBSCAN clásico
        
        auto start1 = high_resolution_clock::now();
        dbscan(pointsClassic, eps, minPts);
        auto end1 = high_resolution_clock::now();
        auto duration1 = duration_cast<milliseconds>(end1 - start1).count();
        string outputClassic = file.substr(0, file.find('.')) + "_dbscan.csv";
        writeCSV(outputClassic, pointsClassic);
        cout << "DBSCAN clásico: " << duration1 << " ms -> " << outputClassic << endl;

        // DBSCAN KDTree
        auto pointsKD = readCSV(file);
        auto start2 = high_resolution_clock::now();
        dbscanKdTree(pointsKD, eps, minPts);
        auto end2 = high_resolution_clock::now();
        auto duration2 = duration_cast<milliseconds>(end2 - start2).count();
        string outputKD = file.substr(0, file.find('.')) + "_kdtree.csv";
        writeCSV(outputKD, pointsKD);
        cout << "DBSCAN KDTree:  " << duration2 << " ms -> " << outputKD << endl;

        cout << "--------------------------------------------\n";
    }

    return 0;
}