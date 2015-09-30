#include "heatminder.h"

HEATMINDER::HEATMINDER(uint8_t SSR)
{
    _SSR_pin = SSR;
    pinMode(_SSR_pin, OUTPUT);
    digitalWrite(_SSR_pin, LOW);
    
    morning_start_hour = 0;
    morning_start_minute = 0;
    morning_end_hour = 0;
    morning_end_minute = 0;
    evening_start_hour = 0;
    evening_start_minute = 0;
    evening_end_hour = 0;
    evening_end_minute = 0;

    init_state = 1;
    time_state = 1;
	tempC_inside = 0;
	tempC_outside = 0;
	tempC_flow = 0;
	tempC_return = 0;
	tempC_test = 0;

    set_temp_morning_C = 0;
    set_temp_evening_C = 0;
    anti_cond_temp_C = 0;
    delta_temp = 0;
}

void HEATMINDER::set_sensor(DallasTemperature* sens, DeviceAddress insideThermometer, \
		DeviceAddress outsideThermometer, DeviceAddress flowThermometer, \
		DeviceAddress returnThermometer, DeviceAddress test_address)
{
	_sensors = sens;

    _sensors->begin();
    // set the resolution to 10 bit (good enough?)
    _sensors->setResolution(insideThermometer, 10);
    _sensors->setResolution(outsideThermometer, 10);
    _sensors->setResolution(flowThermometer, 10);
    _sensors->setResolution(returnThermometer, 10);

	set_inside_therm_address(insideThermometer);
	set_outside_therm_address(outsideThermometer);
	set_flow_therm_address(flowThermometer);
	set_return_therm_address(returnThermometer);
	set_test_therm_address(test_address);
}

void HEATMINDER::run_boiler(float flow_set_temp)
{
	uint32_t timer;
    digitalWrite(_SSR_pin, HIGH);
    Serial.print("Flow Temp = ");
    Serial.println(tempC_flow);
    timer = millis();
    delay(120000);
    while(tempC_flow < flow_set_temp)
    {
        read_temps();
    }
    Serial.print("Flow Temp = ");
    Serial.println(tempC_flow);
    Serial.print("Boiler ON (sec) = ");
    Serial.println((millis()-timer)/1000);
    digitalWrite(_SSR_pin, LOW);
    delay(60000);
}

float HEATMINDER::weather_compensation(void)
{
    //float y = (-3.65 * tempC_outside) + 93;
	float y = 40;
    return y;
}

void HEATMINDER::read_temps(void)
{
    _sensors->requestTemperatures();
    tempC_inside = _sensors->getTempC(insideThermometer);
    tempC_outside = _sensors->getTempC(outsideThermometer);
    tempC_flow = _sensors->getTempC(flowThermometer);
    tempC_return = _sensors->getTempC(returnThermometer);
    tempC_test = _sensors->getTempC(test_address);
}

void HEATMINDER::determine_state(ds1302_struct rtc, ramp_data* data, uint16_t data_index)
{
  //0 = before morning start
  //1 = morning ramp up
  //2 = morning hold
  //3 = after morning/before evening
  //4 = evening ramp up
  //5 = evening hold
  //6 = after evening
  
  uint8_t morning_ramp_begin_hour;
  uint8_t morning_ramp_begin_minute;
  uint8_t evening_ramp_begin_hour;
  uint8_t evening_ramp_begin_minute;
  
  int16_t temp_time;
  
  //calculate begin of ramp time
  temp_time = (morning_start_hour * 60) + morning_start_minute;
  temp_time = temp_time - data[data_index].estimated_ramp_period_minute;
  morning_ramp_begin_hour = temp_time / 60;
  morning_ramp_begin_minute = temp_time % 60;
  
  temp_time = (evening_start_hour * 60) + evening_start_minute;
  temp_time = temp_time - data[data_index].estimated_ramp_period_minute;
  evening_ramp_begin_hour = temp_time / 60;
  evening_ramp_begin_minute = temp_time % 60;
  
  //temp_time = (tm.Hour * 60) + tm.Minute;
  temp_time = (bcd2bin( rtc.h24.Hour10, rtc.h24.Hour) * 60) + bcd2bin( rtc.Minutes10, rtc.Minutes);
  
  if(init_state)//Run this first to determine state based on time
  {
      init_state = 0;
      if(temp_time > (evening_end_hour*60)+evening_end_minute)
      {
           time_state = 6; 
      }
      else if(temp_time > (evening_start_hour*60)+evening_start_minute)
      {
           time_state = 5; 
      }
      else if(temp_time > (evening_ramp_begin_hour*60)+evening_ramp_begin_minute)
      {
           time_state = 4; 
      }
      else if(temp_time > (morning_end_hour*60)+morning_end_minute)
      {
           time_state = 3; 
      }
      else if(temp_time > (morning_start_hour*60)+morning_start_minute)
      {
           time_state = 2; 
      }
      else if(temp_time > (morning_ramp_begin_hour*60)+morning_ramp_begin_minute)
      {
           time_state = 1; 
      }
      else
      {
           time_state = 0;
      }
  }
  else//Run this each loop to see if time based state change occurs
  {
      if((temp_time > (evening_end_hour*60)+evening_end_minute)&&(time_state == 5))
      {
           time_state = 6; 
      }
      else if((temp_time > (evening_ramp_begin_hour*60)+evening_ramp_begin_minute)&&(time_state == 3))
      {
           time_state = 4; 
      }
      else if((temp_time > (morning_end_hour*60)+morning_end_minute)&&(time_state == 2))
      {
           time_state = 3; 
      }
      else if((temp_time > (morning_ramp_begin_hour*60)+morning_ramp_begin_minute)&&(time_state == 0))
      {
           time_state = 1;
      }
      else if((temp_time < (morning_ramp_begin_hour*60)+morning_ramp_begin_minute)&&(time_state == 6))
      {
           time_state = 0;
      }
  }
}

