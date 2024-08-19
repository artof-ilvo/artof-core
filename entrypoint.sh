#!/bin/bash
set -e

# Copy files from the working directory to the volume
[ ! -f "${ILVO_PATH}/settings.json" ] && cp /app/files/settings.json $ILVO_PATH
[ ! -f "${ILVO_PATH}/config.json" ] && cp /app/files/config.json $ILVO_PATH
[ ! -f "${ILVO_PATH}/types.json" ] && cp /app/files/types.json $ILVO_PATH
[ ! -f "${ILVO_PATH}/redis.init.json" ] && cp /app/files/redis.init.json $ILVO_PATH
[ ! -d "${ILVO_PATH}/field" ] && cp -r /app/files/field $ILVO_PATH
[ ! -d "${ILVO_PATH}/implement" ] && cp -r /app/files/implement $ILVO_PATH

# Your existing startup command goes here
exec "$@"
