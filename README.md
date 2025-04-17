# A Novel Data Structure for Visualizing Similarities in Georeferenced Multidimensional Data

## Profesor
- Erick Mauricio Gomez Nieto

## Alumno
- Anthony Angel Briceño Quiroz

## 📌 Proyecto Final - CS312: Estructura de Datos Avanzada (2025-I)

Este proyecto tiene como objetivo el diseño e implementación de una estructura de datos personalizada para recuperar los datos más similares entre sí, a partir de datos georreferenciados con múltiples atributos numéricos. La estructura debe ser capaz de manejar eficientemente dos tipos de consultas, considerando siempre un área geográfica delimitada.

---

## 🎯 Objetivos

1. Implementar una estructura de datos desde cero, optimizada para consultas de similitud en datos multidimensionales georreferenciados.
2. Permitir dos tipos de consultas:
   - **Consulta directa (`k-NN`)**: dados los atributos de un punto específico, devolver los `k` puntos más similares dentro de un área geográfica delimitada.
   - **Agrupamiento global**: identificar y visualizar los grupos de puntos más similares entre sí sin necesidad de una consulta inicial.
3. Restringir siempre las consultas a una región definida por coordenadas geográficas (latitud y longitud mínima y máxima).
4. Utilizar únicamente atributos **numéricos** para el proceso de indexación y búsqueda.

---

## 🧠 Estructura de Datos Propuesta

La estructura de datos será completamente nueva y diseñada desde cero. Estará compuesta por tres componentes principales:

- **Filtro Geográfico**: Se utilizará una cuadrícula geoespacial personalizada para limitar rápidamente los datos a una región específica, basada en los límites de latitud y longitud definidos en cada consulta.
- **Índice Multidimensional**: Cada punto se representará como un vector de atributos numéricos. Se usará una técnica de normalización para que los atributos estén en la misma escala, y se almacenarán en una estructura optimizada para búsquedas por distancia (por ejemplo, un array de buckets por magnitud vectorial o un árbol binario modificado).
- **Módulo de Similitud y Agrupamiento**: Se usará la distancia euclidiana como métrica principal de similitud. Para la consulta sin punto dado, se implementará un algoritmo de clustering básico como una versión simplificada de DBSCAN o agrupamiento por vecindad densa.

---

## Contacto
- **Correo:** anthony.briceno@ucsp.edu.pe