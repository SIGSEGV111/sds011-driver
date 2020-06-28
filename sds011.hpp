#pragma once

#include <stdint.h>
#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s

#define SYSWARN(expr) ([&](){ const auto r = (expr); if( (long)(r) == -1L ) { perror("statement '" #expr "' in file '" __FILE__ "' at line " xstr(__LINE__)); }; return r; })()
#define SYSERR(expr)  ([&](){ const auto r = (expr); if( (long)(r) == -1L ) {  throw("statement '" #expr "' in file '" __FILE__ "' at line " xstr(__LINE__)); }; return r; })()

namespace SDS011
{
	enum class ECommandCode : uint8_t
	{
		GET_SET_REPORTING_MODE = 2,
		QUERY_DATA = 4,
		SET_DEVICE_ID = 5,
		GET_SET_SLEEP_WORK = 6,
		GET_SET_WORKING_PERIOD = 8,
		GET_FIRMWARE_VERSION = 7,
	};

	struct firmware_info_t
	{
		uint16_t year;
		uint8_t month;
		uint8_t day;
		uint16_t device_id;
	};

	class TSDS011
	{
		protected:
			const int fd;
			const uint16_t device_id;
			float pm10, pm25;


			// read a dataset from the sensor (buffer has to be 10bytes in size)
			bool ReadFromSensor(uint8_t* buffer, const int timeout);
			void ReadFromSensor(uint8_t* buffer) { if(!ReadFromSensor(buffer, -1)) throw "internal programming error"; }

			// send a command to the sensor
			void SendCommand(const ECommandCode cmdcode, const uint8_t* const payload, const uint8_t sz_payload);

		public:
			// synchronizes with the sensor
			// required to recover from communication errors
			void Sync();

			float PM25() const { return this->pm25; }	// PM2,5
			float PM10() const { return this->pm10; }	// PM10
			uint16_t DeviceID() const { return this->device_id; }	// Device ID

			// the FD used to communicate with the sensor
			// should only be used if you want to implement your own poll function
			int FD() const { return this->fd; }

			// check for new values from sensor
			// waits for the specified amount of milliseconds for an update from the sensor
			// if new data arrive within this time the function returns true
			// otherwise it returns false
			bool Refresh(const unsigned timeout);

			// like above, but waits forever
			void Refresh() { if(!Refresh(-1)) throw "internal programming error"; }

			// use this function to power down the sensor when you are not using it
			// this prolongs the life expectancy of the IR-LED in the sensor
			// true = ON; false = OFF
			void SetPowerState(const bool);
			bool GetPowerState();

			// "work 30 seconds and sleep n*60-30 seconds"
			void SetSampleInterval(const uint8_t n);

			// fetches firmware data, see struct firmware_info_t for details
			firmware_info_t GetFirmwareInfo();

			TSDS011(const char* const tty, const uint16_t device_id = 0xffff);
			TSDS011(const int fd_tty, const uint16_t device_id = 0xffff);
			~TSDS011();
			TSDS011(const TSDS011&) = delete;
	};
}
