import os
import time
import math
import scipy
import pickle
import warnings
import datetime
import numpy as np
import pandas as pd
import matplotlib
import seaborn as sns
import xgboost as xgb
import matplotlib.pylab as plt

# This library helps to open street map
import folium 

# Similar to pandas, but helps in parallel computing
# Below are some resources for getting familiar with Dask
# https://www.youtube.com/watch?v=ieW3G7ZzRZ0
# https://github.com/dask/dask-tutorial
# https://github.com/dask/dask-tutorial/blob/master/07_dataframe.ipynb
import dask.dataframe as dd

# This library is used while we calculate the straight line distance between two (lat, lon) pairs 
# in miles. Get the haversine distance
import gpxpy.geo 

# Used to determine the size of plots
from matplotlib import rcParams 

# https://pyroomacoustics.readthedocs.io/en/pypi-release/pyroomacoustics.doa.detect_peaks.html
# Used for detection of peaks
from pyroomacoustics.doa.detect_peaks import detect_peaks

# Download migwin: https://mingw-w64.org/doku.php/download/mingw-builds
# Install it in your system and keep the path, migw_path ='installed path'
# mingw_path = 'C:\\Program Files\\mingw-w64\\x86_64-5.3.0-posix-seh-rt_v4-rev0\\mingw64\\bin'
# os.environ['PATH'] = mingw_path + ';' + os.environ['PATH']

from sklearn.cluster import MiniBatchKMeans, KMeans
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_squared_error, mean_absolute_error
from sklearn.model_selection import GridSearchCV, RandomizedSearchCV
from sklearn.linear_model import LinearRegression

from prettytable import PrettyTable
from datetime import datetime as dt

matplotlib.use('nbagg');
warnings.filterwarnings("ignore");

# To find the running time of the entire kernel
globalstart = dt.now()
# Looking at the features
archivo = '2Database/2processed_data_complete_limpio.csv'
df = dd.read_csv(archivo)
df.columns
df.visualize()
df.fare_amount.sum().visualize()
# The table below shows few datapoints along with all our features
print("Columnas: ",df.columns)

print(df.head())


# 4. Filtrar outliers geográficos (bounding box de NYC)
# Ajusta los valores si tu área de interés es diferente
bbox = {
    "lat_min": 40.5774,
    "lat_max": 40.9176,
    "lon_min": -74.15,
    "lon_max": -73.7004
}


outliers_pickup = df[
    (df.pickup_longitude <= bbox["lon_min"]) | (df.pickup_latitude <= bbox["lat_min"]) |
    (df.pickup_longitude >= bbox["lon_max"]) | (df.pickup_latitude >= bbox["lat_max"])
].compute()
print("Número de outliers (pickup):", len(outliers_pickup))

# 5. Visualizar outliers en un mapa (solo los primeros 1000 para no saturar)
map_osm = folium.Map(location=[40.734695, -73.990372], tiles='Stamen Toner')
sample = outliers_pickup.head(1000)
for _, row in sample.iterrows():
    if row['pickup_latitude'] != 0:
        folium.Marker([row['pickup_latitude'], row['pickup_longitude']]).add_to(map_osm)
map_osm.save('3Preprocesamiento/img/outliers_pickup_map.html')
print("Mapa de outliers (pickup) guardado en: 3Preprocesamiento/img/outliers_pickup_map.html")

# 6. (Opcional) Filtrar el DataFrame para quedarte solo con los datos dentro del bounding box
df_clean = df[
    (df.pickup_longitude > bbox["lon_min"]) & (df.pickup_latitude > bbox["lat_min"]) &
    (df.pickup_longitude < bbox["lon_max"]) & (df.pickup_latitude < bbox["lat_max"])
]

# 7. Guardar el DataFrame limpio (opcional, solo si lo necesitas en disco)
df_clean.compute().to_csv('2Database/2processed_data_limpio_bbox.csv', index=False)
print("Archivo limpio guardado en: 2Database/2processed_data_limpio_bbox.csv")

# 8. Tiempo total de ejecución
print("Tiempo total de ejecución:", dt.now() - globalstart)
# Creating a map with a base location
# Read more about the folium here: http://python-visualization.github.io/folium/

# Note: We don't need to remember any of these, we don't need an in-depth knowledge on these 
# maps and plots

map_osm = folium.Map(location=[40.734695, -73.990372], tiles='Stamen Toner')

# We will spot only the first 1000 outliers on the map. Plotting all the outliers will take more time
sample_locations = outlier_locations.head(1000)
for i,j in sample_locations.iterrows():
    if int(j['pickup_latitude']) != 0:
        folium.Marker(list((j['pickup_latitude'],j['pickup_longitude']))).add_to(map_osm)

map_osm
# Plotting dropoff cordinates which are outside the bounding box of New-York 
# We will collect all the points outside the bounding box of NYC into outlier_locations
outlier_locations = df[((df.dropoff_longitude <= -74.15) | (df.dropoff_latitude <= 40.5774)| \
    (df.dropoff_longitude >= -73.7004) | (df.dropoff_latitude >= 40.9176))]
print("Number of Outlier Locations:", len(outlier_locations))

# Creating a map with a base location
# Read more about the folium here: http://python-visualization.github.io/folium/

# Note: We don't need to remember any of these, we don't need an in-depth knowledge on these 
# maps and plots

map_osm = folium.Map(location=[40.734695, -73.990372], tiles='Stamen Toner')

# We will spot only the first 1000 outliers on the map, plotting all the outliers will take more time
sample_locations = outlier_locations.head(1000)
for i,j in sample_locations.iterrows():
    if int(j['pickup_latitude']) != 0:
        folium.Marker(list((j['dropoff_latitude'],j['dropoff_longitude']))).add_to(map_osm)

map_osm
# The timestamps are converted to unix so as to get duration (trip-time) & speed.
# Also, the pickup-times in unix are used while binning 

# In the data, we have time in the format "YYYY-MM-DD HH:MM:SS" 
# We convert this string to python time format and then into unix time stamp
# https://stackoverflow.com/a/27914405
def convert_to_unix(s):
    return time.mktime(datetime.datetime.strptime(s, "%Y-%m-%d %H:%M:%S").timetuple())

