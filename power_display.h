
#include "esphome.h"
#include "Preferences.h"

// Define the price levels (SEK/kWh)
#define BELOW_VERY_CHEAP_TEXT "Jättebilligt"
#define VERY_CHEAP 0.5
#define VERY_CHEAP_TEXT "Mycket billigt"
#define CHEAP 1.0
#define CHEAP_TEXT "Billigt"
#define NORMAL 1.5
#define NORMAL_TEXT "Normalt"
#define EXPENSIVE 3.0
#define EXPENSIVE_TEXT "Dyrt"
#define VERY_EXPENSIVE 4.0
#define VERY_EXPENSIVE_TEXT "Mycket dyrt"
#define EXTREMELY_EXPENSIVE 5.0
#define EXTREMELY_EXPENSIVE_TEXT "Extremt dyrt"

// Global functions to save and retrieve values from NVM, to survive a reboot

void SaveValueToNvm(String key, double value) {
	ESP_LOGD((String("SaveValueToNvm : ") + key).c_str(), String(value).c_str());
	Preferences preferences; 								// Create an instance of the preferences library
	preferences.begin("my_partition", false);				// Open the preferences partition
	preferences.putDouble(key.c_str(), value);		  		// Write the value to preferences
	preferences.end();
}

void SaveStringToNvm(String key, String value) {
	ESP_LOGD((String("SaveStringToNvm : ") + key).c_str(), value.c_str());
	Preferences preferences; 								// Create an instance of the preferences library
	preferences.begin("my_partition", false);				// Open the preferences partition
	preferences.putString(key.c_str(), value);		  		// Write the value to preferences
	preferences.end();
}

double LoadValueFromNvm(String key) {
	Preferences preferences; 								// Create an instance of the preferences library
	preferences.begin("my_partition", false);				// Open the preferences partition
	double value = preferences.getDouble(key.c_str(), 42); 	// Read the value from preferences
	preferences.end();
	ESP_LOGD((String("LoadValueFromNvm : ") + key).c_str(), String(value).c_str());
  return value;
}

String LoadStringFromNvm(String key) {
	Preferences preferences; 								// Create an instance of the preferences library
	preferences.begin("my_partition", false);				// Open the preferences partition
	String value = preferences.getString(key.c_str(), ""); 	// Read the value from preferences
	preferences.end();
	ESP_LOGD((String("LoadStringFromNvm : ") + key).c_str(), value.c_str());
  return value;
}

// Global variables. Needed to retain values after reboot
double currentPower, currentPrice, todayMaxPrice, dailyEnergy;
String TodaysPrices;

void SaveValuesToNVM () {
	if (currentPower != 0)
		SaveValueToNvm("CurrentPower", currentPower);
	if (currentPrice != 0)
		SaveValueToNvm("CurrentPrice", currentPrice);
	if (todayMaxPrice != 0)
		SaveValueToNvm("TodayMaxPrice", todayMaxPrice);
	if (TodaysPrices != "")
		SaveStringToNvm("TodaysPrices", TodaysPrices);
	if (dailyEnergy != 0)
		SaveValueToNvm("DailyEnergy", dailyEnergy);
}	
	
// PowerDisplay class:
class PowerDisplay : public Component {

public:

	void DisplayIcons (display::DisplayBuffer *buff, int x, int y) {
		if (currentPower >= 0)
			buff->image(x, y, &id(grid_power));
		else
			buff->image(x, y, &id(solar_power));
	}

	void CreateGraph (display::DisplayBuffer *buff, int x, int y, int width, int  height, Color color = COLOR_ON) {
		setPos(x, y);
		setWidth(width);
		setHeigh(height);		
		buff->rectangle(x, y, width, height, color);
	}

	void SetGraphScale (double xMin, double  xMax, double yMin) {		
		if (isnan(todayMaxPrice) || todayMaxPrice == 0)
			todayMaxPrice = LoadValueFromNvm("TodayMaxPrice");
		double  yMax = todayMaxPrice;
		xFactor = graphWidth / (xMax-xMin);
		yFactor = graphHeight / (yMax-yMin);
	}
	
