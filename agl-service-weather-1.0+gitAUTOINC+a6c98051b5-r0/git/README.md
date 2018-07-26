# Weather Service

## Overview

Weather service uses current weather conditions from the OpenWeathermap webservice.

## Verbs
| Name            | Description                     | JSON Parameters                                 |
|:----------------|:--------------------------------|:------------------------------------------------|
| subscribe       | subscribe to media events       | *Request:* {"value":"weather"}                  |
| unsubscribe     | unsubscribe to media events     | *Request:* {"value":"weather"}                  |
| api_key         | get/set API key                 | *Request:* {"value": "openweather map api key"} |
| current_weather | get current weather conditions  | See **current_weather Reporting** section       |


### current_weather Reporting

JSON Response is the current weather of the location detected from the geoclue service.

OpenWeatherAPI Response API Documentation: *http://openweathermap.org/current*

Example Response:
<pre>
{
    "coord": {
      "lon": -122.63,
      "lat": 45.64
    },
    "weather": [
      {
        "id": 800,
        "main": "Clear",
        "description": "clear sky",
        "icon": "01n"
      }
    ],
    "base": "stations",
    "main": {
      "temp": 48.65,
      "pressure": 1023,
      "humidity": 76,
      "temp_min": 44.6,
      "temp_max": 51.8
    },
    "visibility": 16093,
    "wind": {
      "speed": 3.36,
      "deg": 320
    },
    "clouds": {
      "all": 1
    },
    "dt": 1517885760,
    "sys": {
      "type": 1,
      "id": 2963,
      "message": 0.0073,
      "country": "US",
      "sunrise": 1517930709,
      "sunset": 1517966681
    },
    "id": 5814616,
    "name": "Vancouver",
    "cod": 200,
    "url": "http:\/\/api.openweathermap.org\/data\/2.5\/weather?lat=45.6447&lon=-122.6298&units=imperial&APPID=a860fa437924aec3d0360cc749e25f0e"
}
</pre>

### weather Event JSON Response

JSON response for this event has the same results as documented in **current_weather** section.
