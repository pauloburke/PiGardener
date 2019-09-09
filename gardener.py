# -*- coding: utf-8 -*-

"""
# Configuration file to Pi Gardener

#Pump flux in L/min
water_flux = 3. 

#Default volume to irrigation in Liters. It may depends on the type of plant.
default_water_dispensing = 1.5

#Irrigation times in 24h format separated by commas (eg. 06:00,18:00 )
watering_times = 06:00,19:51

#----------------------------------------------------------------

#Use weather forecast data? (True/False)
use_weather_data = True

#OpenWeatherMaps API key
api_key = insert_your_OWM_API_key

#City codes file from OWM link (Do not change if it's working)
city_codes_url = http://bulk.openweathermap.org/sample/city.list.json.gz

#Current city (If city not found, try with/without accents)
city = Sao Carlos

#Current country two letter code
country = BR

#Daily time to update weather forecast data
weather_update_time = 00:01

"""

import sys, os.path
import time
import datetime
import gzip
import requests, json
import schedule
import logging



#Change this function to modify weather based irrigation volume.
def weather_based_volume_function(default_volume,humidity,temperature):
    return default_volume*(1.+(1.-(humidity/90.)))*(temperature/25.)


def liter_to_time(L,wf): #seconds
    return L/(wf/60.)

def update_weather_forecast_data(city_id,key,max_tries=60):
    base_url = "http://api.openweathermap.org/data/2.5/forecast?"      
    complete_url = base_url + "appid=" + key + "&id=" + str(city_id) + "&units=metric"
    http_status_code = 404
    owm_status_code = "404"
    tries = 0
    logger.info("Getting weather forecast data from OpenWeatherMap...")
    while (owm_status_code == "404" or http_status_code!=200) and tries<max_tries:
        response = requests.get(complete_url)
        http_status_code = response.status_code
        if http_status_code == 200:
            data = response.json()
            owm_status_code = data["cod"]
        if owm_status_code!="200":
            time.sleep(1.5)
        logger.info("Try #"+str(tries+1)+" of "+str(max_tries))
        tries += 1
    try:
        if owm_status_code == "200" and tries<max_tries:
            with open("weather.data","w") as f:
                json.dump(data,f)
            global last_weather_update
            last_weather_update = time.time()
            logger.info("Weather forecast data updated.")
            return 1
        else:
            logger.warning("Warning: Failed to update weather forecast data. (http_request code: "+str(http_status_code)+", Owm Code: "+srt(owm_status_code)+")")
            return 0
    except:
        logger.error("Error: Failed to write weather forecast data.")
        return 0    

def get_city_codes(url,max_tries=60):
    http_status_code = 404
    tries = 0
    logger.info("Getting OpenWeatherMap city codes file...")
    while http_status_code!=200 and tries<max_tries:
        response = requests.get(url, allow_redirects=True)
        http_status_code = response.status_code
        logger.info("Try #"+str(tries+1)+" of "+str(max_tries))
        tries += 1
    if tries>=max_tries:
        logger.critical("Critical Error: Failed to obtain city codes file from OpenWeatherMap. (http_request code: "+str(http_status_code)+")")
        sys.exit()
    try:
        with open('city.list.json.gz', 'wb') as f:
            f.write(response.content)
        logger.info("Download done.")
    except:
        logger.critical("Critical Error: Failed to write city codes file. Check script permissions.")
        sys.exit()

def find_city_code(city,country):
    logger.info("Looking for city ID...")
    with gzip.GzipFile("city.list.json.gz", 'r') as fin:
        data = json.loads(fin.read().decode('utf-8'))
    for entry in data:
        if  entry["country"] == country and entry["name"] == city:
            logger.info(" ".join([str(city),str(country),str(entry["id"])]))
            return entry["id"]
    logger.critical("Critical Error: City not found.")
    sys.exit()