	void SetGraphGrid(display::DisplayBuffer *buff, double xLow, double xInterval, double yLow, double yInterval, Color color = COLOR_ON) {
		double xLabel=0, yLabel = 0;
		double i2;
		Color labelColor = COLOR_CSS_WHITESMOKE;
		for(double i=(xPos + xLow*xFactor); i <= graphWidth+xPos; i+= xInterval*xFactor) {
//			ESP_LOGD("GraphGrid i: ", String(i).c_str());
			buff->line(i, yPos, i, yPos+graphHeight, color);
			buff->printf(i-4, yPos+graphHeight+10, &id(small_text),labelColor , TextAlign::BASELINE_LEFT, "%.0f", xLabel);
			xLabel += xInterval;
			i2 += xInterval*xFactor;
		}
		// For the last hour...
		buff->printf(i2+8, yPos+graphHeight+10, &id(small_text),labelColor , TextAlign::BASELINE_LEFT, "%.0f", xLabel);
		
		
		for(double j=(yLow*yFactor); j < graphHeight; j+= yInterval*yFactor) {
//			ESP_LOGD("GraphGrid j: ", String(j).c_str());
			buff->line(xPos, yPos+graphHeight-j, xPos+graphWidth, yPos+graphHeight-j, color);
			buff->printf(xPos-2, yPos+graphHeight-j, &id(small_text),labelColor , TextAlign::BASELINE_RIGHT, "%.1f", yLabel);
			yLabel += yInterval;
		}
	}
	
	// Functions to set the values from Home Assistant
	
	void SetCurrentPower(double power) {
		if (!isnan(power)) {
			currentPower = power;
		}
	}
	
	void SetCurrentPrice(double price) {
		if (!isnan(price)) {
			currentPrice = price;
		}
	}
	
	void SetTodayMaxPrice(double price) {
		if (!isnan(price) || price == 0) {
			todayMaxPrice = price;
		}
	}
	
	void SetTodaysPrices(String prices) {
		if (prices != "") {
			TodaysPrices = prices;
		}
	}

	void SetTomorrowsPrices(String prices) {
			TomorrowsPrices = prices;
		}
		
	void WriteDailyEnergy(double energy) {
		if (!isnan(energy)) {
			dailyEnergy = energy;
		}
	}
	
