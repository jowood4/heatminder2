#ifndef HEATMINDER_H
#define HEATMINDER_H

#include <Arduino.h>
#include <DS1302.h>
#include <OneWire.h>
#include <DallasTemperature.h>

struct ramp_data {
  uint8_t actual_ramp_period_minute;
  uint8_t estimated_ramp_period_minute;
  float tempC_inside;
  float tempC_outside;
};

class HEATMINDER {
 private:
        uint8_t _SSR_pin;
        
        //OneWire onewire;
        DallasTemperature* _sensors;
        
        //DeviceAddress insideThermometer = { 0x28, 0x86, 0xCD, 0x83, 0x03, 0x00, 0x00, 0x3D };
        DeviceAddress insideThermometer;
        DeviceAddress outsideThermometer;
        DeviceAddress flowThermometer;
        DeviceAddress returnThermometer;
        DeviceAddress test_address;
       
 public:
	HEATMINDER(uint8_t SSR_PIN);
	void run_boiler(float flow_set_temp);
	float weather_compensation(void);
	void read_temps(void);
	void determine_state(ds1302_struct rtc, ramp_data* data, uint16_t data_index);
	void set_inside_therm(DeviceAddress address);
	void set_outside_therm(DeviceAddress address);
	void set_flow_therm(DeviceAddress address);
	void set_return_therm(DeviceAddress address);
	void print_info(ds1302_struct time);
	uint8_t determine_estimated_time(ramp_data* data, uint16_t data_index);
	uint16_t ramp_up(DS1302 RTC, ramp_data* data, uint16_t data_index);
	void hold_temp(void);
	void anti_cond(void);
	void set_inside_therm_address(DeviceAddress address);
	void set_outside_therm_address(DeviceAddress address);
	void set_flow_therm_address(DeviceAddress address);
	void set_return_therm_address(DeviceAddress address);
	void set_test_therm_address(DeviceAddress address);
	void set_sensor(DallasTemperature* sens, DeviceAddress insideThermometer, \
			DeviceAddress outsideThermometer, DeviceAddress flowThermometer, \
			DeviceAddress returnThermometer, DeviceAddress test_address);

	uint8_t time_state;
	float tempC_inside;
	float tempC_outside;
	float tempC_flow;
	float tempC_return;
	float tempC_test;

	float set_temp_morning_C;
	float set_temp_evening_C;
	float anti_cond_temp_C;
	float delta_temp;

	uint8_t init_state;

    uint8_t morning_start_hour;
    uint8_t morning_start_minute;
    uint8_t morning_end_hour;
    uint8_t morning_end_minute;
    uint8_t evening_start_hour;
    uint8_t evening_start_minute;
    uint8_t evening_end_hour;
    uint8_t evening_end_minute;
};


#endif
