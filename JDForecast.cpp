/**dengjie
*/
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "JDForecast.h"
#include <ArduinoJson.h>
// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()


JDForecast::JDForecast() {

}


void JDForecast::updateForecastsById(JDForecastData *data,JDForecastData *nowData, String appId, String locationId, uint8_t maxForecasts) {
  this->maxForecasts = maxForecasts;
  doUpdate(data,nowData, buildUrl(appId, locationId));
}

/*
  cityPy城市拼音 或中文
*/
String JDForecast::buildUrl(String appId, String cityPy) {
  //  return "http://apip.weatherdt.com/plugin/data?location=" + cityPy + "&key=" + appId;
  return "http://way.jd.com/he/freeweather?city=" + cityPy + "&appkey=" + appId;
}

void JDForecast::doUpdate(JDForecastData *data,JDForecastData *nowData, String url) {
  this->fData = data;
  this->nowData = nowData;
  if ((WiFi.status() == WL_CONNECTED)) {


    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin(client, url); //HTTP
    http.addHeader("Content-Type", "application/json");

    Serial.println("url:" + url);
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    int httpCode = http.POST("");

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        hourlyForecastParse(payload);

        //        Serial.println("received jdJson:\n<<");
        //        Serial.println(jdJson);
        //        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
  this->fData = nullptr;
  this->nowData = nullptr;
//  if (jdJson.length() > 0) {
//    hourlyForecastParse(jdJson);
//  }
}



void JDForecast::hourlyForecastParse(String nJson) {
  String condCode;
  String hou;
  String dateVal;
//  String fJson=nJson;
  String hourlyForecastJson = nJson.substring(nJson.indexOf("\"hourly_forecast"));
  hourlyForecastJson = "{" + hourlyForecastJson.substring(0, hourlyForecastJson.indexOf("]") + 1) + "}";
  
  Serial.println("jdJson:" + hourlyForecastJson);
//  int hfjLen = fJson.length() + 1;
//  char jsonAy[hfjLen];
//  fJson.toCharArray(jsonAy, hfjLen);

  int hfjLen = hourlyForecastJson.length() + 1;
  char jsonAy[hfjLen];
  hourlyForecastJson.toCharArray(jsonAy, hfjLen);
  
//  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(7) + JSON_ARRAY_SIZE(8) + 2*JSON_OBJECT_SIZE(1) + 25*JSON_OBJECT_SIZE(2) + 31*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 9*JSON_OBJECT_SIZE(7) + 3*JSON_OBJECT_SIZE(8) + 7*JSON_OBJECT_SIZE(11) + 3390;
  const size_t capacity = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(1) + 8*JSON_OBJECT_SIZE(2) + 8*JSON_OBJECT_SIZE(4) + 8*JSON_OBJECT_SIZE(7) + 970;
  DynamicJsonDocument root(capacity);

  deserializeJson(root, jsonAy);
//  JsonObject result_HeWeather5_0 = root["result"]["HeWeather5"][0];
//  JsonArray hourly = result_HeWeather5_0["hourly_forecast"];
  JsonArray hourly = root["hourly_forecast"];
  //now arduinoJson 无法解析大json，放弃这段
//  JsonObject result_HeWeather5_0_now = result_HeWeather5_0["now"];
//  condCode = result_HeWeather5_0_now["cond"]["code"].as<int>();
//  this->nowData = nullptr;
//  this->nowData->condCode = condCode;
//  this->nowData->condTxt = result_HeWeather5_0_now["cond"]["txt"].as<String>();
//  this->nowData->iconMeteoCon = getMeteoconIcon(condCode, 0);
//  this->nowData->hum = result_HeWeather5_0_now["hum"].as<int>();
//  this->nowData->tmp = result_HeWeather5_0_now["tmp"].as<float>();
//
//  JsonObject result_HeWeather5_0_aqi_city = result_HeWeather5_0["aqi"]["city"];
//  this->nowData->aqi = result_HeWeather5_0_aqi_city["aqi"].as<String>();
//  this->nowData->qlty = result_HeWeather5_0_aqi_city["qlty"].as<String>();
//  this->nowData->pm25 = result_HeWeather5_0_aqi_city["pm25"].as<String>();
  //forecast
  uint8_t idx = 0;
  for(JsonVariant v : hourly) {
    if (idx >= maxForecasts) {
      //超出最大预报天数，退出
      Serial.println("超出最大预报天数，退出，idx:"+String(idx));
      break;
    }
    JsonObject hourly_i = v.as<JsonObject>();
    //日期时间
    dateVal = hourly_i["date"].as<String>();
    hou = dateVal.substring(dateVal.indexOf(":") - 2, dateVal.indexOf(":"));
    
    if (hou.startsWith("0")) {
      hou = hou.substring(1);
    }
    
//    if(hou.toInt()<(timeInfo->tm_hour)){
//      continue;//小于当前时间继续下次循环
//    }
    fData[idx].date = dateVal;
    fData[idx].intHou = hou.toInt();
    if (fData[idx].intHou < 5 || fData[idx].intHou > 19) {
      //黑夜
      fData[idx].dayNight = 1;
    } else {
      //白天
      fData[idx].dayNight = 0;
    }
    //    Serial.println("intHou:" + fData[idx].intHou);
    //相对湿度
    fData[idx].hum = hourly_i["hum"].as<int>();
    //温度
    fData[idx].tmp = hourly_i["tmp"].as<float>();
    //天气代码
    condCode = hourly_i["cond"]["code"].as<String>();
    fData[idx].condCode = condCode.toInt();
    fData[idx].iconMeteoCon = getMeteoconIcon(fData[idx].condCode, fData[idx].dayNight);
    fData[idx].condTxt = hourly_i["cond"]["txt"].as<String>();
    idx++;
  }
  root.clear();

  //now
  /*
   * {"now":{
                    "cond":{
                        "code":"399",
                        "txt":"雨"
                    },
                    "fl":"8",
                    "hum":"100",
                    "pcpn":"0.4",
                    "pres":"1003",
                    "tmp":"9",
                    "vis":"16",
                    "wind":{
                        "deg":"261",
                        "dir":"西风",
                        "sc":"2",
                        "spd":"6"
                    }
                }}
   */
  String nowJson = nJson.substring(nJson.indexOf("\"now"));
  nowJson = "{" + nowJson.substring(0, nowJson.indexOf("\"status") - 1);
  int ent = 0;
  
  int bg = nowJson.indexOf("code");
  ent = nowJson.indexOf("\"",bg+7);
  int cdCode = nowJson.substring(bg + 7, ent).toInt();
  this->nowData->condCode = cdCode;
  this->nowData->iconMeteoCon = getMeteoconIcon(cdCode, 0);//
  
  bg = nowJson.indexOf("txt");
  ent = nowJson.indexOf("\"",bg+6);
  this->nowData->condTxt = nowJson.substring(bg + 6, ent);

  bg = nowJson.indexOf("hum");
  ent = nowJson.indexOf("\"",bg+6);
  this->nowData->hum = nowJson.substring(bg + 6, ent).toInt();

  bg = nowJson.indexOf("tmp");
  ent = nowJson.indexOf("\"",bg+6);
  this->nowData->tmp = nowJson.substring(bg + 6, ent).toFloat();
  Serial.println("nowJson:"+nowJson);
  //aqi
  /*
   * 
 {
  "city":{
   "aqi":"29",
    "qlty":"优",
    "pm25":"20",
    "pm10":"11",
    "no2":"22",
    "so2":"4",
    "co":"0.9",
    "o3":"24"
  }
}
   */
  String aqiJson =  nJson.substring(nJson.indexOf("\"city"));
  aqiJson = "{" + aqiJson.substring(0, aqiJson.indexOf("},") + 1);
  ent = 0;
  
  bg = aqiJson.indexOf("aqi");
  ent = aqiJson.indexOf("\"",bg+6);
  this->nowData->aqi = aqiJson.substring(bg + 6, ent);

  bg = aqiJson.indexOf("qlty");
  ent = aqiJson.indexOf("\"",bg+7);
  this->nowData->qlty = aqiJson.substring(bg + 7, ent);

  bg = aqiJson.indexOf("pm25");
  ent = aqiJson.indexOf("\"",bg+7);
  this->nowData->pm25 = aqiJson.substring(bg + 7, ent);

  Serial.println("aqiJson:"+aqiJson);
  
}


String JDForecast::getMeteoconIcon(int icon, int dn) {
  //天气代码查询 https://dev.heweather.com/docs/refer/condition
  String rtCode = ")";
  //clear sky67 66
  if (icon == 100 || icon==150) {
    rtCode = (dn == 1 ? "C" : "B");
  }
  if (icon == 104) {
    rtCode = "3";
  }
  
  //多云
  if (icon == 154) {
    rtCode = "E";
  }
  // few clouds
  if (icon == 103) {
    rtCode = "4";
  }
  if (icon == 153) {
    rtCode = "I";
  }
  // scattered clouds
  if (icon == 102 ) {
    rtCode = (dn == 1 ? "5" : "N");
  }
  // broken clouds
  if (icon == 101) {
    rtCode = (dn == 1 ? "%" : "Y");
  }
  // rain
  if (icon == 305 || icon == 300 || icon == 301) {
    rtCode = (dn == 1 ? "7" : "Q");
  }
  // shower rain
  if (icon >= 306 && icon <= 399) {
    rtCode = (dn == 1 ? "8" : "R");
  }

  if (icon == 302 || icon == 304) {
    rtCode = (dn == 1 ? "6" : "O");
  }
  
  // thunderstorm
  if ( icon == 303) {
    rtCode = (dn == 1 ? "&" : "P");
  }
  // snow
  if (icon >= 400 && icon <= 499) {
    rtCode = (dn == 1 ? "#" : "W");
  }
  // mist
  if (icon >= 500 && icon <= 515) {
    rtCode = "M";
  }
  return rtCode;
}
