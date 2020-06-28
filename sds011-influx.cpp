#include "sds011.hpp"
#include <sys/time.h>

int main(int argc, char* argv[])
{
	using namespace SDS011;

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s <tty-device>\n", argv[0]);
		return 1;
	}

	try
	{
		TSDS011 sensor(argv[1]);
		for(;;)
		{
			try
			{
				sensor.Refresh(-1);

				timeval tv_start;
				SYSERR(gettimeofday(&tv_start, NULL));
				const unsigned long long ns = tv_start.tv_sec * 1000000000ULL + tv_start.tv_usec * 1000ULL;

				// write in influx compatible format to stdout
				printf("pm2.5=%f,pm10=%f %llu\n", sensor.PM25(), sensor.PM10(), ns);

				// flush output to prevent caching
				fflush(stdout);
			}
			catch(const char* const msg)
			{
				fprintf(stderr, "[WARN] %s\n", msg);
			}
		}
	}
	catch(const char* const err)
	{
		perror(err);
		return 2;
	}
	catch(...)
	{
		perror("<unknown statement>");
		return 3;
	}

	return 4;
}
