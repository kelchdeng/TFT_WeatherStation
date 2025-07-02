/**dengjie
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
// time
#include <time.h>      // time() ctime()
#include <sys/time.h>  // struct timeval
#include <coredecls.h> // settimeofday_cb()
// OTA
#include <ArduinoOTA.h>
// 多线程
// #include <Ticker.h>
// TFT
#include <Adafruit_GFX.h>    // Core graphics library Adafruit_GFX_Library
#include <Adafruit_ST7735.h> // Hardware-specific library Adafruit_ST7735_and_ST7789_Library
#include <SPI.h>

#include "Forecast.h"
#include "GFXWeatherFont.h"

// 点灯lib
//  #define BLINKER_WIFI
// #define BLINKER_APCONFIG

// #include <Blinker.h>

// 自定义RGB 565颜色 R 取二进制高位5位，G取二进制高位6位，B取二进制高位5位，最后拼成二进制后转16进制
// 深绿色#007F00
#define DGREEN 0x03E0
// #569cd6
#define DIY_BR 0xACFA
// #b5cea8
#define DIY_NEW_GREEN 0xB675
// #4EC9B0
#define DIY_C_GREEN 0x9E56
// #5A96FF
#define DIY_L_GREEN 0xB49F
// #f4a460
#define DIY_SANDY_BROWN 0xF52C

/***************************
   Begin Settings
 **************************/
#define HOSTNAME "ESP8266-TQ-"

// #TFT
//  For the breakout, you can use any 2 or 3 pins
//  These pins will also work for the 1.8" TFT shield
// 文字取模可以用PCtoLCD
#define TFT_CS D0   // cs
#define TFT_DC D1   // dc
#define TFT_RST D2  // res
#define TFT_MOSI D3 // sda
#define TFT_SCLK D4 // scl

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
// #TFT

#define TZ 8     // (utc+) 时区
#define DST_MN 0 // use 60mn for summer time in some countries中国没有夏令时，赋0

// Setup
const int UPDATE_INTERVAL_SECS = 60 * 60; // Update every 3 hour 3小时更新

#define DHTPIN D5 // D5 Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.
// Uncomment the type of sensor in use:
#define DHTTYPE DHT11 // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);

// https://wx.jdcloud.com/market/datas/26/10610
// const String OPEN_WEATHER_MAP_APP_ID = "";
// https://wx.jdcloud.com/market/datas/26/10610
// const String OPEN_WEATHER_MAP_LOCATION_ID = "";

// https://wx.jdcloud.com/market/datas/26/10610 京东和风 appid
//const String JD_APP_ID = "";
// https://wx.jdcloud.com/market/datas/26/10610 京东和风 城市
// 弃用
// const String JD_LOCATION_ID = "";
// 高德 api 已放到头文件


// 弃用
String OPEN_WEATHER_MAP_LANGUAGE = "zh_cn";
// 最大预报数
const uint8_t MAX_FORECASTS = 4;

const boolean IS_METRIC = true;

// Adjust according to your language
// const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
// const String WDAY_NAMES_NO[] = {"7", "1", "2", "3", "4", "zhou_5", "6"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/***************************
   End Settings
 **************************/

// 当前天气对象实例
ForecastData currentWeather;
// 当前天气客户端对象实例
// OpenWeatherMapCurrent currentWeatherClient;
// 预报对象实例数组，数组大小是MAX_FORECASTS定义的值
ForecastData forecasts[MAX_FORECASTS];
// 预报客户端对象实例
Forecast forecastClient;

#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)
time_t now;

// flag changed in the ticker function every 10 minutes 标记更新
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

// declaring prototypes 声明
void configModeCallback(WiFiManager *myWiFiManager);

// 绘制大字时间
void drawBigTime();
// 绘制日期时间
void drawDateTime();
// 绘制当前天气
void drawNowWeather();
// 绘制预报
void drawForecast();
void updateData();

void drawForecastDetails(int dayIndex);

void setReadyForWeatherUpdate();

int numberOfOverlays = 1;
int h = 0;

// Ticker 调用间隔(秒)
// const int blinkInterval = 1;
// Ticker ticker;

const int INTERVAL_SECS = 3; // tft 当前屏幕停留时间（秒）
long now_ms = 0;             // 当前毫秒数
int iFlag = 0;               // 显示标识，0大时间，1时钟，2当前天气，3小时预报
// 点灯lib参数
//  char auth[] = "6bfd4db92425";//"cb343a2932cc";6bfd4db92425
//  char ssid[] = "";
//  char pswd[] = "";

