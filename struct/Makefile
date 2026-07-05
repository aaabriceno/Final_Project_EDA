CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Binario principal
geocluster: main.cpp GeoCluster.cpp GeoCluster.h
	$(CXX) $(CXXFLAGS) main.cpp GeoCluster.cpp -o geocluster

# Tests de conformidad con el paper R*-tree.
# Se compila con M=8, m=3 para ejercitar splits/reinserts con pocos puntos.
test: tests/test_rstar.cpp GeoCluster.cpp GeoCluster.h
	$(CXX) $(CXXFLAGS) -DGEO_MAX_ENTRIES=8 -DGEO_MIN_ENTRIES=3 tests/test_rstar.cpp GeoCluster.cpp -o tests/test_rstar
	./tests/test_rstar

clean:
	rm -f geocluster tests/test_rstar

.PHONY: test clean
