#pragma once

#include <stdint.h>
#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s

#define SYSWARN(expr) ([&](){ const auto r = (expr); if( (long)(r) == -1L ) { perror("statement '" #expr "' in file '" __FILE__ "' at line " xstr(__LINE__)); }; return r; })()
#define SYSERR(expr)  ([&](){ const auto r = (expr); if( (long)(r) == -1L ) {  throw("statement '" #expr "' in file '" __FILE__ "' at line " xstr(__LINE__)); }; return r; })()

namespace SDS011
{
	class TSDS011
	{
		protected:
			const int fd;
			float pm10, pm25;
			uint16_t id;
			const char* status;

			void Sync();

		public:
			float PM25() const { return this->pm25; }	// PM2,5
			float PM10() const { return this->pm10; }	// PM10
			uint16_t ID() const { return this->id; }
			const char* Status() const { return this->status; } // current sensor/driver status

			int FD() const { return this->fd; } // use it only for polling
			bool Refresh(const unsigned timeout = 0);	// check for new values from sensor (true means there was an update, false means no update available right now - buit check Status())

			TSDS011(const char* tty);
			TSDS011(const int fd_tty);
			~TSDS011();
			TSDS011(const TSDS011&) = delete;
	};
}
