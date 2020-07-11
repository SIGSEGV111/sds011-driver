#include "sds011.hpp"
#include <sys/time.h>
#include <signal.h>
#include <sys/file.h>
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

	if(argc != 3)
	{
		fprintf(stderr, "usage: %s <tty-device> <location>\n", argv[0]);
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
				sensor.Refresh(11 * 60 * 1000);

				// get timestamp
				timeval tv_start;
				SYSERR(gettimeofday(&tv_start, NULL));

				SYSERR(flock(STDOUT_FILENO, LOCK_EX));
				printf("%ld.%06ld;%ld.%06ld;\"%s\";\"pm10\";%f\n", tv_start.tv_sec, tv_start.tv_usec, tv_start.tv_sec, tv_start.tv_usec, argv[2], sensor.PM10());
				printf("%ld.%06ld;%ld.%06ld;\"%s\";\"pm25\";%f\n", tv_start.tv_sec, tv_start.tv_usec, tv_start.tv_sec, tv_start.tv_usec, argv[2], sensor.PM25());
				fflush(stdout);
				SYSERR(flock(STDOUT_FILENO, LOCK_UN));
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
