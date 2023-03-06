import meteostat as mt
import datetime
import pytz

zona= "Europe/Madrid"

def main():
    # Weather XData series, compatible with FIT Importer
    GC.createXDataSeries("WEATHER", "TEMPERATURE", "celsius")
    GC.createXDataSeries("WEATHER", "WINDSPEED", "kmh")
    GC.createXDataSeries("WEATHER", "WINDDIRECTION", "degrees")

    act_date = GC.getTag("date")
    act_time = GC.getTag("time")
    act_duration = GC.getTag("Duration")

    # Standard activity data to get lat/long
    seconds = list(GC.series(GC.SERIES_SECS))
    lat = GC.series(GC.SERIES_LAT)
    lon = GC.series(GC.SERIES_LON)
    alt = GC.series(GC.SERIES_ALT)

    # Get weather information once on center or your ride
    avg_lat = (sum(lat)/len(lat))
    avg_lon = (sum(lon)/len(lon))
    avg_alt = (sum(alt)/len(alt))

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

    act_datetime = datetime.datetime.combine(date=act_date, time=act_time)
    i=1
    duration_in_ride=0
    while duration_in_ride <= act_duration:
        next_hour=datetime.datetime(act_date.year, act_date.month, act_date.day, act_time.hour + i, 0)
        duration_in_ride = (next_hour - act_datetime).seconds
        if duration_in_ride <= act_duration:
            try:
                record = df.loc[pytzzona.localize(next_hour)]
                print(record)
                GC.xdataSeries("WEATHER", "secs").append(duration_in_ride)
                index = len(GC.xdataSeries("WEATHER", "secs"))-1
                GC.xdataSeries("WEATHER", "TEMPERATURE")[index] = record['temp']
                GC.xdataSeries("WEATHER", "WINDSPEED")[index] = record['wspd']
                GC.xdataSeries("WEATHER", "WINDDIRECTION")[index] = record['wdir']
            except:
                print('NO WEATHER RECOR FOUND AT ' + str(next_hour))
        else:
            print("Next retrial is outside ride so skip")
        i += 1


if __name__ == "__main__":
    main()
