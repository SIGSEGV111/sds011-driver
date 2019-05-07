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
			if(sensor.Refresh(-1))
			{
				timeval tv_start;
				SYSERR(gettimeofday(&tv_start, NULL));
				const unsigned long long ns = tv_start.tv_sec * 1000000000ULL + tv_start.tv_usec * 1000ULL;

				// write in CSV compatible format to stdout
				printf("%f;%f;%llu\n", sensor.PM25(), sensor.PM10(), ns);
			}
			else
			{
				// huh? better check status...
				fprintf(stderr, "[WARN] received invalid data: %s\n", sensor.Status());
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
