/**dengjie
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Forecast.h"
#include <ArduinoJson.h>
// time
#include <time.h>      // time() ctime()
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()

Forecast::Forecast()
{
}

void Forecast::updateForecastsById(ForecastData *data, ForecastData *nowData, uint8_t maxForecasts)
{
  this->maxForecasts = maxForecasts;
  doUpdate(data, nowData, gaodeUrl());
}

/*
 * 生成天气url
*/
String Forecast::gaodeUrl()
{
  //  return "http://apip.weatherdt.com/plugin/data?location=" + cityPy + "&key=" + appId;
  String localCity = getCity(CITY_URL);
  //  return "http://way.jd.com/he/freeweather?city=" + localCity + "&appkey=" + appId;
  return WEATHER_URL + localCity;
}

/*
 * 调用高德IP定位获取当前城市
 */
String Forecast::getCity(String url)
{
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  http.begin(client, url); // HTTP
  http.addHeader("Content-Type", "application/json");
  String rtCity = "";
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      const size_t capacity = JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 200;
      DynamicJsonDocument doc(capacity);

      int ctLen = payload.length() + 1;
      char jsonAy[ctLen];
      payload.toCharArray(jsonAy, ctLen);
      deserializeJson(doc, jsonAy);

      

      rtCity = doc["adcode"].as<String>();

      //        Serial.println("received jdJson:\n<<");
      //        Serial.println(jdJson);
      //        Serial.println(">>");
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  Serial.println("city：" + rtCity);
  return rtCity;
}

void Forecast::doUpdate(ForecastData *data, ForecastData *nowData, String url)
{
  this->fData = data;
  this->nowData = nowData;
  // #now
  if ((WiFi.status() == WL_CONNECTED))
  {

    Serial.print("[HTTP] begin...\n");

    httpGetNow(url);
    delay(1000);
    httpGetForecasts(url);
  }

  this->fData = nullptr;
  this->nowData = nullptr;
  //  if (jdJson.length() > 0) {
  //    hourlyForecastParse(jdJson);
  //  }
}

void Forecast::httpGetNow(String url)
{
  // now
//  String dataTypeStr = "&data_type=all";//"&data_type=now";
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  http.begin(client, url); // HTTP
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  Serial.println("url:" + url);
  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header and body
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      liveNowParse(payload);

      //        Serial.println("received jdJson:\n<<");
      //        Serial.println(jdJson);
      //        Serial.println(">>");
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  //请求空气质量数据
  //httpGetcsAQI();

}