# We return a data-frame which contains the columns
# 1.'passenger_count'   : Self-explanatory
# 2.'trip_distance'     : Self-explanatory
# 3.'pickup_longitude'  : Self-explanatory
# 4.'pickup_latitude'   : Self-explanatory
# 5.'dropoff_longitude' : Self-explanatory
# 6.'dropoff_latitude'  : Self-explanatory
# 7.'total_amount'      : Total fare that was paid
# 8.'trip_times'        : Duration of each trip
# 9.'pickup_times       : Pickup time converted into unix time 
# 10.'Speed'            : Velocity of each trip

def return_with_trip_times(df):
    duration = df[['tpep_pickup_datetime','tpep_dropoff_datetime']].compute()
    
    # Pickups and dropoffs to unix time
    duration_pickup = [convert_to_unix(x) for x in duration['tpep_pickup_datetime'].values]
    duration_drop = [convert_to_unix(x) for x in duration['tpep_dropoff_datetime'].values]
    
    # Calculate the duration of trips
    # Division by 60 converts the difference from seconds to minutes
    durations = (np.array(duration_drop) - np.array(duration_pickup))/float(60)

    # Append durations of trips and speed in miles/hr to a new dataframe
    new_frame = df[['passenger_count','trip_distance','pickup_longitude','pickup_latitude',
        'dropoff_longitude','dropoff_latitude','total_amount']].compute()
    
    new_frame['trip_times'] = durations
    new_frame['pickup_times'] = duration_pickup
    new_frame['Speed'] = 60*(new_frame['trip_distance']/new_frame['trip_times'])
    
    return new_frame

frame_with_durations = return_with_trip_times(df)
frame_with_durations.head()
# The skewed box-plot shows us the presence of outliers 
plt.figure(figsize = (8, 6))
sns.boxplot(y="trip_times", data = frame_with_durations)
# Calculating 0-100th percentile to find the correct percentile value for removal of outliers
var = frame_with_durations["trip_times"].values
var = np.sort(var, axis = None)

for i in range(0,100,10):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print ("100 percentile value is", var[-1])
# Looking further from the 99th percentile
var = frame_with_durations["trip_times"].values
var = np.sort(var,axis = None)

for i in range(90,100):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print ("100 percentile value is", var[-1])
# Removing data based on our analysis and TLC regulations
frame_with_durations_modified = frame_with_durations[(frame_with_durations.trip_times > 1) & \
    (frame_with_durations.trip_times < 720)]
frame_with_durations_modified.head()
# Box-plot after removal of outliers
plt.figure(figsize = (8, 6))
sns.boxplot(y="trip_times", data = frame_with_durations_modified)
# PDF of trip-times after removing the outliers
tt_sample = frame_with_durations_modified['trip_times'].sample(n = 200000)

plt.figure(figsize = (8, 6))
sns.distplot(tt_sample, hist=True, kde=True)
# Converting the values to log-values to check for log-normal distribution
frame_with_durations_modified['log_times'] = [math.log(i) for i in frame_with_durations_modified['trip_times'].values]

# PDF of log-values of trip-times
ltt_sample = frame_with_durations_modified['log_times'].sample(n = 200000)

plt.figure(figsize = (8, 6))
sns.distplot(ltt_sample, hist=True, kde=True)

# Q-Q plot for checking if trip-times follow the log-normal distribution
# https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.probplot.html
ltt_sample = frame_with_durations_modified['log_times'].sample(n = 200000)

plt.figure(figsize = (8, 6))
scipy.stats.probplot(ltt_sample, plot = plt)
# Check for any outliers in the data after trip duration outliers have been removed
# Box-plot for speeds with outliers
frame_with_durations_modified['Speed'] = 60 * (frame_with_durations_modified['trip_distance'] / \
    frame_with_durations_modified['trip_times'])

plt.figure(figsize = (8, 6))
sns.boxplot(y="Speed", data = frame_with_durations_modified)
# Calculating speed values at each percentile: 0,10,20,30,40,50,60,70,80,90,100 
var = frame_with_durations_modified["Speed"].values
var = np.sort(var,axis = None)
for i in range(0,100,10):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating speed values at each percentile: 90,91,92,93,94,95,96,97,98,99,100
var = frame_with_durations_modified["Speed"].values
var = np.sort(var,axis = None)
for i in range(90,100):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating speed values at each percentile: 99.0,99.1,99.2,99.3,99.4,99.5,99.6,99.7,99.8,99.9,100
var = frame_with_durations_modified["Speed"].values
var = np.sort(var,axis = None)
for i in np.arange(0.0, 1.0, 0.1):
    print("{} percentile value is {}".format(99+i,var[int(len(var)*(float(99+i)/100))]))
print("100 percentile value is ",var[-1])
# Removing further outliers based on the 99.9th percentile value
frame_with_durations_modified = frame_with_durations[(frame_with_durations.Speed > 0) & \
    (frame_with_durations.Speed < 45.31)]
# Average speed of cabs in New-York
sum(frame_with_durations_modified["Speed"]) / float(len(frame_with_durations_modified["Speed"]))
# Up to now, we have removed the outliers based on trip durations and cab speeds
# Let's try if there are any outliers in trip distances
# Box-plot showing outliers in trip-distance values

plt.figure(figsize = (8, 6))
sns.boxplot(y = "trip_distance", data = frame_with_durations_modified)
# Calculating trip distance values at each percentile: 0,10,20,30,40,50,60,70,80,90,100 
var = frame_with_durations_modified["trip_distance"].values
var = np.sort(var,axis = None)
for i in range(0,100,10):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating trip distance values at each percentile: 90,91,92,93,94,95,96,97,98,99,100
var = frame_with_durations_modified["trip_distance"].values
var = np.sort(var,axis = None)
for i in range(90,100):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating trip distance values at each percentile: 99.0,99.1,99.2,99.3,99.4,99.5,99.6,99.7,99.8,99.9,100
var = frame_with_durations_modified["trip_distance"].values
var = np.sort(var,axis = None)
for i in np.arange(0.0, 1.0, 0.1):
    print("{} percentile value is {}".format(99+i,var[int(len(var)*(float(99+i)/100))]))