// BlinkerNumber HUMI("humi");
// BlinkerNumber TEMP("temp");
// BlinkerNumber DCV("dcv");
// float humi_read, temp_read;
// float dcv_read;
// void dataRead(const String & data)
// {
//     Serial.println("Blinker readString: "+data);

//     Blinker.vibrate();

//     //uint32_t BlinkerTime = millis();

//     //Blinker.print("millis", BlinkerTime);
// }

// void heartbeat()
// {
//     HUMI.print(humi_read);
//     TEMP.print(temp_read);
//     DCV.print(dcv_read);

// }
// 点灯lib参数
// led 矩阵
const int N74_ST_BIG = D6;
const int N74_SH_PUSH = D7;
const int N74_DS_DATA = D8;
const int row[8] = {128, 64, 32, 176, 181, 133, 253, 255};
// 自定义
const int led_diy[10][8] = {
    {255, 159, 111, 111, 111, 111, 159, 255}, /*"0",*/
    {223, 159, 95, 223, 223, 223, 223, 143},  /*"1",*/
    {159, 111, 239, 223, 191, 127, 15, 255},  /*"2",*/
    //{159,111,239,223,191,111,31,255},/*"2",*/
    //{143,239,239,223,159,239,239,15},/*"3",*/
    {143, 239, 239, 223, 159, 239, 111, 143}, /*"3",*/
    {223, 159, 95, 95, 15, 223, 223, 223},    /*"4",*/
    //{15,127,127,127,15,239,239,15},/*"5",*/
    {143, 127, 127, 31, 239, 239, 111, 159}, /*"5",*/
    //{15,127,127,127,15,111,111,15},/*"6",*/
    {159, 111, 127, 127, 31, 111, 111, 159},  /*"6",*/
    {15, 239, 239, 223, 191, 191, 191, 191},  /*"7",*/
    {159, 111, 111, 159, 159, 111, 111, 159}, /*"8",*/
    //{15,111,111,15,239,239,239,15}/*"9",*/
    {159, 111, 111, 15, 239, 239, 111, 159}, /*"9",*/
};

const int dot[] = {255, 255, 159, 255, 159, 255, 255, 255}; /*"点",：*/
const int yue[] = {193, 221, 193, 221, 193, 221, 185, 255}; /*"月",0*/
const int nian[] = {239, 192, 183, 65, 215, 129, 247, 247}; /*"年",0*/
// 显示计数器
int cnt = 0;
// 清屏
void clean();
// 显示时间
void showTime();
// 显示日期
void showDate();
// 显示年
void showYear();

void setup()
{
  Serial.begin(115200);
  // BLINKER_DEBUG.stream(Serial);
  Serial.println("start..");

  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip, black tab
  // tft.fillScreen(ST7735_WHITE);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);
  tft.setRotation(tft.getRotation() + 1);
  //  tft.setCursor(5, 5);

  // Initialize device.
  dht.begin();

  WiFiConfig();

  // Get time from network time service. ntp服务设置
  configTime(TZ_SEC, DST_SEC, "ntp1.aliyun.com");

  updateData();

  // ticker.attach(blinkInterval, showTFT);

  // OTA设置并启动 wifi连接成功后执行
  ArduinoOTA.setHostname("ESP8266-TFT-WeatherStation");
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin();

  Serial.println(WiFi.SSID() + ":" + WiFi.psk());
  // 连上wifi后初始点灯lib
  //  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
  // Blinker.begin(auth, ssid, pswd);
  // Blinker.begin(auth);
  //  Blinker.attachData(dataRead);
  //  Blinker.attachHeartbeat(heartbeat);
  //  Serial.println("blinker");

  // led 设置
  pinMode(N74_ST_BIG, OUTPUT);
  pinMode(N74_DS_DATA, OUTPUT);
  pinMode(N74_SH_PUSH, OUTPUT);
  clean();
}