void Forecast::httpGetForecasts(String url)
{
  String dataTypeStr = "&extensions=all";
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  http.begin(client, url + dataTypeStr); // HTTP
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  Serial.println("url:" + url + dataTypeStr);
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header and body
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      threeDayForecastsParse(payload);

      //        Serial.println("received jdJson:\n<<");
      //        Serial.println(jdJson);
      //        Serial.println(">>");
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void Forecast::httpGetcsAQI()
{
  String url = "http://218.76.24.159:8031/Data/Default.ashx?method=GetCountyAqi&UniqueCode=430100001";
  HTTPClient http;
  WiFiClient client;
  http.begin(client, url); // HTTP
  http.setTimeout(10000);
  http.addHeader("Content-Type", "application/json");
  Serial.println("url:" + url);
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header and body
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      aqiParse(payload);

      //        Serial.println("received jdJson:\n<<");
      //        Serial.println(jdJson);
      //        Serial.println(">>");
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void Forecast::liveNowParse(String nJson)
{
  Serial.println("json:"+nJson);                                                                                                                                                  
  // https://arduinojson.org/v6/assistant/#/step1
  // #解析json now begin#
  //  String input;
  StaticJsonDocument<1024> doc;

  DeserializationError error = deserializeJson(doc, nJson);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
//    Serial.println(error.f_str());
    return;
  }

  JsonObject lives_0 = doc["lives"][0];
  const String lives_0_province = lives_0["province"]; // "北京"
  const char* lives_0_city = lives_0["city"]; // "东城区"
  const char* lives_0_adcode = lives_0["adcode"]; // "110101"
  const String lives_0_weather = lives_0["weather"]; // "晴"
  const String lives_0_temperature = lives_0["temperature"]; // "28"
  const char* lives_0_winddirection = lives_0["winddirection"]; // "西南"
  const char* lives_0_windpower = lives_0["windpower"]; // "≤3"
  const String lives_0_humidity = lives_0["humidity"]; // "76"
  const String lives_0_reporttime = lives_0["reporttime"]; // "2025-07-01 19:33:39"
  const char* lives_0_temperature_float = lives_0["temperature_float"]; // "28.0"
  const char* lives_0_humidity_float = lives_0["humidity_float"]; // "76.0"
  
  // 开始赋值
  // this->nowData->condCode = 154;
  String hou = lives_0_reporttime.substring(11, 13);

  if (hou.toInt() < 5 || hou.toInt() > 19)
  {
    // 黑夜
    this->nowData->dayNight = 1;
  }
  else
  {
    // 白天
    this->nowData->dayNight = 0;
  }
  //  Serial.print("hou:");
  //  Serial.println(hou);
  this->nowData->iconMeteoCon = getIcon(lives_0_weather, this->nowData->dayNight);

  this->nowData->condTxt = lives_0_weather;
  this->nowData->hum = lives_0_humidity;
  this->nowData->tmp = lives_0_temperature;
  this->nowData->windClass = lives_0_windpower;
  // this->nowData->aqi = result_now_aqi;

  // this->nowData->qlty = '';
  // this->nowData->pm25 = result_now_pm25;
  Serial.println(this->nowData->condTxt+","+this->nowData->hum+","+this->nowData->tmp+","+this->nowData->windClass+",");


 

//  int idx = 0;
//  for (JsonObject result_forecast : doc["result"]["forecasts"].as<JsonArray>())
//  {
//
//    const char *result_forecast_text_day = result_forecast["text_day"];     // "小雨", "小雨", "小雨", "小雨", "多云"
//    const char *result_forecast_text_night = result_forecast["text_night"]; // "小雨", "多云", "小雨", "多云", "多云"
//    int result_forecast_high = result_forecast["high"];                     // 33, 33, 34, 33, 36
//    int result_forecast_low = result_forecast["low"];                       // 26, 26, 27, 28, 28
//    const char *result_forecast_wc_day = result_forecast["wc_day"];         // "<3级", "<3级", "<3级", "<3级", "<3级"
//    const char *result_forecast_wd_day = result_forecast["wd_day"];         // "东南风", "西南风", "东南风", "西南风", "南风"
//    const char *result_forecast_wc_night = result_forecast["wc_night"];     // "<3级", "<3级", "<3级", "<3级", "<3级"
//    const char *result_forecast_wd_night = result_forecast["wd_night"];     // "东南风", "东南风", "东南风", "西南风", "南风"
//    const char *result_forecast_date = result_forecast["date"];             // "2023-07-19", "2023-07-20", "2023-07-21", ...
//    const char *result_forecast_week = result_forecast["week"];             // "星期三", "星期四", "星期五", "星期六", "星期日"
//    if (idx > 0)
//    {
//      fData[idx - 1].date = result_forecast_date;
//
//      fData[idx - 1].condTxt = result_forecast_text_day;
//      fData[idx - 1].iconMeteoCon = getIcon(result_forecast_text_day, 0);
//      fData[idx - 1].nightcondTxt = result_forecast_text_night;
//      fData[idx - 1].nightIconeteoCon = getIcon(result_forecast_text_night, 1);
//      fData[idx - 1].hightTmp = result_forecast_high;
//      fData[idx - 1].lowTmp = result_forecast_low;
//    }
//
//    idx++;
//  }
  
}

void Forecast::threeDayForecastsParse(String nJson)
{

  // https://arduinojson.org/v6/assistant/#/step1
  // #解析json now begin#
  //  String input;

  DynamicJsonDocument doc(1600);

  DeserializationError error = deserializeJson(doc, nJson);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
//    Serial.println(error.f_str());
    return;
  }


JsonObject forecasts_0 = doc["forecasts"][0];
//const char* forecasts_0_city = forecasts_0["city"]; // "长沙市"
//const char* forecasts_0_adcode = forecasts_0["adcode"]; // "430100"
//const char* forecasts_0_province = forecasts_0["province"]; // "湖南"
//const char* forecasts_0_reporttime = forecasts_0["reporttime"]; // "2025-07-01 20:34:10"
int idx = 0;
for (JsonObject forecasts_0_cast : forecasts_0["casts"].as<JsonArray>()) {

  const String forecasts_0_cast_date = forecasts_0_cast["date"]; // "2025-07-01", "2025-07-02", ...
//  const char* forecasts_0_cast_week = forecasts_0_cast["week"]; // "2", "3", "4", "5"
  const String forecasts_0_cast_dayweather = forecasts_0_cast["dayweather"]; // "晴", "晴", "晴", "晴"
  const String forecasts_0_cast_nightweather = forecasts_0_cast["nightweather"]; // "晴", "晴", "晴", "多云"
  const String forecasts_0_cast_daytemp = forecasts_0_cast["daytemp"]; // "35", "36", "37", "38"
  const String forecasts_0_cast_nighttemp = forecasts_0_cast["nighttemp"]; // "27", "27", "28", "28"
//  const char* forecasts_0_cast_daywind = forecasts_0_cast["daywind"]; // "南", "南", "南", "南"
//  const char* forecasts_0_cast_nightwind = forecasts_0_cast["nightwind"]; // "南", "南", "南", "南"
//  const char* forecasts_0_cast_daypower = forecasts_0_cast["daypower"]; // "1-3", "1-3", "1-3", "1-3"
//  const char* forecasts_0_cast_nightpower = forecasts_0_cast["nightpower"]; // "1-3", "1-3", "1-3", "1-3"
//  const char* forecasts_0_cast_daytemp_float = forecasts_0_cast["daytemp_float"]; // "35.0", "36.0", ...
//  const char* forecasts_0_cast_nighttemp_float = forecasts_0_cast["nighttemp_float"]; // "27.0", "27.0", ...
  if (idx > 0){
      fData[idx - 1].date = forecasts_0_cast_date;

      fData[idx - 1].condTxt = forecasts_0_cast_dayweather;
      fData[idx - 1].iconMeteoCon = getIcon(forecasts_0_cast_dayweather, 0);
      fData[idx - 1].nightcondTxt = forecasts_0_cast_nightweather;
      fData[idx - 1].nightIconeteoCon = getIcon(forecasts_0_cast_nightweather, 1);
      fData[idx - 1].hightTmp = forecasts_0_cast_daytemp;
      fData[idx - 1].lowTmp = forecasts_0_cast_nighttemp;
      Serial.println(fData[idx - 1].date+":"+fData[idx - 1].condTxt+","+fData[idx - 1].hightTmp+",");
    }

    idx++;
}


}

