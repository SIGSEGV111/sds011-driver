.PHONY: all clean

all: sds011-csv sds011-influx

clean:
	rm -vf sds011-csv sds011-influx

sds011-csv: sds011-csv.cpp sds011.cpp
	g++ -Wall -Wextra -O3 -flto -march=native -fdata-sections -ffunction-sections -Wl,--gc-sections sds011-csv.cpp sds011.cpp -o sds011-csv

sds011-influx:  sds011-influx.cpp sds011.cpp
	g++ -Wall -Wextra -O3 -flto -march=native -fdata-sections -ffunction-sections -Wl,--gc-sections sds011-influx.cpp sds011.cpp -o sds011-influx
