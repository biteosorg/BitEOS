#!/bin/sh
cd /opt/besio/bin

if [ -f '/opt/besio/bin/data-dir/config.ini' ]; then
    echo
  else
    cp /config.ini /opt/besio/bin/data-dir
fi

if [ -d '/opt/besio/bin/data-dir/contracts' ]; then
    echo
  else
    cp -r /contracts /opt/besio/bin/data-dir
fi

while :; do
    case $1 in
        --config-dir=?*)
            CONFIG_DIR=${1#*=}
            ;;
        *)
            break
    esac
    shift
done

if [ ! "$CONFIG_DIR" ]; then
    CONFIG_DIR="--config-dir=/opt/besio/bin/data-dir"
else
    CONFIG_DIR=""
fi

exec /opt/besio/bin/nodbes $CONFIG_DIR "$@"