void Forecast::aqiParse(String nJson)
{
  Serial.println("aqi_json:"+nJson);
  // https://arduinojson.org/v6/assistant/#/step1
  // #解析json now begin#
  // String input;

  DynamicJsonDocument doc(4096);

  DeserializationError error = deserializeJson(doc, nJson);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
//    Serial.println(error.f_str());
    return;
  }

  for (JsonObject item : doc.as<JsonArray>()) {

    const char* StationName = item["StationName"]; // "浏阳市", "雨花区环保局", "马坡岭", "经开区环保局", "天心区环保局", "长沙县", ...
    const String AQI = item["AQI"]; // "16", "25", "32", "32", "32", "35", "38", "39", "40", "42", "45", ...
    const char* AQType = item["AQType"]; // "优", "优", "优", "优", "优", "优", "优", "优", "优", "优", "优", "优", "优", ...
    const char* PrimaryEP = item["PrimaryEP"]; // "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", ...
    const char* area = item["area"]; // "长沙市", "长沙市", "长沙市", "长沙市", "长沙市", "长沙市", "长沙市", "长沙市", "长沙市", ...
    const String UniqueCode = item["UniqueCode"]; // "430104999", "430100053", "430100070", "430100072", ...
    const char* sort = item["sort"]; // "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", ...
    const char* QueryTime = item["QueryTime"]; // "2023年11月10日10时", "2023年11月10日10时", "2023年11月10日10时", ...
    if(UniqueCode=="430100072"){
      this->nowData->aqi = AQI;
      return;
    }

  }

}

