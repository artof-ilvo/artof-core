#!/bin/sh
# Package the Docker setup (ilvo/) as a download that runs the given image.
# Usage: package-docker.sh <image> <output.tar.gz>
#   package-docker.sh axelwillekens/artof-core:dev artof-docker-dev.tar.gz
# The archive holds artof-docker/ with docker-compose.yml and the generated
# configuration in data/, without logs or Node-RED data.
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <image> <output.tar.gz>" >&2
    exit 1
fi
image=$1
output=$(realpath "$2")
root=$(cd "$(dirname "$0")/.." && pwd)

python3 "$root/scripts/sync-configs.py" --check

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
stage="$work/artof-docker"
mkdir -p "$stage/data/logs" "$stage/data/node-red"
for name in config.json settings.json types.json redis.init.json field implement; do
    cp -r "$root/ilvo/data/$name" "$stage/data/"
done
# Node-RED runs as uid 1000, which may not be the user who extracts the archive
chmod 777 "$stage/data/node-red"

sed "s|^\(    image: \)axelwillekens/artof-core:.*|\1$image\r|" "$root/ilvo/docker-compose.yml" > "$stage/docker-compose.yml"
grep -q "image: $image" "$stage/docker-compose.yml" || {
    echo "Could not set the artof image in docker-compose.yml" >&2
    exit 1
}

tar -czf "$output" -C "$work" --owner=0 --group=0 artof-docker
echo "Packaged $image as $output"