void loop()
{
  ArduinoOTA.handle();
  //  Blinker.run();
  if (millis() - timeSinceLastWUpdate > (1000L * UPDATE_INTERVAL_SECS))
  {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate)
  {
    updateData();
  }

  if (millis() - now_ms > (1000L * INTERVAL_SECS))
  {

    showTFT();
    now_ms = millis();
  }

  // # led 矩阵
  // 时间
  showTime();

  // 显示5秒时间
  //  if(cnt<5000){
  //    //时间
  //    showTime();
  //  }else if(cnt<8000){
  //    //日期
  //    showDate();
  //  }else{
  //    //显示年
  //    showYear();
  //  }
  //  cnt++;
  //  //总共10秒一轮回，3秒显示日期
  //  if(cnt>10000){
  //    cnt = 0;
  //  }
  // #矩阵
}

void showTFT()
{
  switch (iFlag)
  {
  case 0:
    drawBigTime();
    iFlag++;
    break;
  case 1:
    drawDateTime();
    iFlag++;
    break;
  case 2:
    drawNowWeather();
    iFlag++;
    break;
  case 3:
    drawForecast();
    iFlag = 0;
    break;
  }
  // drawBigTime();
  // delay(5000);

  // drawDateTime();
  // delay(5000);

  // drawNowWeather();
  // delay(5000);

  // drawForecast();
  // delay(5000);
}

// 绘制大字时间
void drawBigTime()
{
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  // 清屏
  tft.fillScreen(ST7735_BLACK);
  // 上横线
  tft.drawFastHLine(0, 35, 160, ST77XX_BLUE);
  // 下横线
  tft.drawFastHLine(0, 85, 160, ST77XX_RED);
  // 天气图标
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.setCursor(4, 32);
  tft.setTextColor(DIY_C_GREEN);
  tft.println(currentWeather.iconMeteoCon);

  // 时间
  tft.drawBitmap(45, 5, shijian, 72, 36, DIY_C_GREEN);

  tft.setCursor(5, 50);
  tft.setFont(NULL);
  tft.setTextSize(5);

  //  tft.setCursor(10, 80);
  //  tft.setFont(&Arimo_Regular_16);
  //  tft.setTextSize(3);

  tft.setTextColor(DIY_SANDY_BROWN);
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  tft.println(String(buff));

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  // 字体16
  tft.setFont(&Arimo_Regular_16);

  tft.setCursor(1, 104);
  tft.setTextSize(1);
  if (isnan(event.temperature))
  {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
    tft.setTextColor(DIY_C_GREEN);
    String temp = String(event.temperature, 1) + "C";
    // tft.print("NOW T : ");
    // 温度
    tft.drawBitmap(10, 88, wen_du, 24, 12, DIY_C_GREEN);
    tft.setCursor(40, 100);
    tft.setTextColor(DIY_SANDY_BROWN);
    tft.println(temp);
    // temp_read = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println(F("Error reading humidity!"));
  }
  else
  {
    // tft.setTextColor(DIY_C_GREEN);
    // tft.print("NOW H : ");
    // 湿度
    tft.drawBitmap(10, 108, shi_du, 24, 12, DIY_C_GREEN);
    tft.setCursor(40, 120);
    tft.setTextColor(DIY_SANDY_BROWN);
    tft.println(String(event.relative_humidity) + "%");
    // humi_read = event.relative_humidity;
  }

  // 刷新温湿度
  //  int dv = analogRead(A0);
  //  int v = map(dv,0,1024,0,330);
  //  dcv_read = (float)v/100.00;//两位小数
}

