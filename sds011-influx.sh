#!/bin/bash

set -e
set -u
set -o pipefail

exec 0</dev/null
P="$(dirname "$(readlink -f "$0")")"

set -x
source "/etc/location.conf"
set +x

echo "Country: $COUNTRY"
echo "City: $CITY"
echo "Building: $BUILDING"
echo "Floor: $FLOOR"
echo "Wing: $WING"
echo "Room: $ROOM"

(
	exec 1>/var/log/sds011-influx.log 2>&1
	"$P/sds011-influx" "$SDS011_TTY" | while read -r data; do
			data="environment,country=$COUNTRY,city=$CITY,building=$BUILDING,floor=$FLOOR,wing=$WING,room=$ROOM $data"
			curl --silent -XPOST "http://$SERVER/write?db=data" --data-binary "$data" || true
	done
) &
