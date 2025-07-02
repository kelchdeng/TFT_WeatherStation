/**The MIT License (MIT)

 Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#pragma once

typedef struct ForecastData
{

	// 日期时间
	String date;
	// 小时 整数
	uint8_t intHou;
	// 白天0，黑夜1
	uint8_t dayNight;
	// 温度
	String tmp;
	// 天气代码
	int condCode;
	// 天气图标
	String iconMeteoCon;
	// 天气描述
	String condTxt;
	// 相对湿度
	String hum;
	// 空气质量
	String aqi;
	// 空气质量 描述
	String qlty;
	// pm2.5
	String pm25;
	// 夜晚天气图标
	String nightIconeteoCon;
	// 夜晚天气描述
	String nightcondTxt;
	// 最高温度
	String hightTmp;
	// 最低温度
	String lowTmp;
	//风力等级
	String windClass;

} ForecastData;

class Forecast
{
private:
	ForecastData *fData;
	ForecastData *nowData;
	void doUpdate(ForecastData *data, ForecastData *nowData, String url); // http请求数据
	// 请求实时天气
	void httpGetNow(String url);
	// 请求天气预报
	void httpGetForecasts(String url);
	String gaodeUrl(); // 生成当天天气URL
	String getCity(String url);					   // 调用高德ip定位获取当前城市adcode，例如430100长沙
	void liveNowParse(String fJson);			   // 当天天气
	void threeDayForecastsParse(String fJson);		   // 解析小时预报json，生成JDForecastData数组
	uint8_t maxForecasts;
	//请求长沙环保局空气质量
	void httpGetcsAQI();
	void aqiParse(String fJson);
  String GAODE_AK = "";
  String CITY_URL = "https://restapi.amap.com/v3/ip?key=" + GAODE_AK;
  String WEATHER_URL = "https://restapi.amap.com/v3/weather/weatherInfo?key="+GAODE_AK+"&city=";

public:
	Forecast();
	void updateForecastsById(ForecastData *data, ForecastData *nowData, uint8_t maxForecasts); // 对外提供数据方法，返回JDForecastData 数组和当前天气对象

	// String getMeteoconIcon(int icon,int dn);
	String getIcon(String text, int dn);
};