def read_weather_forecast_data():
    try:
        with open("weather.data","r") as f:
            data = json.load(f)
            forecast = {}
            for fct in data["list"]:
                day = datetime.date.fromtimestamp(fct["dt"]).day
                if not day in forecast.keys():
                    forecast[day] = {}
                    forecast[day]['entries'] = 1
                    forecast[day]['temp'] = fct["main"]["temp"]
                    forecast[day]['humidity'] = fct["main"]["humidity"]
                forecast[day]['entries'] += 1
                forecast[day]['temp'] += fct["main"]["temp"]
                forecast[day]['humidity'] += fct["main"]["humidity"]
            for day in forecast:
                forecast[day]['humidity'] /= forecast[day]['entries']
                forecast[day]['temp'] /= forecast[day]['entries']
            global wforecast 
            wforecast = forecast
            return 1
    except:
        logger.error("Error: Failed to read weather forecast data file.")
        return 0


def weather_based_irrigation_volume():
    now = time.time()
    if now - last_weather_update > 24*60*60:
        update_weather_forecast_data(city_id,api_key)
        read_weather_forecast_data()
    today = datetime.date.fromtimestamp(now).day
    if today in wforecast.keys():
        liters = weather_based_volume_function(conf["default_water_dispensing"],wforecast[today]["humidity"],wforecast[today]["temp"])
        return liters
    else:
        return conf["default_water_dispensing"]


def irrigate(use_weather):
    if use_weather:
        volume = round(weather_based_irrigation_volume(),2)
    else:
        volume = conf["default_water_dispensing"]
    irr_time = liter_to_time(volume,conf["water_flux"])
    logger.info("Starting irrigation using "+str(volume)+" liters. It will take "+str(irr_time)+" seconds.")
    now = time.time()
    t = now
    print("Irrigating...",end="")
    sys.stdout.flush()
    while (t <= now+irr_time):
        print(".",end="")
        sys.stdout.flush()
        time.sleep(1)
        t = time.time()
    logger.info("\nIrrigation finished! Happy plants!")

def weather_update():
    city_id = find_city_code(city,country)
    update_weather_forecast_data(city_id,api_key)

def read_conf(file_name = "gardener.conf"):
    confs = {}
    try:
        with open(file_name,"r") as f:
            for line in f.readlines():
                if len(line) > 1 or line[0]!="#":
                    info = [i.strip() for i in line.split("=")]
                    if info[0] in ["use_weather_data"]:
                        confs[info[0]] = info[1]=="True"
                    if info[0] in ["api_key","city_codes_url","city","country","weather_update_time"]:
                        confs[info[0]] = info[1]
                    elif info[0] in ["water_flux","default_water_dispensing"]:
                        confs[info[0]] = float(info[1])
                    elif info[0] in ["watering_times"]:
                        confs[info[0]] = [i.strip() for i in info[1].split(",")]
    except:
        logger.critical("Critical Error: failed to read configuration file. Check if the file \""+file_name+"\" exists.")
        sys.exit()
    return confs





#------------------------------------------------------------------------------

last_weather_update = 0.

wforecast = {}


#Logger
logger = logging.getLogger('gardener_log')
logging.basicConfig(filename='gardener.log',level=logging.DEBUG,format='[%(asctime)s] %(levelname)s: %(message)s', datefmt='%m/%d/%Y %H:%M:%S')
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
logger.addHandler(ch)

logger.info("Starting Gardener")

logger.info("Reading configuration file...")

conf = read_conf()

if conf["use_weather_data"]:
    logger.info("Checking city code file...")
    if not os.path.exists('city.list.json.gz') or not os.path.isfile('city.list.json.gz'):
        get_city_codes(conf["city_codes_url"])

    city_id = find_city_code(conf["city"],conf["country"])

    update_weather_forecast_data(city_id,conf["api_key"])

    read_weather_forecast_data()

    logger.info("Scheduling forecast updates...")
    logger.info("Update weather forecast data every day at "+conf["weather_update_time"]+".")
    schedule.every().day.at(conf["weather_update_time"]).do(weather_update)
    
    
logger.info("Scheduling irrigation...")
for w in conf["watering_times"]:
    logger.info("Irrigation scheduled for every day at "+w+".")
    schedule.every().day.at(w).do(irrigate,conf["use_weather_data"])
    
logger.info("All set. Gardener ready!")
while True:
    schedule.run_pending()
    time.sleep(10)
    
    

