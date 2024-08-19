#!/bin/bash

# Function to display usage information
usage() {
    echo "Usage: $0"
    echo
    echo "This script installs dependencies for the ARTOF software, sets up a Redis Docker container,"
    echo "creates and enables the ilvo-robotics service, and configures the environment."
    echo
    echo "No arguments are required."
    exit 1
}

# Function to handle errors
handle_error() {
    echo "Error: $1" >&2
    exit 1
}

# Check for incorrect usage
if [ "$#" -ne 0 ]; then
    usage
fi

echo "Installing Dependencies"
sudo apt-get update || handle_error "Failed to update package list"
sudo apt-get install -y cmake curl libcurl4-openssl-dev iputils-ping \
    libboost-filesystem-dev libboost-system-dev libboost-thread-dev || handle_error "Failed to install dependencies"

echo "Starting Redis stack in Docker"
docker run -d --restart always --name redis-stack-server -p 6379:6379 redis/redis-stack-server:latest || handle_error "Failed to start Redis stack"

echo "Creating ilvo-robotics service"
sudo tee /etc/systemd/system/ilvo-robotics.service > /dev/null <<EOF
[Unit]
Description=ilvo-robotics framework
Wants=network-online.target
After=network.target network-online.target

[Service]
Environment="ILVO_PATH=/var/lib/ilvo"
ExecStartPre=/bin/sleep 20
ExecStart=ilvo-system-manager
Restart=on-failure
StartLimitBurst=5
StartLimitIntervalSec=30

[Install]
WantedBy=multi-user.target
EOF

echo "Reloading systemd and enabling ilvo-robotics service"
sudo systemctl daemon-reload || handle_error "Failed to reload systemd daemon"
sudo systemctl enable ilvo-robotics.service || handle_error "Failed to enable ilvo-robotics service"

echo "Setting environment variable in ~/.bashrc"
if ! grep -q "ILVO_PATH=/var/lib/ilvo" ~/.bashrc; then
    echo "export ILVO_PATH=/var/lib/ilvo" >> ~/.bashrc || handle_error "Failed to update ~/.bashrc"
fi

echo "Done"
