#include "sds011.hpp"
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

static volatile bool do_run = true;

static void OnSignal(int)
{
	do_run = false;
}

int main(int argc, char* argv[])
{
	using namespace SDS011;

	signal(SIGINT,  &OnSignal);
	signal(SIGTERM, &OnSignal);
	signal(SIGHUP,  &OnSignal);
	signal(SIGQUIT, &OnSignal);

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s <tty-device>\n", argv[0]);
		return 1;
	}

	try
	{
		TSDS011 sensor(argv[1]);

		// take a sample every 10min, sensor will be powered down for most of the time in between
		sensor.SetSampleInterval(10);

		while(do_run)
		{
			try
			{
				// take one sample
				sensor.Refresh();

				// get timestamp
				timeval tv_start;
				SYSERR(gettimeofday(&tv_start, NULL));
				const unsigned long long ns = tv_start.tv_sec * 1000000000ULL + tv_start.tv_usec * 1000ULL;

				// write in CSV compatible format to stdout
				printf("%f;%f;%llu\n", sensor.PM25(), sensor.PM10(), ns);
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