// 绘制日期时间
void drawDateTime()
{

  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  // 清屏
  tft.fillScreen(ST7735_BLACK);
  // 下横线
  tft.drawFastHLine(0, 96, 160, ST77XX_RED);

  //  tft.setCursor(2, 24);
  //  //字体28
  //  tft.setFont(&Arimo_Regular_24);
  // String wday = WDAY_NAMES[timeInfo->tm_wday];
  // String wday = WDAY_NAMES_NO[timeInfo->tm_wday];

  tft.setTextColor(DIY_C_GREEN);
  // 星期
  // tft.println(wday);
  // tft.drawBitmap(2,5,wday,72,36,DIY_C_GREEN);
  switch (timeInfo->tm_wday)
  {
  case 0:
    tft.drawBitmap(2, 5, zhou_7, 72, 36, DIY_C_GREEN);
    break;
  case 1:
    tft.drawBitmap(2, 5, zhou_1, 72, 36, DIY_C_GREEN);
    break;
  case 2:
    tft.drawBitmap(2, 5, zhou_2, 72, 36, DIY_C_GREEN);
    break;
  case 3:
    tft.drawBitmap(2, 5, zhou_3, 72, 36, DIY_C_GREEN);
    break;
  case 4:
    tft.drawBitmap(2, 5, zhou_4, 72, 36, DIY_C_GREEN);
    break;
  case 5:
    tft.drawBitmap(2, 5, zhou_5, 72, 36, DIY_C_GREEN);
    break;
  case 6:
    tft.drawBitmap(2, 5, zhou_6, 72, 36, DIY_C_GREEN);
    break;
  default:
    break;
  }
  // 横线
  tft.drawFastHLine(0, 38, 160, ST77XX_BLUE);
  //  tft.setCursor(45, 40);
  // 字体28
  tft.setFont(&Arimo_Regular_24);
  //  tft.setTextSize(2);
  //  tft.println(wday);
  tft.setCursor(2, 60);
  tft.setTextSize(1);
  // 时间
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  tft.setTextColor(DIY_SANDY_BROWN);
  tft.println(String(buff));
  // 年-月-日
  sprintf_P(buff, PSTR("%04d-%02d-%02d"), timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
  tft.setTextColor(DIY_C_GREEN);
  tft.setTextSize(1);
  // tft.setTextSize(2);
  tft.println(String(buff));

  // 天气图标
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.setCursor(4, 122);
  tft.setTextColor(DIY_C_GREEN);
  tft.println(currentWeather.iconMeteoCon);

  // 日期
  tft.setTextColor(DIY_C_GREEN);
  tft.drawBitmap(40, 96, riqi, 72, 36, DIY_C_GREEN);
}

// 绘制当前天气
void drawNowWeather()
{

  // 清屏
  tft.fillScreen(ST7735_BLACK);
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.setCursor(2, 28);
  // 天气图标
  tft.setTextColor(DIY_SANDY_BROWN);
  tft.println(currentWeather.iconMeteoCon);
  // 天气
  tft.drawBitmap(40, 5, tianqi, 72, 36, DIY_C_GREEN);
  // 横线
  tft.drawFastHLine(0, 40, 160, ST77XX_RED);
  // 竖线
  tft.drawFastVLine(55, 40, 128, ST77XX_BLUE);
  // 字体16,
  tft.setFont(&Arimo_Regular_16);
  // tft.setTextColor(DIY_SANDY_BROWN);
  // tft.print("     RH   ");
  // 湿度
  tft.drawBitmap(20, 43, shi_du, 24, 12, DIY_SANDY_BROWN);
  tft.setCursor(65, 55);
  tft.setTextColor(DIY_C_GREEN); // 值和名称颜色不一样，观看方便
  tft.println(currentWeather.hum + " %");

  // tft.setTextColor(DIY_SANDY_BROWN);
  String temp = currentWeather.tmp + " C";
  // tft.print("TEMP   ");
  // 温度
  tft.drawBitmap(20, 58, wen_du, 24, 12, DIY_SANDY_BROWN);
  tft.setCursor(65, 70);
  tft.setTextColor(DIY_C_GREEN);
  tft.println(temp);

  // tft.setFont(&Meteocons_Regular_28);
  // tft.println(char(42));
  // //字体16
  // tft.setFont(&Arimo_Regular_16);
  // tft.setTextSize(1);
  // tft.setTextColor(DIY_SANDY_BROWN);
  // String atmosphere = "    AQI   ";
  // tft.print(atmosphere);
  // 风力
  tft.drawBitmap(20, 73, feng_li, 24, 12, DIY_SANDY_BROWN);
  tft.setCursor(65, 85);
  tft.setTextColor(DIY_C_GREEN);
  // atmosphere = currentWeather.aqi;
  tft.println(currentWeather.windClass);

  tft.setTextColor(DIY_SANDY_BROWN);
  String atmosphere = "    AQI   ";
  tft.print(atmosphere);
  tft.setTextColor(DIY_C_GREEN);
  atmosphere = currentWeather.aqi;
  tft.println(atmosphere);
}

// 天气预报
void drawForecast()
{

  tft.fillScreen(ST7735_BLACK);
  int row = 28;
  tft.setCursor(1, row);
  // 横线
  tft.drawFastHLine(0, 96, 160, ST77XX_RED);
  // 竖线
  tft.drawFastVLine(96, 0, 96, ST77XX_BLUE);

  tft.setTextColor(DIY_C_GREEN);
  drawForecastDetails(0);

  tft.setTextColor(DIY_SANDY_BROWN);
  drawForecastDetails(1);

  tft.setTextColor(DIY_C_GREEN);
  drawForecastDetails(2);
  // 字体16
  tft.setFont(&Arimo_Regular_24);
  tft.setTextSize(1);
  // 天气预报
  tft.drawBitmap(2, 96, yubao, 72, 36, DIY_C_GREEN);
  // 时间
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  tft.setCursor(90, 125);
  tft.setTextColor(DIY_C_GREEN);
  tft.println(String(buff));
}
// 预报详情
// 预报详情
void drawForecastDetails(int dayIndex)
{
  // 天气状况
  char buff[16];
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.print(forecasts[dayIndex].iconMeteoCon);
  // 字体16
  tft.setFont(&Arimo_Regular_24);
  tft.setTextSize(1);
  // 预报日期
  String showDT = forecasts[dayIndex].date.substring(5);

  tft.print(showDT + " ");
  // 预报 温度
  //  String temp = String(forecasts[dayIndex].hightTmp) + "C";
  //  Serial.println(temp);
  tft.print(forecasts[dayIndex].hightTmp);
  // 摄氏度图标
  tft.setFont(&Meteocons_Regular_28);
  tft.setTextSize(1);
  tft.println("*");
}
// void drawForecastDetails( int dayIndex) {
//   //天气状况
//   char buff[16];
//   tft.setFont(&Meteocons_Regular_28);
//   tft.setTextSize(1);
//   tft.print(forecasts[dayIndex].iconMeteoCon);
//     //字体16
//   tft.setFont(&Arimo_Regular_24);
//    tft.setTextSize(1);
//   //预报 小时
//   sprintf_P(buff, PSTR("%02d"), forecasts[dayIndex].intHou);
//   String intHou = String(buff) + ":00 ";
//   tft.print(intHou);
//   //预报 温度
//   String temp = String(forecasts[dayIndex].tmp, 0) + "C";
//   tft.println(temp);

// }

// 更新
void updateData()
{
  forecastClient.updateForecastsById(forecasts, &currentWeather, MAX_FORECASTS);

  readyForWeatherUpdate = false;
  tft.fillScreen(ST7735_BLACK);
  testdrawtext("Update Weather Done...", ST77XX_ORANGE);
  delay(1000);
}

// 开始更新
void setReadyForWeatherUpdate()
{
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

void WiFiConfig()
{
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // Uncomment for testing wifi manager
  // wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  // or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect("WeatherStation_AP", "12345678");

  // Manual Wifi
  // WiFi.begin(WIFI_SSID, WIFI_PWD);
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if(counter>2000){
      return;
    }
    delay(500);
    Serial.print(".");
    testdrawtext(".", DIY_C_GREEN);

    counter++;
  }

  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(DIY_SANDY_BROWN);
  // 字体16
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

void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(DIY_C_GREEN);

  tft.println("Wifi Manager");
  tft.println("Please connect to AP");
  tft.println(myWiFiManager->getConfigPortalSSID());
}

void testdrawtext(String text, uint16_t color)
{
  tft.setTextColor(color);
  // tft.setTextWrap(false);//关闭自动换行
  tft.setCursor(2, 2);
  tft.println(text);
}

// 显示时间
void showTime()
{
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  // 时间
  // sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  // Serial.println(String(buff));
  // 年-月-日
  // sprintf_P(buff, PSTR("%04d-%02d-%02d"),timeInfo->tm_year + 1900,timeInfo->tm_mon + 1,timeInfo->tm_mday );
  // Serial.println(String(buff));
  int h10 = timeInfo->tm_hour / 10;
  int h0 = timeInfo->tm_hour % 10;
  int m10 = timeInfo->tm_min / 10;
  int m0 = timeInfo->tm_min % 10;
  // 要多移位一次
  for (int col = 0; col < 9; col++)
  {
    // 逐行
    digitalWrite(N74_ST_BIG, LOW);
    int showH10 = (led_diy[h10][col] >> 4) << 4;
    int showH0 = (led_diy[h0][col] >> 4) << 7;
    int showM0 = led_diy[m0][col] >> 4;
    int showM10 = (led_diy[m10][col] >> 4) << 5;

    int d = dot[col] >> 4;
    // 都先右移4位，得到高4位，然后显示在前的字符，左移4位当高位，再加上显示在后的字符的高位。
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showM10 + 16 + showM0);                                 // MSBFIRST 高位先入 LSBFIRST低位先入 分钟个位,15是补低位四个1
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showH0 + 64 + (d << 2) + 2 + (led_diy[m10][col] >> 7)); // MSBFIRST 高位先入 LSBFIRST低位先入 点 和分钟十位
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showH10 + 8 + (led_diy[h0][col] >> 5));                 // MSBFIRST 高位先入 LSBFIRST低位先入 小时
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 128 >> col);                                            // MSBFIRST 位先入 LSBFIRST低位先入
    digitalWrite(N74_ST_BIG, HIGH);
    delay(0.2);
  }
}

