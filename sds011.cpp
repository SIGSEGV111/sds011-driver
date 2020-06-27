#include "sds011.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

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
		while(read(this->fd, buffer, sizeof(buffer)) > 0);
	}

	bool TSDS011::Refresh(const unsigned timeout)
	{
		pollfd pfd;
		pfd.fd = this->fd;
		pfd.events = POLLIN|POLLPRI;
		pfd.revents = 0;
		SYSERR(poll(&pfd, 1, timeout));
		if(pfd.revents != POLLIN)
		{
			this->status = "timeout waiting for data from sensor";
			return false;
		}
		usleep(10000);	// poll triggers once the first byte has been received, but we need the complete record not just the first byte. In theory the transfer of the remaining 9bytes takes 9.375ms at 9600baud (mind the start- and stop-bits!), but we wait for 10ms to be sure

		uint8_t buffer[20];
		const long n = SYSERR(read(fd, buffer, sizeof(buffer)));

		if(n >= 10 && buffer[0] == 0xaa && buffer[1] == 0xc0 && buffer[9] == 0xab)
		{
			const uint16_t pm25 = buffer[2] + (buffer[3] << 8);
			const uint16_t pm10 = buffer[4] + (buffer[5] << 8);
			const uint16_t id   = buffer[6] + (buffer[7] << 8);

			const uint8_t expected_checksum = buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6] + buffer[7];
			if(expected_checksum != buffer[8])
			{
				this->status = "checksum error";
				return false;
			}

			this->pm25 = (float)pm25 / 10.0f;
			this->pm10 = (float)pm10 / 10.0f;
			this->id = id;

			this->status = "OK";
			return true;
		}
		else
		{
			this->Sync();
			this->status = "synchronisation error";
			return false;
		}
	}

	TSDS011::TSDS011(const char* tty) : fd(SYSERR(open(tty, O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK))), pm10(-1.0f), pm25(-1.0f), id(0), status("init")
	{
		this->Sync();
	}

	TSDS011::TSDS011(const int fd_tty) : fd(fd_tty), pm10(-1.0f), pm25(-1.0f), id(0), status("init")
	{
		const int flags = SYSERR(fcntl(fd, F_GETFD));
		SYSERR(fcntl(fd, flags | O_NONBLOCK | O_CLOEXEC));
		this->Sync();
	}

	TSDS011::~TSDS011()
	{
		SYSWARN(close(this->fd));
	}
}