// void JDForecast::hourlyForecastParse(String nJson) {
//   String condCode;
//   String hou;
//   String dateVal;
////  String fJson=nJson;
//  String hourlyForecastJson = nJson.substring(nJson.indexOf("\"hourly_forecast"));
//  hourlyForecastJson = "{" + hourlyForecastJson.substring(0, hourlyForecastJson.indexOf("]") + 1) + "}";
//
//  Serial.println("jdJson:" + hourlyForecastJson);
////  int hfjLen = fJson.length() + 1;
////  char jsonAy[hfjLen];
////  fJson.toCharArray(jsonAy, hfjLen);
//
//  int hfjLen = hourlyForecastJson.length() + 1;
//  char jsonAy[hfjLen];
//  hourlyForecastJson.toCharArray(jsonAy, hfjLen);
//
////  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(7) + JSON_ARRAY_SIZE(8) + 2*JSON_OBJECT_SIZE(1) + 25*JSON_OBJECT_SIZE(2) + 31*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 9*JSON_OBJECT_SIZE(7) + 3*JSON_OBJECT_SIZE(8) + 7*JSON_OBJECT_SIZE(11) + 3390;
//  const size_t capacity = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(1) + 8*JSON_OBJECT_SIZE(2) + 8*JSON_OBJECT_SIZE(4) + 8*JSON_OBJECT_SIZE(7) + 970;
//  DynamicJsonDocument root(capacity);
//
//  deserializeJson(root, jsonAy);
////  JsonObject result_HeWeather5_0 = root["result"]["HeWeather5"][0];
////  JsonArray hourly = result_HeWeather5_0["hourly_forecast"];
//  JsonArray hourly = root["hourly_forecast"];
//  //now arduinoJson 无法解析大json，放弃这段
////  JsonObject result_HeWeather5_0_now = result_HeWeather5_0["now"];
////  condCode = result_HeWeather5_0_now["cond"]["code"].as<int>();
////  this->nowData = nullptr;
////  this->nowData->condCode = condCode;
////  this->nowData->condTxt = result_HeWeather5_0_now["cond"]["txt"].as<String>();
////  this->nowData->iconMeteoCon = getMeteoconIcon(condCode, 0);
////  this->nowData->hum = result_HeWeather5_0_now["hum"].as<int>();
////  this->nowData->tmp = result_HeWeather5_0_now["tmp"].as<float>();
////
////  JsonObject result_HeWeather5_0_aqi_city = result_HeWeather5_0["aqi"]["city"];
////  this->nowData->aqi = result_HeWeather5_0_aqi_city["aqi"].as<String>();
////  this->nowData->qlty = result_HeWeather5_0_aqi_city["qlty"].as<String>();
////  this->nowData->pm25 = result_HeWeather5_0_aqi_city["pm25"].as<String>();
//  //forecast
//  uint8_t idx = 0;
//  for(JsonVariant v : hourly) {
//    if (idx >= maxForecasts) {
//      //超出最大预报天数，退出
//      Serial.println("超出最大预报天数，退出，idx:"+String(idx));
//      break;
//    }
//    JsonObject hourly_i = v.as<JsonObject>();
//    //日期时间
//    dateVal = hourly_i["date"].as<String>();
//    hou = dateVal.substring(dateVal.indexOf(":") - 2, dateVal.indexOf(":"));
//
//    if (hou.startsWith("0")) {
//      hou = hou.substring(1);
//    }
//
////    if(hou.toInt()<(timeInfo->tm_hour)){
////      continue;//小于当前时间继续下次循环
////    }
//    fData[idx].date = dateVal;
//    fData[idx].intHou = hou.toInt();
//    if (fData[idx].intHou < 5 || fData[idx].intHou > 19) {
//      //黑夜
//      fData[idx].dayNight = 1;
//    } else {
//      //白天
//      fData[idx].dayNight = 0;
//    }
//    //    Serial.println("intHou:" + fData[idx].intHou);
//    //相对湿度
//    fData[idx].hum = hourly_i["hum"].as<int>();
//    //温度
//    fData[idx].tmp = hourly_i["tmp"].as<float>();
//    //天气代码
//    condCode = hourly_i["cond"]["code"].as<String>();
//    fData[idx].condCode = condCode.toInt();
//    fData[idx].iconMeteoCon = getMeteoconIcon(fData[idx].condCode, fData[idx].dayNight);
//    fData[idx].condTxt = hourly_i["cond"]["txt"].as<String>();
//    idx++;
//  }
//  root.clear();
//
//  //now
//  /*
//   * {"now":{
//                    "cond":{
//                        "code":"399",
//                        "txt":"雨"
//                    },
//                    "fl":"8",
//                    "hum":"100",
//                    "pcpn":"0.4",
//                    "pres":"1003",
//                    "tmp":"9",
//                    "vis":"16",
//                    "wind":{
//                        "deg":"261",
//                        "dir":"西风",
//                        "sc":"2",
//                        "spd":"6"
//                    }
//                }}
//   */
//  String nowJson = nJson.substring(nJson.indexOf("\"now"));
//  nowJson = "{" + nowJson.substring(0, nowJson.indexOf("\"status") - 1);
//  int ent = 0;
//
//  int bg = nowJson.indexOf("code");
//  ent = nowJson.indexOf("\"",bg+7);
//  int cdCode = nowJson.substring(bg + 7, ent).toInt();
//  this->nowData->condCode = cdCode;
//  this->nowData->iconMeteoCon = getMeteoconIcon(cdCode, 0);//
//
//  bg = nowJson.indexOf("txt");
//  ent = nowJson.indexOf("\"",bg+6);
//  this->nowData->condTxt = nowJson.substring(bg + 6, ent);
//
//  bg = nowJson.indexOf("hum");
//  ent = nowJson.indexOf("\"",bg+6);
//  this->nowData->hum = nowJson.substring(bg + 6, ent).toInt();
//
//  bg = nowJson.indexOf("tmp");
//  ent = nowJson.indexOf("\"",bg+6);
//  this->nowData->tmp = nowJson.substring(bg + 6, ent).toFloat();
//  Serial.println("nowJson:"+nowJson);
//  //aqi
//  /*
//   *
// {
//  "city":{
//   "aqi":"29",
//    "qlty":"优",
//    "pm25":"20",
//    "pm10":"11",
//    "no2":"22",
//    "so2":"4",
//    "co":"0.9",
//    "o3":"24"
//  }
//}
//   */
//  String aqiJson =  nJson.substring(nJson.indexOf("\"city"));
//  aqiJson = "{" + aqiJson.substring(0, aqiJson.indexOf("},") + 1);
//  ent = 0;
//
//  bg = aqiJson.indexOf("aqi");
//  ent = aqiJson.indexOf("\"",bg+6);
//  this->nowData->aqi = aqiJson.substring(bg + 6, ent);
//
//  bg = aqiJson.indexOf("qlty");
//  ent = aqiJson.indexOf("\"",bg+7);
//  this->nowData->qlty = aqiJson.substring(bg + 7, ent);
//
//  bg = aqiJson.indexOf("pm25");
//  ent = aqiJson.indexOf("\"",bg+7);
//  this->nowData->pm25 = aqiJson.substring(bg + 7, ent);
//
//  Serial.println("aqiJson:"+aqiJson);
//
//}

