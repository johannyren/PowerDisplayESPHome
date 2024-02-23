#include <sstream>
#include <numeric>
#include "esphome.h"

// Define the price levels (SEK/kWh)
#define BELOW_VERY_CHEAP_TEXT "Jättebilligt"
#define VERY_CHEAP 0.3
#define VERY_CHEAP_TEXT "Mycket billigt"
#define CHEAP 0.6
#define CHEAP_TEXT "Billigt"
#define NORMAL 0.9
#define NORMAL_TEXT "Normalt"
#define EXPENSIVE 1.5
#define EXPENSIVE_TEXT "Dyrt"
#define VERY_EXPENSIVE 2.0
#define VERY_EXPENSIVE_TEXT "Mycket dyrt"
#define EXTREMELY_EXPENSIVE 3.0
#define EXTREMELY_EXPENSIVE_TEXT "Extremt dyrt"

// Global variables. Needed to retain values after reboot
double currentPower, currentPrice, todayMaxPrice, dailyEnergy, accumulatedcosttoday, tomorrowsMaxPrice, tomorrowsMinPrice, tomorrowsAverage;
std::string TodaysPrices, TomorrowsPrices;
	
// PowerDisplay class:
class PowerDisplay : public Component {

public:

	void DisplayIcons (display::Display *buff, int x, int y) {
		if (currentPower >= 0)
			buff->image(x, y, &id(grid_power));
		else
			buff->image(x, y, &id(solar_power));
	}

	void CreateGraph (display::Display *buff, int x, int y, int width, int  height, Color color = COLOR_ON) {
		setPos(x, y);
		setWidth(width);
		setHeigh(height);		
		buff->rectangle(x, y, width, height, color);
	}

	void SetGraphScale (double xMin, double  xMax, double yMin) {		
		double  yMax = todayMaxPrice;
		xFactor = graphWidth / (xMax-xMin);
		yFactor = graphHeight / (yMax-yMin);
	}

	void SetGraphScaleTomorrow (double xMin, double  xMax, double yMin) {		
		double  yMax = tomorrowsMaxPrice;
		xFactor = graphWidth / (xMax-xMin);
		yFactor = graphHeight / (yMax-yMin);
	}
	
	void SetGraphGrid(display::Display *buff, double xLow, double xInterval, double yLow, double yInterval, Color color = COLOR_ON) {
		double xLabel=0, yLabel = 0;
		double i2 = 0;
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

	void SetTodaysPrices(std::string prices) {
		if (prices != "") {
			TodaysPrices = prices;
		}
	}

	void SetTomorrowsPrices(std::string prices) {
			TomorrowsPrices = prices;
		}

	void WriteDailyEnergy(double energy) {
		if (!isnan(energy)) {
			dailyEnergy = energy;
		}
	}

	void SetAccumulatedCostToday (double costtoday) {
		if (!isnan(costtoday)) {
			accumulatedcosttoday = costtoday;
		}
	}
	
	// Display current power usage
	void WritePowerText(display::Display *buff, int x, int y) {	
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%.0f W", currentPower);		
	}

	// Display text at the top of the second page
	void WriteTomorrowText(display::Display *buff, int x, int y) {	
		buff->print(x, y, &id(large_text), COLOR_CSS_WHITESMOKE, TextAlign::BASELINE_CENTER, "I morgon");		
	}

	// Display current price and the price level
	void WritePriceText(display::Display *buff, int x, int y) {
		
		buff->printf(120, 257, &id(price_text), COLOR_CSS_WHITESMOKE, TextAlign::BASELINE_CENTER, "%.2f kr/kWh", currentPrice);
		
		std::string price;
		if (inRange(currentPrice, EXTREMELY_EXPENSIVE, 100)) {price = EXTREMELY_EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, VERY_EXPENSIVE, EXTREMELY_EXPENSIVE)){price = VERY_EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, EXPENSIVE, VERY_EXPENSIVE)){price = EXPENSIVE_TEXT;}
		else if (inRange(currentPrice, NORMAL, EXPENSIVE)){price = NORMAL_TEXT;}
		else if (inRange(currentPrice, CHEAP, NORMAL)){price = CHEAP_TEXT;}
		else if (inRange(currentPrice, VERY_CHEAP, CHEAP)){price = VERY_CHEAP_TEXT;}
		else if (inRange(currentPrice, -100, VERY_CHEAP)){price = BELOW_VERY_CHEAP_TEXT;} 		
		
