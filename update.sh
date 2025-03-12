#!/bin/bash

# Function to display usage information
usage() {
    echo "Usage: $0 [tag]"
    echo "  tag  Optional. Specify the Docker image tag and the corresponding .deb file version to download."
    echo "       Defaults to 'latest' if not provided."
    exit 1
}

# Validate number of arguments
if [ "$#" -gt 1 ]; then
    echo "Error: Invalid number of arguments."
    usage
fi

# Set tag to 'latest' by default if not provided as an argument
tag=${1:-latest}

# Set variables for paths and files
backup_dir="backup"
latest_backup_dir="$backup_dir/latest"
deb_file="artof-core-${tag}.deb"
ilvo_service="ilvo-robotics.service"
minio_url="https://minio.ilvo.be:9000/tv115-ilvo-robotics"

# Ensure ILVO_PATH is set
if [ -z "$ILVO_PATH" ]; then
    echo "Error: ILVO_PATH environment variable is not set."
    exit 1
fi

# Clean up any previous .deb files and download the new one
rm -f artof-core-*.deb
if ! wget -q "${minio_url}/${deb_file}"; then
    echo "Error: Failed to download the artof-core package from ${minio_url}/${deb_file}." >&2
    exit 1
fi

echo "Update ARTOF software"

# Stop the service
if ! sudo systemctl stop "$ilvo_service"; then
    echo "Error: Failed to stop $ilvo_service" >&2
    exit 1
fi

echo "Backing up original data"
mkdir -p "$backup_dir"
if ! zip -qr "$backup_dir/$(date +"%Y-%m-%d_%H:%M").bak.zip" "$ILVO_PATH"; then
    echo "Error: Failed to backup original data" >&2
    exit 1
fi

echo "Backing up configuration files"
mkdir -p "$latest_backup_dir"
sudo cp -r "$ILVO_PATH"/*.json "$latest_backup_dir/"

echo "Removing previous artof-core binaries"
if ! sudo dpkg -r artof-core >/dev/null 2>&1; then
    echo "Error: Failed to remove previous artof-core binaries" >&2
    exit 1
fi

echo "Installing new artof-core binaries"
if ! sudo dpkg -i "$deb_file" >/dev/null 2>&1; then
    echo "Error: Failed to install new artof-core binaries" >&2
    exit 1
fi

num_json_files=$(ls "$latest_backup_dir"/*.json 2>/dev/null | wc -l)
echo "Restoring $num_json_files original configuration files"
sudo cp -r "$latest_backup_dir"/*.json "$ILVO_PATH"

echo "Clearing Redis database"
if ! redis-cli FLUSHALL >/dev/null 2>&1; then
    echo "Error: Failed to clear Redis database" >&2
    exit 1
fi

echo "Updating system addon"
if ! docker pull "axelwillekens/artof-system:$tag" >/dev/null 2>&1; then
    echo "Error: Failed to pull the Docker image." >&2
    exit 1
fi

docker container stop system >/dev/null 2>&1
docker container rm system >/dev/null 2>&1

# Start the service in the background
sudo systemctl start "$ilvo_service" &

# Perform a 20-second countdown
for ((i=20; i>=0; i--)); do
    echo -ne "Starting $ilvo_service (takes $i sec) ...\r"
    sleep 1
done

# Check the service status after the countdown
if systemctl is-active --quiet "$ilvo_service"; then
    echo -e "\n$ilvo_service started successfully."
else
    echo -e "\nError: $ilvo_service failed to start."
fi

echo "Done"