#include "sds011.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <endian.h>

// static void Hexdump(const void* const buffer, const unsigned size)
// {
// 	for(unsigned i = 0; i < size; i++)
// 		fprintf(stderr, "%02hhx ", ((const char*)buffer)[i]);
// 	fputc('\n', stderr);
// }

namespace SDS011
{
	void TSDS011::Sync()
	{
		// set terminal options
		struct termios tios;
		memset(&tios, 0, sizeof(tios));
		tios.c_iflag = IGNBRK;
		tios.c_oflag = 0;
		tios.c_cflag = CREAD | CLOCAL | B9600 | CS8;
		tios.c_lflag = 0;
		if(tcsetattr(this->fd, TCSANOW, &tios) != 0) throw "tcsetattr() failed";

		// read all data there is
		uint8_t buffer[128];
		while(read(this->fd, buffer, sizeof(buffer)) > 0)
		{
			// wait 10ms for data, on timeout we can be sure we are in-sync with the sensor
			pollfd pfd;
			pfd.fd = this->fd;
			pfd.events = POLLIN|POLLPRI;
			pfd.revents = 0;
			if(SYSERR(poll(&pfd, 1, 10)) == 0)
				break;	// no data in buffer and no data received for 10ms => in-sync with sensor
		}

// 		SetPowerState(true);
//
// 		const firmware_info_t info = GetFirmwareInfo();
//
// 		fprintf(stderr, "[INFO] synchronized with sensor\n[INFO] firmware date: %04hu-%02hhu-%02hhu\n[INFO] Device ID: %04hx\n", info.year, info.month, info.day, info.device_id);
//
//  	SetPowerState(false);
	}

	static uint8_t ComputeChecksum(const uint8_t* payload, const uint8_t sz_payload)
	{
		uint8_t checksum = 0;
		for(uint8_t i = 0; i < sz_payload; i++)
			checksum += payload[i];
		return checksum;
	}

	bool TSDS011::ReadFromSensor(uint8_t* buffer, const int timeout)
	{
		for(;;)
		{
			pollfd pfd;
			pfd.fd = this->fd;
			pfd.events = POLLIN;
			pfd.revents = 0;

			if(SYSERR(poll(&pfd, 1, timeout)) == 0)
				return false;	// timeout

			if(pfd.revents != POLLIN)
				throw "poll() logic error";

			usleep(10000);	// poll triggers once the first byte has been received, but we need the complete record not just the first byte. In theory the transfer of the remaining 9bytes takes 9.375ms at 9600baud (mind the start- and stop-bits!), but we wait for 10ms to be sure

			if(SYSERR(read(fd, buffer, 10)) != 10)
				throw "received incomplete dataset from sensor";

			if(buffer[0] != 0xaa || (buffer[1] != 0xc5 && buffer[1] != 0xc0) || buffer[9] != 0xab)
				throw "recveived garbage from sensor";

			const uint8_t expected_checksum = ComputeChecksum(buffer + 2, 6);
			if(expected_checksum != buffer[8])
				throw "checksum mismatch";

			// check if this is a response from the sensor we are working with, or from some other sensor on the shared bus
			if(this->device_id == 0xffff || memcmp(buffer + 6, &this->device_id, 2) == 0)
				break;

			// some other sensor sent some data on the shared bus => ignore and start over
		}

		return true;
	}

	bool TSDS011::Refresh(const unsigned timeout)
	{
		uint8_t buffer[10];
		if(!ReadFromSensor(buffer, timeout))
			return false;

		const uint16_t pm25 = buffer[2] + (buffer[3] << 8);
		const uint16_t pm10 = buffer[4] + (buffer[5] << 8);

		this->pm25 = (float)pm25 / 10.0f;
		this->pm10 = (float)pm10 / 10.0f;

		return true;
	}

