CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

test: tests/test_rstarlib.cpp rstartree.hpp indice_por_id.hpp grupos_por_hoja.hpp
	$(CXX) $(CXXFLAGS) tests/test_rstarlib.cpp -o tests/test_rstarlib
	./tests/test_rstarlib

ejemplo: ejemplo/ejemplo_taxis.cpp rstartree.hpp indice_por_id.hpp grupos_por_hoja.hpp
	$(CXX) $(CXXFLAGS) ejemplo/ejemplo_taxis.cpp -o ejemplo/ejemplo_taxis
	./ejemplo/ejemplo_taxis

clean:
	rm -f tests/test_rstarlib ejemplo/ejemplo_taxis

.PHONY: test ejemplo clean