print("100 percentile value is ",var[-1])
# Removing further outliers based on the 99.9th percentile value
frame_with_durations_modified=frame_with_durations[(frame_with_durations.trip_distance > 0) & \
    (frame_with_durations.trip_distance < 23)]
# Box-plot after removal of outliers
plt.figure(figsize = (8, 6))
sns.boxplot(y="trip_distance", data = frame_with_durations_modified)
# Up to now we have removed the outliers based on trip durations, cab speeds, and trip distances
# Let's try if there are any outliers in the total_amount
# Box-plot showing outliers in fare
plt.figure(figsize = (8, 6))
sns.boxplot(y="total_amount", data = frame_with_durations_modified)
# Calculating total fare amount values at each percentile: 0,10,20,30,40,50,60,70,80,90,100 
var = frame_with_durations_modified["total_amount"].values
var = np.sort(var,axis = None)
for i in range(0,100,10):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating total fare amount values at each percentile: 90,91,92,93,94,95,96,97,98,99,100
var = frame_with_durations_modified["total_amount"].values
var = np.sort(var,axis = None)
for i in range(90,100):
    print("{} percentile value is {}".format(i,var[int(len(var)*(float(i)/100))]))
print("100 percentile value is ",var[-1])
# Calculating total fare amount values at each percentile: 99.0,99.1,99.2,99.3,99.4,99.5,99.6,99.7,99.8,99.9,100
var = frame_with_durations_modified["total_amount"].values
var = np.sort(var,axis = None)
for i in np.arange(0.0, 1.0, 0.1):
    print("{} percentile value is {}".format(99+i,var[int(len(var)*(float(99+i)/100))]))
print("100 percentile value is ",var[-1])
# The below plot shows us the fare values (sorted) to find a sharp increase, to remove those values 
# as outliers. Plot the fare amount excluding the last two values in sorted data
plt.figure(figsize = (8, 6))
plt.plot(var[:-2])
plt.show()
# A very sharp increase in fare values can be seen. Plotting last three total fare values
# And we can observe that there is a shared increase in the values
plt.figure(figsize = (8, 6))
plt.plot(var[-3:])
plt.show()
# Now looking at values not including the last two points, we again find a drastic increase at 
# around 1000 fare values. We plot the last 50 values, excluding the last two values
plt.figure(figsize = (8, 6))
plt.plot(var[-50:-2])
plt.show()
# Now looking at values not including the last two points, we again find a drastic increase at 
# around 1000 fare values. We plot the last 50 values, excluding the last two values
plt.figure(figsize = (8, 6))
plt.plot(var[-50:-2])
plt.show()
print ("Removing outliers in the df of Jan-2015")
frame_with_durations_outliers_removed = remove_outliers(frame_with_durations)
print("Fraction of data points that remain after removing outliers", 
    float(len(frame_with_durations_outliers_removed)) / len(frame_with_durations))
    # Trying different cluster sizes to choose the right K in K-means
coords = frame_with_durations_outliers_removed[['pickup_latitude', 'pickup_longitude']].values
neighbours = []
 
def find_min_distance(cluster_centers, cluster_len):
    nice_points = 0
    wrong_points = 0
    less2 = []
    more2 = []
    # Stored & Calculated in miles
    min_dist = 1000
    
    for i in range(0, cluster_len):
        nice_points = 0
        wrong_points = 0
        for j in range(0, cluster_len):
            if j != i:
                distance = gpxpy.geo.haversine_distance(cluster_centers[i][0], cluster_centers[i][1],
                    cluster_centers[j][0], cluster_centers[j][1])
                # `distance` is in meters, so, we have converted it into miles
                min_dist = min(min_dist, distance/(1.60934*1000))
                if (distance/(1.60934*1000)) <= 2: nice_points += 1
                else: wrong_points += 1
        less2.append(nice_points)
        more2.append(wrong_points)
    neighbours.append(less2)
    print ("On choosing a cluster size of:", cluster_len, "\nAvg. #Clusters within the vicinity (i.e. intercluster-distance < 2):", 
        np.ceil(sum(less2)/len(less2)), "\nAvg. #Clusters outside the vicinity (i.e. intercluster-distance > 2):", 
        np.ceil(sum(more2)/len(more2)), "\nMin inter-cluster distance:", min_dist, "\n")

# https://scikit-learn.org/stable/modules/clustering.html#mini-batch-kmeans
def find_clusters(increment):
    kmeans = MiniBatchKMeans(n_clusters=increment, batch_size=10000, random_state=42).fit(coords)
    frame_with_durations_outliers_removed['pickup_cluster'] = kmeans.predict(frame_with_durations_outliers_removed[['pickup_latitude', 'pickup_longitude']])
    cluster_centers = kmeans.cluster_centers_
    cluster_len = len(cluster_centers)
    return cluster_centers, cluster_len

for increment in range(10, 100, 10):
    cluster_centers, cluster_len = find_clusters(increment)
    find_min_distance(cluster_centers, cluster_len)            
# Getting 40 clusters using K-Means
kmeans = MiniBatchKMeans(n_clusters=40, batch_size=10000, random_state=0).fit(coords)
frame_with_durations_outliers_removed['pickup_cluster'] = kmeans.predict(frame_with_durations_outliers_removed[['pickup_latitude', 'pickup_longitude']])
# Plotting the cluster centers on OSM
cluster_centers = kmeans.cluster_centers_
cluster_len = len(cluster_centers)
map_osm = folium.Map(location = [40.734695, -73.990372], tiles='Stamen Toner')
for i in range(cluster_len):
    folium.Marker(list((cluster_centers[i][0], cluster_centers[i][1])), 
        popup=(str(cluster_centers[i][0])+str(cluster_centers[i][1]))).add_to(map_osm)
map_osm
# Visualising the clusters on a map
def plot_clusters(frame):
    city_long_border = (-74.03, -73.75)
    city_lat_border = (40.63, 40.85)
    fig, ax = plt.subplots(ncols=1, nrows=1, figsize = (12, 10))
    ax.scatter(frame.pickup_longitude.values[:100000], frame.pickup_latitude.values[:100000], s=10, 
        lw=0, c=frame.pickup_cluster.values[:100000], cmap='tab20', alpha=0.2)
    ax.set_xlim(city_long_border)
    ax.set_ylim(city_lat_border)
    ax.set_xlabel('Longitude')
    ax.set_ylabel('Latitude')
    plt.show()

