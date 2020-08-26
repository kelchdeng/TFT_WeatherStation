/**dengjie
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
//OTA
#include <ArduinoOTA.h>
//TFT
#include <Adafruit_GFX.h>    // Core graphics library Adafruit_GFX_Library
#include <Adafruit_ST7735.h> // Hardware-specific library Adafruit_ST7735_and_ST7789_Library
#include <SPI.h>

#include "JDForecast.h"
#include "GFXWeatherFont.h"





/***************************
   Begin Settings
 **************************/
#define HOSTNAME "ESP8266-TQ-"

//#TFT 
// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     D0 //cs
#define TFT_DC     D1 //dc
#define TFT_RST    D2 //res
#define TFT_MOSI   D3 //sda
#define TFT_SCLK   D4 //scl

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
//#TFT

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries中国没有夏令时，赋0

// Setup
const int UPDATE_INTERVAL_SECS = 20 * 60; // Update every 20 minutes 每20分钟更新



// https://wx.jdcloud.com/market/datas/26/10610 
//const String OPEN_WEATHER_MAP_APP_ID = "";
// https://wx.jdcloud.com/market/datas/26/10610 
//const String OPEN_WEATHER_MAP_LOCATION_ID = "";

// https://wx.jdcloud.com/market/datas/26/10610 京东和风 appid
const String JD_APP_ID = "";
// https://wx.jdcloud.com/market/datas/26/10610 京东和风 城市
const String JD_LOCATION_ID = "";

//弃用
String OPEN_WEATHER_MAP_LANGUAGE = "zh_cn";
//最大预报天数 暂时没用
const uint8_t MAX_FORECASTS = 4;

const boolean IS_METRIC = true;

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/***************************
   End Settings
 **************************/

//当前天气对象实例
JDForecastData currentWeather;
//当前天气客户端对象实例
//OpenWeatherMapCurrent currentWeatherClient;
//预报对象实例数组，数组大小是MAX_FORECASTS定义的值
JDForecastData forecasts[MAX_FORECASTS];
//预报客户端对象实例
JDForecast forecastClient;

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes 标记更新
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

//declaring prototypes 声明
void configModeCallback (WiFiManager *myWiFiManager);

//绘制大字时间
void drawBigTime();
//绘制日期时间
void drawDateTime();
//绘制当前天气
void drawNowWeather();
//绘制预报
void drawForecast();
void updateData();

void drawForecastDetails( int dayIndex) ;

void setReadyForWeatherUpdate();

int numberOfOverlays = 1;
int h = 0;


void setup() {
  Serial.begin(9600);
  Serial.println("start..");
  
  // OTA设置并启动
  ArduinoOTA.setHostname("WeatherStation");
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin();
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
  tft.setRotation(tft.getRotation()+1);
//  tft.setCursor(5, 5);

  WiFiConfig();

  
  // Get time from network time service. ntp服务设置
  configTime(TZ_SEC, DST_SEC, "ntp1.aliyun.com");


  updateData();


}