		buff->printf(x, y, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%s", price.c_str());		
	}

	// Write the timeline on the graph
	void WriteTimeLine(display::Display *buff, double hour, double minute, Color color = COLOR_ON) {
		double timeLineVal = hour + (minute/60);
		buff->line(xPos + timeLineVal*xFactor, yPos, xPos + timeLineVal*xFactor, yPos+graphHeight, color);
	}

	// Write energy consumed so far today
	void WriteDailyAmount(display::Display *buff, int x, int y, Color color = COLOR_ON) {
		buff->printf(x, y, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Idag: %.1f kWh", dailyEnergy);
		if (accumulatedcosttoday>0) {
			buff->printf(x, y+23, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Kostnad: %.2f kr", accumulatedcosttoday);
			} else {
			buff->printf(x, y+23, &id(energy_text), color, TextAlign::BASELINE_CENTER, "Kostnad: %.2f kr", CalculateAccumulatedCost(currentPrice, dailyEnergy));
			}				
	}

	// Write info about min, max and average price tomorrow
	void WritePriceInfo(display::Display *buff, int x, int y) {
		buff->printf(x, y, &id(energy_text), PriceColour(tomorrowsMinPrice), TextAlign::BASELINE_CENTER, "Lägsta pris: %.2f kr/kWh", tomorrowsMinPrice);
		buff->printf(x, y+23, &id(energy_text), PriceColour(tomorrowsMaxPrice), TextAlign::BASELINE_CENTER, "Högsta pris: %.2f kr/kWh", tomorrowsMaxPrice);
		buff->printf(x, y+182, &id(price_text), PriceColour(tomorrowsAverage), TextAlign::BASELINE_CENTER, "Snitt %.2f kr/kWh", tomorrowsAverage);

		if (!PriceVectorTomorrow.empty()) {
			std::string price;
			if (inRange(tomorrowsAverage, EXTREMELY_EXPENSIVE, 100)) {price = EXTREMELY_EXPENSIVE_TEXT;}
			else if (inRange(tomorrowsAverage, VERY_EXPENSIVE, EXTREMELY_EXPENSIVE)){price = VERY_EXPENSIVE_TEXT;}
			else if (inRange(tomorrowsAverage, EXPENSIVE, VERY_EXPENSIVE)){price = EXPENSIVE_TEXT;}
			else if (inRange(tomorrowsAverage, NORMAL, EXPENSIVE)){price = NORMAL_TEXT;}
			else if (inRange(tomorrowsAverage, CHEAP, NORMAL)){price = CHEAP_TEXT;}
			else if (inRange(tomorrowsAverage, VERY_CHEAP, CHEAP)){price = VERY_CHEAP_TEXT;}
			else if (inRange(tomorrowsAverage, -100, VERY_CHEAP)){price = BELOW_VERY_CHEAP_TEXT;} 		
		
			buff->printf(x, y+222, &id(large_text), PriceColour(currentPrice), TextAlign::BASELINE_CENTER, "%s", price.c_str());
		}
	}

	// Draw the graph
	void DrawPriceGraph (display::Display *buff) {
		double lastprice = 0;
		double price = 0;
		if (!PriceVector.empty()) {
			for (int priceCount=0;priceCount<24;priceCount++) {
				price = PriceVector.at(priceCount);
				lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
			}
		}
		if (!PriceVectorTomorrow.empty()) {
			price = PriceVectorTomorrow.at(0);
			if (price > 0)
				lastprice = AddPrice(buff, 24, price, 23, lastprice);
		}
	}

	// Draw the graph tomorrow
	void DrawPriceGraphTomorrow (display::Display *buff) {
		double lastprice = 0;
		double price = 0;

		if (!PriceVectorTomorrow.empty()) {
		tomorrowsMaxPrice = *max_element(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end());
		tomorrowsMinPrice = *min_element(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end());
		tomorrowsAverage = accumulate(PriceVectorTomorrow.begin(), PriceVectorTomorrow.end() ,0.0) / PriceVectorTomorrow.size();

		for (int priceCount=0;priceCount<24;priceCount++) {
			price = PriceVectorTomorrow.at(priceCount);
			lastprice = AddPrice(buff, priceCount, price, priceCount-1, lastprice);
			}
		}
		if (PriceVectorTomorrow.empty()) {
			tomorrowsMaxPrice = 0;
			tomorrowsMinPrice = 0;
			tomorrowsAverage = 0;
		}

	}

	// Deserialize the JSON string from NordPool and put it into two vectors
	void SetPrices(std::string day) {
		std::string prices;

		if(day == "today") {
			prices = TodaysPrices;

			if (prices.length() > 10) {
				prices.erase(0, 1);
				prices.replace(prices.size() - 1, 1, " ");

				PriceVector.clear();
				std::stringstream ss(prices);
				double i;
				while (ss >> i)
				{
					PriceVector.push_back(i);
					if (ss.peek() == ',')
						ss.ignore();
				}
		//  Uncomment the lines below to get log messages to show the contents of the Todays vector
		//		for (auto it = PriceVector.begin(); it != PriceVector.end(); it++) { 
		//			ss << *it << " ";
		//		}
		//	ESP_LOGD("TodaysPrices: ", ss.str().c_str());
			}

		}

		if(day == "tomorrow") {
			prices = TomorrowsPrices;
			
			if (prices == "" || prices == "[]") {
				PriceVectorTomorrow.clear();
			}

   			if (prices.length() > 10) {
				prices.erase(0, 1);
				prices.replace(prices.size() - 1, 1, " ");

				PriceVectorTomorrow.clear();
				std::stringstream st(prices);
				double i;
				while (st >> i)
				{
					PriceVectorTomorrow.push_back(i);
					if (st.peek() == ',')
						st.ignore();
				}
		//  Uncomment the lines below to get log messages to show the contents of the Tomorrow vector
		//		for (auto it = PriceVectorTomorrow.begin(); it != PriceVectorTomorrow.end(); it++) { 
		//			st << *it << " ";
		//		}
		//	ESP_LOGD("TomorrowsPrices: ", st.str().c_str());
			}
		}
	}

private:
	display::Display *vbuff;

	int graphWidth, graphHeight, xPos, yPos;
	float xFactor, yFactor;

	double prevDailyEnergy, accumulatedCost;

	std::vector<double> PriceVector;
	std::vector<double> PriceVectorTomorrow;

	void setPos(int x, int y) {xPos = x; yPos = y;}
	void setWidth (int width) {graphWidth = width;}
	void setHeigh(int height) {graphHeight = height;}
	
	void DrawGraphLine(display::Display *buff, double x1, double x2, double y1, double y2, Color color = COLOR_ON) {
		buff->line(xPos + x1*xFactor, yPos+graphHeight-(y1*yFactor), xPos + x2*xFactor, yPos+graphHeight-(y2*yFactor), color);		
//		ESP_LOGD("GraphGrid x1: ", String(xPos + x1*xFactor).c_str());
	}

	double AddPrice(display::Display *buff, int hour, double price, int lastHour, double lastPrice)	{
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
		else if (inRange(nNewPrice, -100, VERY_CHEAP)){colour = COLOR_CSS_DARKGREEN;}     
		return colour; 
	}
	
	double CalculateAccumulatedCost(double currentPrice, double dailyEnergy) {
		if (currentPrice == 0)
			return 0;
		double nEnergyDelta = dailyEnergy - prevDailyEnergy; 	
		prevDailyEnergy = dailyEnergy;
		accumulatedCost += (nEnergyDelta * currentPrice);
		return accumulatedCost;
	}

	bool inRange(float val, float minimum, float maximum)
	{
	  return ((minimum <= val) && (val <= maximum));
	}

}; //class