plot_clusters(frame_with_durations_outliers_removed)
def add_pickup_bins(frame, df, year):
    unix_pickup_times = [i for i in frame['pickup_times'].values]
    unix_times = [[1420070400,1422748800,1425168000,1427846400,1430438400,1433116800],\
                    [1451606400,1454284800,1456790400,1459468800,1462060800,1464739200]]
    start_pickup_unix = unix_times[year-2015][df-1]
    
    tenminutewise_binned_unix_pickup_times=[(int((i-start_pickup_unix)/600)+33) for i in unix_pickup_times]
    frame['pickup_bins'] = np.array(tenminutewise_binned_unix_pickup_times)
    return frame
# Clustering, making pickup bins and grouping by pickup cluster and pickup bins
jan_2015_frame = add_pickup_bins(frame_with_durations_outliers_removed,1,2015)
jan_2015_groupby = jan_2015_frame[['pickup_cluster','pickup_bins','trip_distance']] \
    .groupby(['pickup_cluster','pickup_bins']).count()
jan_2015_groupby = jan_2015_groupby.rename(columns = {'trip_distance': 'pickups'})
# Data Preparation for the months of Jan,Feb and March 2016
# Here, `df` refers to the Dask Dataframe
def datapreparation(df, kmeans, month_no, year_no):
    # Add the trip-times
    frame_with_durations = return_with_trip_times(df)
    
    # Remove the outliers
    frame_with_durations_outliers_removed = remove_outliers(frame_with_durations)
    print("\n")
    
    # Estimating Clusters
    frame_with_durations_outliers_removed['pickup_cluster'] = kmeans.predict(frame_with_durations_outliers_removed[['pickup_latitude', 'pickup_longitude']])

    # Performing the Group-By operation
    final_updated_frame = add_pickup_bins(frame_with_durations_outliers_removed, month_no, year_no)
    final_groupby_frame = final_updated_frame[['pickup_cluster','pickup_bins','trip_distance']].groupby(['pickup_cluster','pickup_bins']).count()
    final_groupby_frame = final_groupby_frame.rename(columns = {'trip_distance': 'pickups'})
    return final_updated_frame, final_groupby_frame
    
month_jan_2016 = dd.read_csv('../input/nyc-yellow-taxi-trip-data/yellow_tripdata_2016-01.csv')
month_feb_2016 = dd.read_csv('../input/nyc-yellow-taxi-trip-data/yellow_tripdata_2016-02.csv')
month_mar_2016 = dd.read_csv('../input/nyc-yellow-taxi-trip-data/yellow_tripdata_2016-03.csv')

jan_2016_frame, jan_2016_groupby = datapreparation(month_jan_2016, kmeans, 1, 2016)
feb_2016_frame, feb_2016_groupby = datapreparation(month_feb_2016, kmeans, 2, 2016)
mar_2016_frame, mar_2016_groupby = datapreparation(month_mar_2016, kmeans, 3, 2016)
def return_unq_pickup_bins(frame):
    values = []
    for i in range(0,40):
        new = frame[frame['pickup_cluster'] == i]
        list_unq = list(set(new['pickup_bins']))
        list_unq.sort()
        values.append(list_unq)
    return values
# For every df, we get all indices of 10-min intervals in which atleast one pickup happened
jan_2015_unique = return_unq_pickup_bins(jan_2015_frame)
jan_2016_unique = return_unq_pickup_bins(jan_2016_frame)
feb_2016_unique = return_unq_pickup_bins(feb_2016_frame)
mar_2016_unique = return_unq_pickup_bins(mar_2016_frame)
# For each cluster, number of 10-min intervals with 0 pickups
for i in range(40):
    print("For the", i, "th cluster, number of 10-min intervals with 0 pickups:", 4464 - len(set(jan_2015_unique[i])))
# For every df, we get all indices of 10-min intervals in which atleast one pickup happened
jan_2015_unique = return_unq_pickup_bins(jan_2015_frame)
jan_2016_unique = return_unq_pickup_bins(jan_2016_frame)
feb_2016_unique = return_unq_pickup_bins(feb_2016_frame)
mar_2016_unique = return_unq_pickup_bins(mar_2016_frame)
# For each cluster, number of 10-min intervals with 0 pickups
for i in range(40):
    print("For the", i, "th cluster, number of 10-min intervals with 0 pickups:", 4464 - len(set(jan_2015_unique[i])))
# Smoothing vs Filling
# Sample plot that shows two variations of filling missing values
# We have taken the #pickups for cluster region 2
plt.figure(figsize = (10,5))
plt.plot(jan_2015_fill[4464:8920], label="Filled with zeroes")
plt.plot(jan_2015_smooth[4464:8920], label="Filled with avg. values")
plt.legend()
plt.show()
# Jan-2015 data is smoothed, Jan, Feb & Mar 2016 data missing values are filled with zeroes
jan_2015_smooth = smoothing(jan_2015_groupby['pickups'].values, jan_2015_unique)
jan_2016_smooth = fill_missing(jan_2016_groupby['pickups'].values, jan_2016_unique)
feb_2016_smooth = fill_missing(feb_2016_groupby['pickups'].values, feb_2016_unique)
mar_2016_smooth = fill_missing(mar_2016_groupby['pickups'].values, mar_2016_unique)

# Making list of all the values of pickup data in every bin for a period of 3 months, 
# and storing them region-wise. 
# regions_cum: It will contain 40 lists, each list will contain 4464 + 4176 + 4464 values,
# which represents the #pickups, that have happened for three months in 2016 data
regions_cum = []

# Number of 10-min indices for Jan 2015: 24*31*60/10 = 4464
# Number of 10-min indices for Jan 2016: 24*31*60/10 = 4464
# Number of 10-min indices for Feb 2016: 24*29*60/10 = 4176
# Number of 10-min indices for Mar 2016: 24*31*60/10 = 4464