void loop() {
   ArduinoOTA.handle();
  if (millis() - timeSinceLastWUpdate > (1000L * UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate) {
    updateData();
  }

  drawBigTime();
  delay(5000);

  drawDateTime();
  delay(5000);

  drawNowWeather();
  delay(5000);
  
  drawForecast();
  delay(5000);

}

//绘制大字时间
void drawBigTime() {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  //清屏
  tft.fillScreen(ST7735_BLACK);
  //时间
  tft.drawBitmap(2,5,shijian,72,36,ST77XX_YELLOW);
  tft.setCursor(2, 56);
  tft.setFont(NULL);
  tft.setTextSize(5);
  tft.setTextColor(ST7735_GREEN);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  tft.println(String(buff));


  
}

//绘制日期时间
void drawDateTime() {
  
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  //清屏
  tft.fillScreen(ST7735_BLACK);
  
  tft.setCursor(2, 24);
  //字体28
  tft.setFont(&Arimo_Regular_24);
  String wday = WDAY_NAMES[timeInfo->tm_wday];
  //年-月-日
  sprintf_P(buff, PSTR("%04d-%02d-%02d"),timeInfo->tm_year + 1900,timeInfo->tm_mon + 1,timeInfo->tm_mday );
  tft.setTextColor(ST7735_ORANGE);
  tft.setTextSize(1);
  // tft.setTextSize(2);
  tft.println(String(buff));
  
  tft.setTextColor(ST7735_ORANGE);
  //星期
  tft.println(wday);
  //时间
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  tft.setTextColor(ST7735_GREEN);
  
  tft.println(String(buff));
  tft.drawBitmap(2,96,shizhong,72,36,ST77XX_YELLOW);
  
}

//绘制当前天气
void drawNowWeather() {
  
  
  //清屏
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.setCursor(2, 28);
  //天气图标
  tft.println(currentWeather.iconMeteoCon);
  //天气
  tft.drawBitmap(40,5,tianqi,72,36,ST77XX_YELLOW);
 
  //字体16
  tft.setFont(&Arimo_Regular_16);
  tft.setTextColor(ST7735_RED);
  tft.println("HUM:"+String(currentWeather.hum)+"%");
  tft.setTextColor(ST7735_GREEN);
  String temp = String(currentWeather.tmp, 1) + (IS_METRIC ? "°C" : "°F");
  tft.println("TEMP:"+temp);
  // tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  String atmosphere = "AQI:" + currentWeather.aqi; 
  tft.println(atmosphere);
  atmosphere = "PM2.5:" + currentWeather.pm25;
  tft.println(atmosphere);
}

//天气预报
void drawForecast() {

  tft.fillScreen(ST7735_BLACK);
  int row = 28;
  tft.setCursor(1, 28);

  
  tft.setTextColor(ST7735_RED);
  drawForecastDetails(0);

  tft.setTextColor(ST7735_GREEN);
  drawForecastDetails(1);

  tft.setTextColor(ST77XX_YELLOW);
  drawForecastDetails(2);
  //天气预报
  tft.drawBitmap(2,96,yubao,72,36,ST77XX_YELLOW);
}
//预报详情
void drawForecastDetails( int dayIndex) {
  //天气状况

  
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.print(forecasts[dayIndex].iconMeteoCon);
    //字体16
  tft.setFont(&Arimo_Regular_24);
   tft.setTextSize(1);
  //预报 小时
  String intHou = String(forecasts[dayIndex].intHou) + ":00 ";
  tft.print(intHou);
  //预报 温度
  String temp = String(forecasts[dayIndex].tmp, 0) + " C";
  tft.println(temp);
  
  
}


//更新
void updateData() {
  forecastClient.updateForecastsById(forecasts, &currentWeather, JD_APP_ID, JD_LOCATION_ID, MAX_FORECASTS);


  readyForWeatherUpdate = false;
  tft.fillScreen(ST7735_BLACK);
  testdrawtext("Update Weather Done...",ST7735_RED);

}


//开始更新
void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

void WiFiConfig() {
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect("WeatherStation_AP","12345678");

  //Manual Wifi
  //WiFi.begin(WIFI_SSID, WIFI_PWD);
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);


  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    testdrawtext(".",ST7735_RED);


    counter++;
  }

  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST7735_GREEN);
   //字体16
  tft.setFont(&Arimo_Regular_16);
  tft.println("Connected!");
  tft.println(WiFi.SSID());
  tft.println(WiFi.localIP());

  Serial.println("WiFi Connected!");
  Serial.print("IP ssid: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP addr: ");
  Serial.println(WiFi.localIP());
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ST7735_RED);

  tft.println("Wifi Manager");
  tft.println("Please connect to AP");
  tft.println(myWiFiManager->getConfigPortalSSID());
  
  
}


void testdrawtext(String text, uint16_t color) {
  tft.setTextColor(color);
  //tft.setTextWrap(false);//关闭自动换行
  tft.println(text);
}