void HEATMINDER::print_info(ds1302_struct time)
{
	char buffer[80];     // the code uses 70 characters.

	sprintf( buffer, "Time = %02d:%02d:%02d, ", \
	bcd2bin( time.h24.Hour10, time.h24.Hour), \
	bcd2bin( time.Minutes10, time.Minutes), \
	bcd2bin( time.Seconds10, time.Seconds));
	Serial.print(buffer);

	sprintf(buffer, "Date(day of month) = %d, Month = %d, " \
	"Day(day of week) = %d, Year = %d", \
	bcd2bin( time.Date10, time.Date), \
	bcd2bin( time.Month10, time.Month), \
	time.Day, \
	2000 + bcd2bin( time.Year10, time.Year));
	Serial.println( buffer);
	Serial.print("State = ");
	Serial.print(time_state);
	Serial.print("\n");

	Serial.print("Inside Temp = ");
	Serial.print(tempC_inside);
	Serial.print("\n");
	Serial.print("Outside Temp = ");
	Serial.print(tempC_outside);
	Serial.print("\n");
	Serial.print("Flow Temp = ");
	Serial.print(tempC_flow);
	Serial.print("\n");
	Serial.print("Return Temp = ");
	Serial.print(tempC_return);
	Serial.print("\n");
	Serial.print("Test Temp = ");
	Serial.print(tempC_test);
	Serial.print("\n");
}

uint8_t HEATMINDER::determine_estimated_time(ramp_data* data, uint16_t data_index)
{
	float temp_delta_sum = 0;
	float time_sum = 0;
	float rate;
	uint8_t estimated_time;

	for(uint16_t i=0;i<(data_index-1);i++)
	{
		temp_delta_sum = temp_delta_sum + (data[i].tempC_outside - data[i].tempC_inside);
		time_sum = time_sum + data[i].actual_ramp_period_minute;
	}
	rate = time_sum / temp_delta_sum;

	estimated_time = rate * (data[data_index].tempC_outside - data[data_index].tempC_inside);

	return estimated_time;
}

uint16_t HEATMINDER::ramp_up(DS1302 RTC, ramp_data* data, uint16_t data_index)
{
    ds1302_struct time;
    uint16_t time1, time2;
    RTC.clock_burst_read( (uint8_t *) &time);

	time1 = 60*(bcd2bin(time.h24.Hour10, time.h24.Hour))+bcd2bin(time.Minutes10, time.Minutes);

	data[data_index].tempC_inside = tempC_inside;
	data[data_index].tempC_outside = tempC_outside;
	data[data_index].estimated_ramp_period_minute = determine_estimated_time(data, data_index);
    //time1 = millis();
    while(tempC_inside < set_temp_evening_C)
    {
        run_boiler(weather_compensation());
        read_temps();
    }
    RTC.clock_burst_read( (uint8_t *) &time);
    time2 = 60*(bcd2bin(time.h24.Hour10, time.h24.Hour))+bcd2bin(time.Minutes10, time.Minutes);
    data[data_index].actual_ramp_period_minute = time2 - time1;
    data_index = data_index + 1;
    return data_index;
}

void HEATMINDER::hold_temp(void)
{
    if(tempC_inside < set_temp_evening_C - delta_temp)
    {
        run_boiler(weather_compensation());
    }
}

void HEATMINDER::anti_cond(void)
{
	if(tempC_inside < anti_cond_temp_C)
	{
		run_boiler(weather_compensation());
	}
}

void HEATMINDER::set_inside_therm_address(DeviceAddress address)
{
	for(uint8_t i = 0; i<8; i++)
	{
		insideThermometer[i] = address[i];
	}
}

void HEATMINDER::set_outside_therm_address(DeviceAddress address)
{
	for(uint8_t i = 0; i<8; i++)
	{
		outsideThermometer[i] = address[i];
	}
}

void HEATMINDER::set_flow_therm_address(DeviceAddress address)
{
	for(uint8_t i = 0; i<8; i++)
	{
		flowThermometer[i] = address[i];
	}
}

void HEATMINDER::set_return_therm_address(DeviceAddress address)
{
	for(uint8_t i = 0; i<8; i++)
	{
		returnThermometer[i] = address[i];
	}
}

void HEATMINDER::set_test_therm_address(DeviceAddress address)
{
	for(uint8_t i = 0; i<8; i++)
	{
		test_address[i] = address[i];
	}
}