for i in range(0, 40):
    regions_cum.append(jan_2016_smooth[4464*i:4464*(i+1)] + feb_2016_smooth[4176*i:4176*(i+1)] + \
        mar_2016_smooth[4464*i:4464*(i+1)])

print(len(regions_cum), len(regions_cum[0]))

# A function to generate unique colors
def uniqueish_color():
    return plt.cm.gist_ncar(np.random.random())

first_x = list(range(0,4464))
second_x = list(range(4464,8640))
third_x = list(range(8640,13104))

for i in range(40):
    plt.figure(figsize = (10,4))
    plt.plot(first_x,regions_cum[i][:4464], color = uniqueish_color(), label='Jan 2016')
    plt.plot(second_x,regions_cum[i][4464:8640], color = uniqueish_color(), label='Feb 2016')
    plt.plot(third_x,regions_cum[i][8640:], color = uniqueish_color(), label='Mar 2016')
    plt.legend()
    plt.show()

Y = np.fft.fft(np.array(jan_2016_smooth)[0:4460])
freq = np.fft.fftfreq(4460, 1)
n = len(freq)
plt.figure()
plt.plot(freq[:int(n/2)], np.abs(Y)[:int(n/2)])
plt.xlabel("Frequency")
plt.ylabel("Amplitude")
plt.show()

indexes_jan2016 = detect_peaks(regions_cum[0][:4464], mph = 200, show=True)
indexes_feb2016 = detect_peaks(regions_cum[0][4464:8640], mph = 200, show=True)
indexes_mar2016 = detect_peaks(regions_cum[0][8640:], mph = 200, show=True)

def extract_fft_features(cluster, df):
    """
    The `fft` function when returns the DFT, the sampling frequencies are in the form of 
    +- f1, -+ f2, and so on. So, when we will be considering the 3 frequencies corresponding to
    the 3 highest peaks, we will consider the positive counterparts only. (If you want, you can 
    consider the negative counterparts only, as well)
    """
    
    # Creating an empty list for the 4 features
    features = []
    
    if df == 0:
        Y = np.fft.fft(regions_cum[cluster][0:4464])
        freq = np.fft.fftfreq(4464, 1)
    elif df == 1:
        Y = np.fft.fft(regions_cum[cluster][4464:8640])
        freq = np.fft.fftfreq(4176, 1)
    elif df == 2:
        Y = np.fft.fft(regions_cum[cluster][8640:])
        freq = np.fft.fftfreq(4464, 1)
    
    # `indexes` contains the indices of the peaks
    indexes = detect_peaks(Y, mph = 1000, show = True)
    
    # Number of peaks is the first feature
    features.append(len(indexes))
    
    if len(indexes) != 0:
        # Getting the Amplitude and Frequency of the Peaks using the `indexes`
        amp_peaks = np.abs(Y[indexes])
        freq_peaks = freq[indexes]

        # Getting the Indices which would sort the amp_peaks array
        # Since the Indices sort the array in the Increasing Order, hence reversing the array of indices
        sorted_ind = np.argsort(amp_peaks)
        sorted_ind = sorted_ind[ : :-1]

        # Contains the frequencies of the Highest Peaks in order
        high_peak_freq = np.take(freq_peaks, sorted_ind)
        
        for i in range(0, 6, 2):
            if i < len(high_peak_freq): features.append(np.abs(high_peak_freq[i]))
            else: features.append(0)

    else: features.extend([0, 0, 0])
    return features
    
    
# Creating an empty numpy array, sto store the FFT-based features
fft_feat = np.empty([40, 13104, 4])    

for i in range(40):
    features = extract_fft_features(i, 0)
    fft_feat[i][:4464] = features
    features = extract_fft_features(i, 1)
    fft_feat[i][4464:8640] = features
    features = extract_fft_features(i, 2)
    fft_feat[i][8640:] = features
# Preparing the Dataframe only with x(i) values as Jan 2015 data and y(i) values as Jan 2016
ratios_jan = pd.DataFrame()
ratios_jan['Given'] = jan_2015_smooth
ratios_jan['Prediction'] = jan_2016_smooth
ratios_jan['Ratios'] = ratios_jan['Prediction']*1.0 / ratios_jan['Given']*1.0
def MA_R_Predictions(ratios):
    predicted_ratio = (ratios['Ratios'].values)[0]
    error, predicted_values, predicted_ratio_values = [], [], []
    window_size = 3
    for i in range(0, 4464*40):
        if i % 4464 == 0:
            predicted_ratio_values.append(0)
            predicted_values.append(0)
            error.append(0)
            continue
        predicted_val = int(((ratios['Given'].values)[i])*predicted_ratio)
        predicted_ratio_values.append(predicted_ratio)
        predicted_values.append(predicted_val)
        error.append( abs(predicted_val-(ratios['Prediction'].values)[i]) )
        if i+1 >= window_size:
            predicted_ratio = sum((ratios['Ratios'].values)[(i+1)-window_size:(i+1)]) / window_size
        else:
            predicted_ratio = sum((ratios['Ratios'].values)[0:(i+1)]) / (i+1)
            
    ratios['MA_R_Predicted'] = predicted_values
    ratios['MA_R_Error'] = error
    
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error]) / len(error)
    return ratios, mape_err, mse_err
def MA_P_Predictions(ratios):
    predicted_value = (ratios['Prediction'].values)[0]
    error, predicted_values = [], []
    window_size = 1
    for i in range(0, 4464*40):
        predicted_values.append(predicted_value)
        error.append(abs(predicted_value-(ratios['Prediction'].values)[i]))
        if i+1 >= window_size:
            predicted_value = int(sum((ratios['Prediction'].values)[(i+1)-window_size:(i+1)]) / window_size)
        else:
            predicted_value = int(sum((ratios['Prediction'].values)[0:(i+1)]) / (i+1))
            
    ratios['MA_P_Predicted'] = predicted_values
    ratios['MA_P_Error'] = error
    
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error]) / len(error)
    return ratios, mape_err, mse_err
