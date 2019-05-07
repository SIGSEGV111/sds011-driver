.PHONY: all clean

all: sds011-csv sds011-influx

clean:
	rm -vf sds011-csv sds011-influx

sds011-csv: sds011-csv.cpp sds011.cpp
	g++ -Wall -Wextra -Werror -O2 -g -flto -fwhole-program sds011-csv.cpp sds011.cpp -o sds011-csv

sds011-influx:  sds011-influx.cpp sds011.cpp
	g++ -Wall -Wextra -Werror -O2 -g -flto -fwhole-program sds011-influx.cpp sds011.cpp -o sds011-influx