	// Display current power usage
	void WritePowerText(display::DisplayBuffer *buff, int x, int y) {
		if (isnan(currentPower) || currentPower == 0)
			currentPower = LoadValueFromNvm("CurrentPower");
		if (isnan(currentPrice) || currentPrice == 0)
			currentPrice = LoadValueFromNvm("CurrentPrice");	
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%.0f W", currentPower);		
	}
	// Display current price and the price level
	void WritePriceText(display::DisplayBuffer *buff, int x, int y) {
		
		if (isnan(currentPrice) || currentPrice == 0)
			currentPrice = LoadValueFromNvm("CurrentPrice");
		
		buff->printf(120, 257, &id(price_text), COLOR_CSS_WHITESMOKE, TextAlign::BASELINE_CENTER, "%.2f kr/kWh", currentPrice);
		
		String price;
		if (inRange(currentPrice, EXTREMELY_EXPENSIVE, 100)) {price = EXTREMELY_EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, VERY_EXPENSIVE, EXTREMELY_EXPENSIVE)){price = VERY_EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, EXPENSIVE, VERY_EXPENSIVE)){price = EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, NORMAL, EXPENSIVE)){price = NORMAL_TEXT;}
		else if (inRange(currentPrice, CHEAP, NORMAL)){price = CHEAP_TEXT;}
		else if (inRange(currentPrice, VERY_CHEAP, CHEAP)){price = VERY_CHEAP_TEXT;}
		else if (inRange(currentPrice, 0, VERY_CHEAP)){price = BELOW_VERY_CHEAP_TEXT;} 		
		
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%s", price.c_str());		
	}
	// Write the timeline on the graph
	void WriteTimeLine(display::DisplayBuffer *buff, double hour, double minute, Color color = COLOR_ON) {
		double timeLineVal = hour + (minute/60);
		buff->line(xPos + timeLineVal*xFactor, yPos, xPos + timeLineVal*xFactor, yPos+graphHeight, color);
	}
	// Write energy consumed so far today
	void WriteDailyAmount(display::DisplayBuffer *buff, int x, int y, Color color = COLOR_ON) {
		if (isnan(dailyEnergy) || dailyEnergy == 0) {			
			dailyEnergy = LoadValueFromNvm("DailyEnergy");		
		}
		buff->printf(x, y, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Idag: %.1f kWh", dailyEnergy);
		buff->printf(x, y+23, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Kostnad: %.2f kr", CalculateAccumulatedCost(currentPrice, dailyEnergy));				
	}

	// Draw the graph
	void DrawPriceGraph (display::DisplayBuffer *buff) {
		double lastprice = 0;
		double price;
  
		for (int priceCount=0;priceCount<24;priceCount++)
		{
			price = priceArray[priceCount];
			lastprice = AddPrice(buff, priceCount, price,  priceCount-1,  lastprice);
		}
		// Add last piece if we know the price at midninght tomorrow
		price = priceArrayTomorrow[0];
		if (price > 0)
			lastprice = AddPrice(buff, 24, price,  23,  lastprice);
	}
	
	// Deserialize the JSON string from NordPool
	void SetPrices(String day) {
		String prices;

		if (TodaysPrices == "")
			TodaysPrices = LoadStringFromNvm("TodaysPrices");
		
		if(day == "tomorrow")
			prices = TomorrowsPrices;
		else
			prices = TodaysPrices;
		
		prices.replace("[", "");
		prices.replace("]", " ");
		String array[25];
		int r=0,t=0;
		
		for(int i=0;i<prices.length();i++)
		{
			if(prices[i] == ' ' || prices[i] == ',')
			{
			if (i-r > 1)
			{
				array[t] = prices.substring(r,i);
				t++;
			}
			r = (i+1);
			}
		}

		for(int k=0 ;k<=t ;k++)
		{
//		ESP_LOGD("SetPrices Prices: ", array[k].c_str());
		if(day == "tomorrow")
			priceArrayTomorrow[k] = array[k].toFloat();
		else
			priceArray[k] = array[k].toFloat();
	  }
	}
	
	
private:
	display::DisplayBuffer *vbuff;
	
	int graphWidth, graphHeight, xPos, yPos;
	float xFactor, yFactor;
	
	double priceArray[50];
	double priceArrayTomorrow[50];	
	double prevDailyEnergy, accumulatedCost;
	String TomorrowsPrices;


	void setPos(int x, int y) {xPos = x; yPos = y;}
	void setWidth (int width) {graphWidth = width;}
	void setHeigh(int height) {graphHeight = height;}
	
	void DrawGraphLine(display::DisplayBuffer *buff, double x1, double x2, double y1, double y2, Color color = COLOR_ON) {
		buff->line(xPos + x1*xFactor, yPos+graphHeight-(y1*yFactor), xPos + x2*xFactor, yPos+graphHeight-(y2*yFactor), color);		
//		ESP_LOGD("GraphGrid x1: ", String(xPos + x1*xFactor).c_str());
	}

	double AddPrice(display::DisplayBuffer *buff, int hour, double price, int lastHour, double lastPrice)	{
		if(lastHour<0)
			lastHour=0;	  
		if(lastPrice==0)
			lastPrice = price;	
		DrawGraphLine(buff, lastHour, hour, lastPrice, price, PriceColour(lastPrice));
	  return price;
	}
	
	Color PriceColour (float nNewPrice) {
		Color colour;
		if (inRange(nNewPrice, EXTREMELY_EXPENSIVE, 100)) {colour = COLOR_CSS_MAROON;}
		else if (inRange(nNewPrice, VERY_EXPENSIVE, EXTREMELY_EXPENSIVE)){colour =  COLOR_CSS_RED;}
		else if (inRange(nNewPrice, EXPENSIVE, VERY_EXPENSIVE)){colour =  COLOR_CSS_ORANGE;}
		else if (inRange(nNewPrice, NORMAL, EXPENSIVE)){colour = COLOR_CSS_GREENYELLOW;}
		else if (inRange(nNewPrice, CHEAP, NORMAL)){colour = COLOR_CSS_GREEN;}
		else if (inRange(nNewPrice, VERY_CHEAP, CHEAP)){colour = COLOR_CSS_DARKGREEN;}
		else if (inRange(nNewPrice, 0, VERY_CHEAP)){colour = COLOR_CSS_DARKGREEN;}     
		return colour; 
	}
	
	double CalculateAccumulatedCost(double currentPrice, double dailyEnergy) {
		if (currentPrice == 0)
			return 0;
		double nEnergyDelta = dailyEnergy - prevDailyEnergy; 	
		prevDailyEnergy = dailyEnergy;
		accumulatedCost += (nEnergyDelta * currentPrice);
		//SaveValueToNvm("AccumulatedCost", accumulatedCost);
		return accumulatedCost;
	}

	bool inRange(float val, float minimum, float maximum)
	{
	  return ((minimum <= val) && (val <= maximum));
	}
	



}; //class








