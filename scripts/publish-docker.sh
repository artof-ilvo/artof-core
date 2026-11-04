#!/bin/sh
# Upload the Docker setup to MinIO as docker/artof-docker-<name>.tar.gz.
# Usage: publish-docker.sh <name> <image>
#   publish-docker.sh dev axelwillekens/artof-core:dev
#   publish-docker.sh v1.2.0 axelwillekens/artof-core:v1.2.0
# Expects AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY and MINIO_ENDPOINT in the
# environment.
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <name> <image>" >&2
    exit 1
fi
name=$1
image=$2
remote=s3://tv115-ilvo-robotics/docker

# MinIO rejects the checksum headers newer AWS CLI versions send by default
export AWS_DEFAULT_REGION="${AWS_DEFAULT_REGION:-us-east-1}"
export AWS_REQUEST_CHECKSUM_CALCULATION=when_required
export AWS_RESPONSE_CHECKSUM_VALIDATION=when_required

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
archive="$work/artof-docker-$name.tar.gz"
"$(dirname "$0")/package-docker.sh" "$image" "$archive"
aws --endpoint-url "$MINIO_ENDPOINT" s3 --only-show-errors cp "$archive" "$remote/artof-docker-$name.tar.gz"
echo "Published $remote/artof-docker-$name.tar.gz"