// 显示日期
void showDate()
{
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  // 年-月-日
  // sprintf_P(buff, PSTR("%04d-%02d-%02d"),timeInfo->tm_year + 1900,timeInfo->tm_mon + 1,timeInfo->tm_mday );
  // Serial.println(String(buff));
  int mon = timeInfo->tm_mon + 1;
  int mon10 = mon / 10;
  int mon0 = mon % 10;
  int d10 = timeInfo->tm_mday / 10;
  int d0 = timeInfo->tm_mday % 10;
  // 要多移位一次
  for (int col = 0; col < 9; col++)
  {
    // 逐行
    digitalWrite(N74_ST_BIG, LOW);
    int showM10 = (led_diy[mon10][col] >> 4) << 4;
    int showD10 = (led_diy[d10][col] >> 4) << 4;

    int d = dot[col] >> 4;
    // 都先右移4位，得到高4位，然后显示在前的字符，左移4位当高位，再加上显示在后的字符的高位。
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showD10 + (led_diy[d0][col] >> 4));   // MSBFIRST 高位先入 LSBFIRST低位先入 日
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, yue[col]);                            // MSBFIRST 高位先入 LSBFIRST低位先入 月字
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showM10 + (led_diy[mon0][col] >> 4)); // MSBFIRST 高位先入 LSBFIRST低位先入 月
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 128 >> col);                          // MSBFIRST 位先入 LSBFIRST低位先入
    digitalWrite(N74_ST_BIG, HIGH);
    delay(0.2);
  }
}