def MA_P_Predictions(ratios):
    predicted_value = (ratios['Prediction'].values)[0]
    error, predicted_values = [], []
    window_size = 1
    for i in range(0, 4464*40):
        predicted_values.append(predicted_value)
        error.append(abs(predicted_value-(ratios['Prediction'].values)[i]))
        if i+1 >= window_size:
            predicted_value = int(sum((ratios['Prediction'].values)[(i+1)-window_size:(i+1)]) / window_size)
        else:
            predicted_value = int(sum((ratios['Prediction'].values)[0:(i+1)]) / (i+1))
            
    ratios['MA_P_Predicted'] = predicted_values
    ratios['MA_P_Error'] = error
    
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error]) / len(error)
    return ratios, mape_err, mse_err
def WA_P_Predictions(ratios):
    predicted_value=(ratios['Prediction'].values)[0]
    error, predicted_values = [], []
    window_size = 2
    for i in range(0, 4464*40):
        predicted_values.append(predicted_value)
        error.append(abs(predicted_value-(ratios['Prediction'].values)[i]))
        
        sum_values = 0
        sum_of_coeff = 0
        if i+1 >= window_size:
            for j in range(window_size, 0, -1):
                sum_values += j*(ratios['Prediction'].values)[i-window_size+j]
                sum_of_coeff += j

        else:
            for j in range(i+1,0,-1):
                sum_values += j*(ratios['Prediction'].values)[j-1]
                sum_of_coeff+=j
        
        predicted_value = int(sum_values / sum_of_coeff)
    
    ratios['WA_P_Predicted'] = predicted_values
    ratios['WA_P_Error'] = error
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error])/len(error)
    return ratios, mape_err, mse_err
def EA_R1_Predictions(ratios):
    predicted_ratio=(ratios['Ratios'].values)[0]
    alpha = 0.6
    error, predicted_values, predicted_ratio_values = [], [], []
    for i in range(0, 4464*40):
        if i % 4464 == 0:
            predicted_ratio_values.append(0)
            predicted_values.append(0)
            error.append(0)
            continue
        predicted_val = int(((ratios['Given'].values)[i])*predicted_ratio)
        predicted_ratio_values.append(predicted_ratio)
        predicted_values.append(predicted_val)
        error.append(abs((predicted_val - (ratios['Prediction'].values)[i])))
        predicted_ratio = (alpha*predicted_ratio) + (1-alpha)*((ratios['Ratios'].values)[i])
    
    ratios['EA_R1_Predicted'] = predicted_values
    ratios['EA_R1_Error'] = error
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error])/len(error)
    return ratios, mape_err, mse_err
def EA_P1_Predictions(ratios):
    predicted_value= (ratios['Prediction'].values)[0]
    alpha = 0.3
    error, predicted_values = [], []
    for i in range(0, 4464*40):
        if i % 4464 == 0:
            predicted_values.append(0)
            error.append(0)
            continue
        predicted_values.append(predicted_value)
        error.append(abs((predicted_value-(ratios['Prediction'].values)[i])))
        predicted_value =int((alpha*predicted_value) + (1-alpha)*((ratios['Prediction'].values)[i]))
    
    ratios['EA_P1_Predicted'] = predicted_values
    ratios['EA_P1_Error'] = error
    avg_pred_val = (sum(ratios['Prediction'].values) / len(ratios['Prediction'].values))
    mape_err = (1 / len(error)) * (sum(error) / avg_pred_val)
    mse_err = sum([e**2 for e in error])/len(error)
    return ratios, mape_err, mse_err
mape_err = [0]*10
mse_err = [0]*10
ratios_jan, mape_err[0], mse_err[0] = MA_R_Predictions(ratios_jan)
ratios_jan, mape_err[1], mse_err[1] = MA_P_Predictions(ratios_jan)
ratios_jan, mape_err[2], mse_err[2] = WA_R_Predictions(ratios_jan)
ratios_jan, mape_err[3], mse_err[3] = WA_P_Predictions(ratios_jan)
ratios_jan, mape_err[4], mse_err[4] = EA_R1_Predictions(ratios_jan)
ratios_jan, mape_err[5], mse_err[5] = EA_P1_Predictions(ratios_jan)
x = PrettyTable()

x.field_names = ["Baseline Model", "MAPE", "MSE"]
x.add_rows([
    ["Simple Moving Averages (Ratios)", mape_err[0], mse_err[0]],
    ["Simple Moving Averages (2016 Values)", mape_err[1], mse_err[1]],
    ["Weighted Moving Averages (Ratios)", mape_err[2], mse_err[2]],
    ["Weighted Moving Averages (2016 Values)", mape_err[3], mse_err[3]],
    ["Exponential Weighted Moving Averages (Ratios)", mape_err[4], mse_err[4]],
    ["Exponential Weighted Moving Averages (2016 Values)", mape_err[5], mse_err[5]]
])

print ("Error Metric Matrix (Forecasting Methods) - MAPE & MSE")
print(x)
# Number of 10-min indices for Jan 2016: 24*31*60/10 = 4464
# Number of 10-min indices for Feb 2016: 24*29*60/10 = 4176
# Number of 10-min indices for Mar 2016: 24*31*60/10 = 4464
# regions_cum: It will contain 40 lists, each list will contain (4464+4176+4464) = 13104 values 
# which represents the #pickups that have happened for three months in 2016.

# Since, we are considering the #pickups for the last 5 time-bins, hence, we are omitting the 
# first 5 time bins from our dataframe, and therefore, our prediction starts from 5th 10-min interval


# We take #pickups that have happened in last 5 10-min intervals
number_of_time_stamps = 5

# It is a list of lists
# It will contain 13099 #pickups for each cluster
output = []

# tsne_lat will contain 13099 times latitude of cluster center for every cluster
# Ex: [[cent_lat 13099times], [cent_lat 13099times], [cent_lat 13099times] .... 40 lists]
# It is a list of lists
tsne_lat = []


# tsne_lon will contain 13099 times longitude of cluster center for every cluster
# Ex: [[cent_long 13099times], [cent_long 13099times], [cent_long 13099times] .... 40 lists]
# It is a list of lists
tsne_lon = []

# We will code each day as below
# Sun = 0, Mon = 1, Tue = 2, Wed = 3, Thu = 4, Fri = 5, Sat = 6
# For every cluster, we will be adding 13099 values, each value represent to which day of 
# the week that pickup bin belongs to.
# It is a list of lists
tsne_weekday = []