	firmware_info_t TSDS011::GetFirmwareInfo()
	{
		SendCommand(ECommandCode::GET_FIRMWARE_VERSION, nullptr, 0);

		uint8_t buffer[10];
		if(!ReadFromSensor(buffer, 1000))
			throw "timeout while waiting for sensor response";

		if(buffer[2] != 0x07)
			throw "sensor responded with an unexpected command code";

		firmware_info_t info;

		info.year = 2000 + buffer[3];
		info.month = buffer[4];
		info.day = buffer[5];
		memcpy(&info.device_id, buffer + 6, 2);

		return info;
	}

	void TSDS011::SendCommand(const ECommandCode cmdcode, const uint8_t* const payload, const uint8_t sz_payload)
	{
		if(sz_payload > 12)
			throw "payload can be at most 12bytes";

		uint8_t buffer[19];
		memset(buffer, 0, sizeof(buffer));

		buffer[0] = 0xaa;	// header
		buffer[1] = 0xb4;	// wtf? (0xb4 probably means master => sensor.... or something...)
		buffer[2] = (uint8_t)cmdcode;
		memcpy(buffer + 3, payload, sz_payload);
		memcpy(buffer + 15, &this->device_id, 2);
		buffer[17] = ComputeChecksum(buffer + 2, 15);
		buffer[18] = 0xab;

		if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer))
			throw "write command to sensor failed";
	}

	void TSDS011::SetPowerState(const bool target_state)
	{
		uint8_t payload[2] = { 0x01, (uint8_t)(target_state ? 0x01 : 0x00) };
		SendCommand(ECommandCode::GET_SET_SLEEP_WORK, payload, sizeof(payload));

		uint8_t buffer[10];
		if(!ReadFromSensor(buffer, 1000))
			throw "timeout while waiting for sensor response";

		if(buffer[2] != 0x06)
			throw "sensor responded with an unexpected command code";

		if(buffer[4] != (uint8_t)(target_state ? 0x01 : 0x00))
			throw "sensor refused to enter requested power state";
	}

	bool TSDS011::GetPowerState()
	{
		uint8_t payload[1] = { 0x00 };
		SendCommand(ECommandCode::GET_SET_SLEEP_WORK, payload, sizeof(payload));

		uint8_t buffer[10];
		if(!ReadFromSensor(buffer, 1000))
			throw "timeout while waiting for sensor response";

		if(buffer[2] != 0x06)
			throw "sensor responded with an unexpected command code";

		if(buffer[4] > 1)
			throw "sensor reported unknown power state";

		return buffer[4] == 1;
	}

	void TSDS011::SetSampleInterval(const uint8_t n)
	{
		uint8_t payload[2] = { 0x01, n };
		SendCommand(ECommandCode::GET_SET_WORKING_PERIOD, payload, sizeof(payload));

		uint8_t buffer[10];
		if(!ReadFromSensor(buffer, 1000))
			throw "timeout while waiting for sensor response";

		if(buffer[2] != 0x08)
			throw "sensor responded with an unexpected command code";

		if(buffer[3] != 1 || buffer[4] != n)
			throw "sensor refused to set sample interval";
	}

	TSDS011::TSDS011(const char* const tty, const uint16_t device_id) : fd(SYSERR(open(tty, O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK))), device_id(htole16(device_id)), pm10(-1.0f), pm25(-1.0f)
	{
		this->Sync();
	}

	TSDS011::TSDS011(const int fd_tty, const uint16_t device_id) : fd(fd_tty), device_id(htole16(device_id)), pm10(-1.0f), pm25(-1.0f)
	{
		const int flags = SYSERR(fcntl(fd, F_GETFD));
		SYSERR(fcntl(fd, flags | O_NONBLOCK | O_CLOEXEC));
		this->Sync();
	}

	TSDS011::~TSDS011()
	{
// 		try { this->SetPowerState(false); }
// 		catch(...) { fprintf(stderr, "[WARN] failed to power down sensor\n"); }
		SYSWARN(close(this->fd));
	}
}