// 显示年
void showYear()
{
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  // 年-月-日
  // sprintf_P(buff, PSTR("%04d-%02d-%02d"),timeInfo->tm_year + 1900,timeInfo->tm_mon + 1,timeInfo->tm_mday );
  // Serial.println(String(buff));
  int y = timeInfo->tm_year + 1900;
  int hxx = y / 100;
  int lxx = y % 100;
  int y10 = hxx / 10;
  int y0 = hxx % 10;
  int t10 = lxx / 10;
  int t0 = lxx % 10;
  // 要多移位一次
  for (int col = 0; col < 9; col++)
  {
    // 逐行
    digitalWrite(N74_ST_BIG, LOW);
    int showY10 = (led_diy[y10][col] >> 4) << 4;
    int showT10 = (led_diy[t10][col] >> 4) << 4;

    int d = dot[col] >> 4;
    // 都先右移4位，得到高4位，然后显示在前的字符，左移4位当高位，再加上显示在后的字符的高位。
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, nian[col]);                         // MSBFIRST 高位先入 LSBFIRST低位先入 年字
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showT10 + (led_diy[t0][col] >> 4)); // MSBFIRST 高位先入 LSBFIRST低位先入 年低位
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, showY10 + (led_diy[y0][col] >> 4)); // MSBFIRST 高位先入 LSBFIRST低位先入 年高位
    shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 128 >> col);                        // MSBFIRST 位先入 LSBFIRST低位先入
    digitalWrite(N74_ST_BIG, HIGH);
    delay(0.2);
  }
}
// 清屏
void clean()
{
  digitalWrite(N74_ST_BIG, LOW);
  shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 255); // MSBFIRST 高位先入 LSBFIRST低位先入
  shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 255); // MSBFIRST 高位先入 LSBFIRST低位先入
  shiftOut(N74_DS_DATA, N74_SH_PUSH, LSBFIRST, 255); // MSBFIRST 高位先入 LSBFIRST低位先入
  shiftOut(N74_DS_DATA, N74_SH_PUSH, MSBFIRST, 0);   // MSBFIRST 高位先入 LSBFIRST低位先入
  digitalWrite(N74_ST_BIG, HIGH);
}