# It's a numpy array of shape (40 * 13099, 5) = (523960, 5)
# Each row corresponds to an entry in out data
# For the first row we will have [f0,f1,f2,f3,f4], where fi = #pickups happened in (i+1)th 
# 10-min interval (bin). The second row will have [f1,f2,f3,f4,f5]
# The third row will have [f2,f3,f4,f5,f6], and so on...
tsne_feature = []
tsne_feature = [0] * number_of_time_stamps

# Jan 2016 is Thursday, so we start our day from 4: "(int(k/144))%7+4"
# regions_cum is a list of lists 
# [[x1,x2,x3..x13104], [x1,x2,x3..x13104], [x1,x2,x3..x13104], ... 40 lists]

for i in range(0, 40):
    tsne_lat.append([kmeans.cluster_centers_[i][0]]*13099)
    tsne_lon.append([kmeans.cluster_centers_[i][1]]*13099)
    tsne_weekday.append([int(((int(k/144))%7+4)%7) for k in range(5,4464+4176+4464)])
    tsne_feature = np.vstack((tsne_feature, [
        regions_cum[i][r : r + number_of_time_stamps] \
        for r in range(0, len(regions_cum[i]) - number_of_time_stamps)
    ]))
    output.append(regions_cum[i][5:])

# Removing the first dummy row
tsne_feature = tsne_feature[1:]

print(len(tsne_lat[0])*len(tsne_lat))
print(tsne_feature.shape[0])
print(len(tsne_weekday)*len(tsne_weekday[0]))
print(len(output)*len(output[0]))
# Adding the FFT-Based Features, Removing the first 5 rows for each of the clusters
# fft_feat_rem5 = fft_feat[ : ,5: , :]
# print(fft_feat_rem5.shape)

# fft_feat_prep = fft_feat_rem5.reshape((523960, 4))
# print(fft_feat_prep.shape)

# tsne_feature = np.hstack([tsne_feature, fft_feat_prep])
# print(tsne_feature.shape)
# From the baseline models we said that the EWMA gives us the least error
# We will try to add the same EWMA at t as a feature to our data
# EWMA => P'(t) = alpha * P'(t-1) + (1-alpha) * P(t-1) 
alpha = 0.3

# It is a temporary array that stores EWMA for each 10-min interval, 
# For each cluster it will get reset. For every cluster it contains 13104 values
predicted_values = []

# It is similar like tsne_lat
# It is a list of lists
# [[x5,x6,x7..x13104], [x5,x6,x7..x13104], [x5,x6,x7..x13104], .. 40 lists]
predict_list = []
tsne_flat_exp_avg = []

for r in range(0,40):
    for i in range(0,13104):
        if i==0:
            predicted_value = regions_cum[r][0]
            predicted_values.append(0)
            continue
        predicted_values.append(predicted_value)
        predicted_value = int((alpha*predicted_value) + (1-alpha)*(regions_cum[r][i]))
    predict_list.append(predicted_values[5:])
    predicted_values = []
# train-test split: 70%-30% split
print("Size of train data:", int(13099*0.7))
print("Size of test data:", int(13099*0.3))

# Extracting first 9169 timestamp values i.e., 70% of 13099 (total timestamps) for our training data
train_features =  [tsne_feature[13099*i:(13099*i+9169)] for i in range(0,40)]
# temp = [0]*(12955 - 9068)
test_features = [tsne_feature[13099*i+9169:13099*(i+1)] for i in range(0,40)]
print("Number of Data-clusters:", len(train_features), "\nNumber of data points in train data", 
    len(train_features[0]), "\nEach data-point contains", len(train_features[0][0]),"features")
print("\n")
print("Number of Data-clusters:", len(test_features), "\nNumber of data points in test data", 
    len(test_features[0]), "\nEach data-point contains", len(test_features[0][0]), "features")
    # Extracting first 9169 timestamp values i.e., 70% of 13099 (total timestamps) for our training data
tsne_train_flat_lat = [i[:9169] for i in tsne_lat]
tsne_train_flat_lon = [i[:9169] for i in tsne_lon]
tsne_train_flat_weekday = [i[:9169] for i in tsne_weekday]
tsne_train_flat_output = [i[:9169] for i in output]
tsne_train_flat_exp_avg = [i[:9169] for i in predict_list]
# Extracting the rest of the timestamp values i.e., 30% of 13099 (total timestamps) for our test data
tsne_test_flat_lat = [i[9169:] for i in tsne_lat]
tsne_test_flat_lon = [i[9169:] for i in tsne_lon]
tsne_test_flat_weekday = [i[9169:] for i in tsne_weekday]
tsne_test_flat_output = [i[9169:] for i in output]
tsne_test_flat_exp_avg = [i[9169:] for i in predict_list]
# The above variables contain values in the form of list of lists (i.e., list of values of 
# each region), here we make all of them in one list.
train_new_features = []
for i in range(0, 40):
    train_new_features.extend(train_features[i])
    
test_new_features = []
for i in range(0, 40):
    test_new_features.extend(test_features[i])
# Converting lists of lists into a single list i.e., flatten
# a  = [[1,2,3,4],[4,6,7,8]]
# print(sum(a,[]))
# [1, 2, 3, 4, 4, 6, 7, 8]

tsne_train_lat = sum(tsne_train_flat_lat, [])
tsne_train_lon = sum(tsne_train_flat_lon, [])
tsne_train_weekday = sum(tsne_train_flat_weekday, [])
tsne_train_output = sum(tsne_train_flat_output, [])
tsne_train_exp_avg = sum(tsne_train_flat_exp_avg,[])
# Converting lists of lists into a single list i.e., flatten
# a  = [[1,2,3,4],[4,6,7,8]]
# print(sum(a,[]))
# [1, 2, 3, 4, 4, 6, 7, 8]

tsne_test_lat = sum(tsne_test_flat_lat, [])
tsne_test_lon = sum(tsne_test_flat_lon, [])
tsne_test_weekday = sum(tsne_test_flat_weekday, [])
tsne_test_output = sum(tsne_test_flat_output, [])
tsne_test_exp_avg = sum(tsne_test_flat_exp_avg,[])
# Preparing the data-frame for our train data

