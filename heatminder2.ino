#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS1302.h>
#include <heatminder.h>
//#include <Time.h>

DeviceAddress insideThermometer = { 0x28, 0x86, 0xCD, 0x83, 0x03, 0x00, 0x00, 0x3D };
DeviceAddress outsideThermometer = { 0x28, 0xCE, 0x0F, 0x84, 0x03, 0x00, 0x00, 0xCE };
DeviceAddress flowThermometer = { 0x28, 0xD9, 0xEC, 0x3E, 0x06, 0x00, 0x00, 0x3B };
DeviceAddress returnThermometer = { 0x28, 0xB7, 0xF8, 0x83, 0x03, 0x00, 0x00, 0xB1 };
DeviceAddress test_address = { 0x28, 0xCD, 0x8A, 0xAA, 0x05, 0x00, 0x00, 0x3A };

// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302 RTC(2, 3, 4);
// Optional connection for RTC module
const uint8_t DS1302_GND_PIN = 33;
const uint8_t DS1302_VCC_PIN = 35;
const uint8_t SSR_PIN = 5;
const uint8_t ONE_WIRE_BUS = 7;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

HEATMINDER hm_obj(SSR_PIN);
ramp_data data[50];
uint16_t data_index = 0;

void setup(void)
{
	hm_obj.set_sensor(&sensors, insideThermometer, outsideThermometer, \
			flowThermometer, returnThermometer, test_address);

    //seconds, minutes, hours, dayofweek, dayofmonth, month, year
    //RTC.set_time(0, 38, 18, 4, 1, 1, 2015);
    Serial.begin(9600);

    // Activate RTC module
    pinMode(DS1302_GND_PIN, OUTPUT);
    pinMode(DS1302_VCC_PIN, OUTPUT);

    digitalWrite(DS1302_GND_PIN, LOW);
    digitalWrite(DS1302_VCC_PIN, HIGH);

    data[0].estimated_ramp_period_minute = 60;

    //hard coded occupancy times, one cycle for morning and one for evening
    hm_obj.set_temp_morning_C = 25;
    hm_obj.set_temp_evening_C = 25;
    hm_obj.anti_cond_temp_C = 17;
    hm_obj.delta_temp = 2;
}

void loop(void)
{
    ds1302_struct rtc;
    RTC.clock_burst_read( (uint8_t *) &rtc);    

    hm_obj.determine_state(rtc, data, data_index);
    hm_obj.read_temps();
    hm_obj.print_info(rtc);

    //hm_obj.time_state = 7;

    switch (hm_obj.time_state) {
		case 0: //before morning start
				hm_obj.anti_cond();
				break;
		case 1: //morning ramp up
                data_index = hm_obj.ramp_up(RTC, data, data_index);
                hm_obj.time_state = 2;
                break;
        case 2: //morning hold
                hm_obj.hold_temp();
                break;
        case 3: //between morning and evening
        		hm_obj.anti_cond();
                break;
        case 4: //evening ramp up
        		data_index = hm_obj.ramp_up(RTC, data, data_index);
                hm_obj.time_state = 5;
                break;
        case 5: //evening hold
        		hm_obj.hold_temp();
                break;
        case 6: //after evening
        		hm_obj.anti_cond();
                break;
    }
    delay(5000);
}
