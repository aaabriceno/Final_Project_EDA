import pandas as pd
import numpy as np

#np.random.uniform sirve para generar números aleatorios en un rango específico
#np.random.randint sirve para generar números enteros aleatorios en un rango específico
np.random.seed(42)


n = 1000000 #nuimero de datos aleatorios
# Generar datos aleatorios
#geografía
lati = np.random.uniform(-90, 90, n)
longi = np.random.uniform(-180, 180, n) 

latitudes = np.round(lati, 2)
longitudes = np.round(longi, 2)


data = pd.DataFrame({
    'latitud': latitudes,
    'longitud': longitudes
})

# Generar 20 atributos aleatorios
for i in range (1,21):
    data[f'attr{i}'] = np.random.randint(0, 200, n)
    
df = pd.DataFrame(data)
#Guardar el DataFrame en un archivo CSV
df.to_csv('Preprocesamiento/puntos.csv', index=False)
print("Archivo CSV generado con éxito.")