// String JDForecast::getMeteoconIcon(int icon, int dn) {
//   //天气代码查询 https://dev.heweather.com/docs/refer/condition
//   String rtCode = ")";
//   //clear sky67 66
//   if (icon == 100 || icon==150) {
//     rtCode = (dn == 1 ? "C" : "B");
//   }
//   if (icon == 104) {
//     rtCode = "3";
//   }

//   //多云
//   if (icon == 154) {
//     rtCode = "E";
//   }
//   // few clouds
//   if (icon == 103) {
//     rtCode = "4";
//   }
//   if (icon == 153) {
//     rtCode = "I";
//   }
//   // scattered clouds
//   if (icon == 102 ) {
//     rtCode = (dn == 1 ? "5" : "N");
//   }
//   // broken clouds
//   if (icon == 101) {
//     rtCode = (dn == 1 ? "%" : "Y");
//   }
//   // rain
//   if (icon == 305 || icon == 300 || icon == 301) {
//     rtCode = (dn == 1 ? "7" : "Q");
//   }
//   // shower rain
//   if (icon >= 306 && icon <= 399) {
//     rtCode = (dn == 1 ? "8" : "R");
//   }

//   if (icon == 302 || icon == 304) {
//     rtCode = (dn == 1 ? "6" : "O");
//   }

