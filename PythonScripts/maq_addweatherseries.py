'''
    It only must add weather if: there is GPS data and power is estimated
    Delete temperature series, in case it exists
    Call to processor "Estimate Headwind Values"
'''

import meteostat as mt
import datetime
import pytz
import pandas as pd

from scipy.interpolate import interp1d

zona= "Europe/Madrid"

def main():

    GC.setTag('Error', '0')
    activity = GC.activity()

    if not ('altitude' in activity and 'longitude' in activity and 'latitude' in activity):
        return

    metrics = GC.activityMetrics()

    #if 'Virtual' in metrics['SubSport']:
    if not metrics['PotenciaEstimada']:
        return


    # Try to delete temperature series, in case it exists
    GC.deleteSeries(GC.SERIES_TEMP)

    act_date = metrics["date"]
    act_time = metrics["time"]

    # Standard activity data to get lat/long
    seconds_ride = list(GC.series(GC.SERIES_SECS))
    lat = GC.series(GC.SERIES_LAT)
    lon = GC.series(GC.SERIES_LON)
    alt = GC.series(GC.SERIES_ALT)

    # Get weather information once on center of your ride
    lat2 = [x for x in lat if x!=0]
    lon2 = [x for x in lon if x!=0]
    alt2 = [x for x in alt if x!=0]
    avg_lat = sum(lat2)/len(lat2)
    avg_lon = sum(lon2)/len(lon2)
    avg_alt = sum(alt2)/len(alt2)

    print("Date: " + act_date.strftime("%d/%m/%y"))
    print("Time: " + act_time.strftime("%H:%M"))
    print("LAT:" + str(avg_lat) + " LON: " + str(avg_lon) + " ALT: " + str(avg_alt))


    mt.Stations.cache_dir = '/tmp'
    mt.Hourly.cache_dir='/tmp'

    day = datetime.datetime.combine(date=act_date, time=datetime.time(hour=0,minute=0,second=0))
    print(mt.Stations().inventory('hourly',day).nearby(avg_lat, avg_lon, 20000).fetch())

    center_point = mt.Point(lat=avg_lat, lon=avg_lon, alt=avg_alt)
    pytzzona = pytz.timezone(zona)
    weather = mt.Hourly(center_point, day, day+datetime.timedelta(days=1), zona)
    weather.normalize()
    weather.interpolate()
    df = weather.fetch()

    print(df[['temp','wspd','wdir']])

    act_datetime = datetime.datetime.combine(date=act_date, time=act_time)

    # Interpolator functions
    weather_t = df.index.values.astype(datetime.datetime)/1000000000

    print('tiempos Weather: ')
    print([weather_t[i] for i in range(0,10)])

    f_temp = interp1d(weather_t, df['temp'])
    f_wspd = interp1d(weather_t, df['wspd'])
    f_wdir = interp1d(weather_t, df['wdir'], kind='nearest')

    # Seconds for XDataSeries: one sample per 600 seconds
    secs = [i for i in range(0, int(seconds_ride[-1]), 600)]
    # x axis as input of the interpolator functions
    times_ride = [(act_datetime + datetime.timedelta(seconds=secs[i])).timestamp() for i in range(len(secs))]


    # Se crea la serie compatible con el processor que estima la columna headwind a partir del viento y su dirección
    GC.createXDataSeries("WEATHER", "TEMPERATURE", "ºC")
    GC.createXDataSeries("WEATHER", "WINDSPEED", "kmh")
    GC.createXDataSeries("WEATHER", "WINDHEADING", "degrees")

    # Escritura en las columnas

    # Remove rows in data series, in case there are rows
    xdata = GC.xdataSeries("WEATHER","secs")
    for i in range(0, len(xdata)):
        xdata.remove(0)


    interp_temps = f_temp(times_ride)
    interp_wspd = f_wspd(times_ride)
    interp_wdir = f_wdir(times_ride)
    for i in range(0, len(times_ride)):
        GC.xdataSeries("WEATHER", "secs").append(secs[i])
        GC.xdataSeries("WEATHER", "TEMPERATURE")[-1] = interp_temps[i]
        GC.xdataSeries("WEATHER", "WINDSPEED")[-1] = interp_wspd[i]
        GC.xdataSeries("WEATHER", "WINDHEADING")[-1] = interp_wdir[i]


    # DataProcessor

    GC.postProcess("Estimate Headwind Values")


def manageError(e):
    print("EEEEE :", e.__class__)
    GC.setTag('Notes', GC.activityMetrics()['Notes'] + '\n ERROR in processing script AddWeatherSeries')
    GC.setTag('Error', '1')

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        manageError(e)