# Considering FFT-Based Features
# columns = ['ft_5','ft_4','ft_3','ft_2','ft_1', 'len_peaks', 'freq_1', 'freq_2', 'freq_3']
# Not Considering FFT-Based Features
columns = ['ft_5','ft_4','ft_3','ft_2','ft_1']

df_train = pd.DataFrame(data = train_new_features, columns = columns) 
df_train['lat'] = tsne_train_lat
df_train['lon'] = tsne_train_lon
df_train['weekday'] = tsne_train_weekday
df_train['exp_avg'] = tsne_train_exp_avg
print(df_train.shape)
df_train.head()
# Preparing the data-frame for our test data
df_test = pd.DataFrame(data=test_new_features, columns=columns) 
df_test['lat'] = tsne_test_lat
df_test['lon'] = tsne_test_lon
df_test['weekday'] = tsne_test_weekday
df_test['exp_avg'] = tsne_test_exp_avg
print(df_test.shape)
df_test.head()
# No hyper-parameter tuning to do
lr_reg = LinearRegression().fit(df_train, tsne_train_output)
y_pred = lr_reg.predict(df_train)
lr_train_predictions = [round(value) for value in y_pred]
y_pred = lr_reg.predict(df_test)
lr_test_predictions = [round(value) for value in y_pred]
# params = {
#     'max_features': ['auto', 'sqrt', 'log2'],
#     'min_samples_leaf': [1, 2, 3, 4],
#     'min_samples_split': [1, 2, 3, 4],
#     'n_estimators': [i for i in range(0, 101, 20)],
#     'n_jobs': [-1]
# }
# rfr = RandomForestRegressor()
# model = RandomizedSearchCV(rfr, params, verbose = 1)
# model.fit(df_train, tsne_train_output)
# print(model.best_params_)

# Best Params found using RandomizedSearch
# {'n_jobs': -1, 'n_estimators': 40, 'min_samples_split': 3, 
# 'min_samples_leaf': 4, 'max_features': 'sqrt'}
# Training the Model with the best hyper-parameters found using the above Randomized Search
regr1 = RandomForestRegressor(max_features='sqrt', min_samples_leaf = 4,
    min_samples_split = 3, n_estimators = 40, n_jobs = -1)
regr1.fit(df_train, tsne_train_output)
# Predicting on train & test data using our trained Random Forest model 
y_pred = regr1.predict(df_train)
rndf_train_predictions = [round(value) for value in y_pred]
y_pred = regr1.predict(df_test)
rndf_test_predictions = [round(value) for value in y_pred]
# Feature importances based on analysis using Random Forest
print (df_train.columns)
print (regr1.feature_importances_)
# params = {
#     'learning_rate': [0.01, 0.1, 1],
#     'n_estimators': [i for i in range(1, 1001, 250)],
#     'max_depth': [2, 3, 4],
#     'min_child_weight': [2, 3, 4],
#     'gamma': [0, 0.1, 0.5],
#     'subsample': [0.5, 0.8, 1],
#     'reg_alpha': [100, 200],
#     'reg_lambda': [100, 200],
#     'colsample_bytree': [0.4, 0.8, 1.0],
#     'n_jobs': [-1]
# }
# xgbr = xgb.XGBRegressor()
# model = RandomizedSearchCV(xgbr, params, verbose = 2)
# model.fit(df_train, tsne_train_output)
# print(model.best_params_)

# Best Params found using RandomizedSearch
# {'subsample': 0.8, 'reg_lambda': 200, 'reg_alpha': 200, 'n_jobs': -1, 
# 'n_estimators': 1000, 'min_child_weight': 3, 'max_depth': 3, 'learning_rate': 0.1, 
# 'gamma': 0, 'colsample_bytree': 0.8}
# Training the Model with the best hyper-parameters found using the above Randomized Search
x_model = xgb.XGBRegressor(
    learning_rate=0.1, n_estimators=1000, max_depth=3, min_child_weight=3,
    gamma=0, subsample=0.8, reg_alpha=200, reg_lambda=200, colsample_bytree=0.8, n_jobs=-1
)
x_model.fit(df_train, tsne_train_output)
# Predicting on train & test data using our trained XgBoost regressor model
y_pred = x_model.predict(df_train)
xgb_train_predictions = [round(value) for value in y_pred]
y_pred = x_model.predict(df_test)
xgb_test_predictions = [round(value) for value in y_pred]
# Feature importances based on analysis using XgBoost
print (df_train.columns)
print(x_model.feature_importances_)
train_mape = []
test_mape = []

train_mape.append((mean_absolute_error(tsne_train_output,df_train['ft_1'].values)) / (sum(tsne_train_output)/len(tsne_train_output)))
train_mape.append((mean_absolute_error(tsne_train_output,df_train['exp_avg'].values)) / (sum(tsne_train_output)/len(tsne_train_output)))
train_mape.append((mean_absolute_error(tsne_train_output,rndf_train_predictions)) / (sum(tsne_train_output)/len(tsne_train_output)))
train_mape.append((mean_absolute_error(tsne_train_output, xgb_train_predictions)) / (sum(tsne_train_output)/len(tsne_train_output)))
train_mape.append((mean_absolute_error(tsne_train_output, lr_train_predictions)) / (sum(tsne_train_output)/len(tsne_train_output)))

test_mape.append((mean_absolute_error(tsne_test_output, df_test['ft_1'].values)) / (sum(tsne_test_output)/len(tsne_test_output)))
test_mape.append((mean_absolute_error(tsne_test_output, df_test['exp_avg'].values)) / (sum(tsne_test_output)/len(tsne_test_output)))
test_mape.append((mean_absolute_error(tsne_test_output, rndf_test_predictions)) / (sum(tsne_test_output)/len(tsne_test_output)))
test_mape.append((mean_absolute_error(tsne_test_output, xgb_test_predictions)) / (sum(tsne_test_output)/len(tsne_test_output)))
test_mape.append((mean_absolute_error(tsne_test_output, lr_test_predictions)) / (sum(tsne_test_output)/len(tsne_test_output)))
print("Time taken to run this entire kernel is:", dt.now() - globalstart)