//   // thunderstorm
//   if ( icon == 303) {
//     rtCode = (dn == 1 ? "&" : "P");
//   }
//   // snow
//   if (icon >= 400 && icon <= 499) {
//     rtCode = (dn == 1 ? "#" : "W");
//   }
//   // mist
//   if (icon >= 500 && icon <= 515) {
//     rtCode = "M";
//   }
//   return rtCode;
// }

/*
 text 天气文本，dn 1晚上、0白天
*/
String Forecast::getIcon(String text, int dn)
{
  // 天气代码查询
  String rtCode = ")";

  // clear sky67 66
  if (text == "晴")
  {
    rtCode = (dn == 1 ? "2" : "1");
  }
  // //多云 4晚上，E白天
  // if (text == "多云") {
  //   rtCode = (dn == 1 ? "4" : "E");
  // }
  // 阴天
  if (text == "多云")
  {
    rtCode = (dn == 1 ? "4" : "3");
  }
  if (text == "阴")
  {
    rtCode = (dn == 1 ? "Y" : "%");
  }
  // 小雨 7实心1点雨 Q空心1点雨
  if (text.endsWith("雨"))
  {
    rtCode = (dn == 1 ? "Q" : "7");
  }
  // 阵雨 8实心3点雨  R空心3点雨
  if (text.endsWith("阵雨"))
  {
    rtCode = (dn == 1 ? "T" : "!");
  }

  // 雷阵雨 6实心云1闪电 O空心云1闪电
  if (text.endsWith("雷阵雨"))
  {
    rtCode = (dn == 1 ? "Z" : "&");
  }
  // 雨夹雪 #实心云2雨1雪 W空心云2雨1雪
  if (text.endsWith("雪"))
  {
    rtCode = (dn == 1 ? "W" : "#");
  }
  // 阵雪 #实心云2雨1雪 W空心云2雨1雪
  if (text.endsWith("阵雪"))
  {
    rtCode = (dn == 1 ? "U" : "\"");
  }
  // 雾
  if (text.endsWith("雾"))
  {
    rtCode = "M";
  }
  if (text.endsWith("霾"))
  {
    rtCode = (dn == 1 ? "K" : "J");
  }

  // rtCode = "#";
  //  // few clouds 3少云
  //  if (text == "多云") {
  //    rtCode = "4";
  //  }
  //  if (text == 153) {
  //    rtCode = "I";//空心多云
  //  }
  //  // scattered clouds 5阴天实心 N空心
  //  if (text == 102 ) {
  //    rtCode = (dn == 1 ? "5" : "N");
  //  }
  //  // broken clouds
  //  if (text == 101) {
  //    rtCode = (dn == 1 ? "%" : "Y");//%实心云（多） Y多云空心
  //  }
  //  // rain
  //  if (text == 305 || text == 300 || text == 301) {
  //    rtCode = (dn == 1 ? "7" : "Q");
  //  }
  //  // shower rain 阵雨 8实心3点雨  R空心3点雨
  //  if (text >= 306 && text <= 399) {
  //    rtCode = (dn == 1 ? "8" : "R");
  //  }

  // if (text == 302 || text == 304) {
  //   rtCode = (dn == 1 ? "6" : "O");
  // }

  // // thunderstorm
  // if ( text == 303) {
  //   rtCode = (dn == 1 ? "&" : "P");
  // }
  // // snow
  // if (text >= 400 && text <= 499) {
  //   rtCode = (dn == 1 ? "#" : "W");
  // }
  // // mist
  // if (text >= 500 && text <= 515) {
  //   rtCode = "M";
  // }
  return rtCode;
}
