# ARTOF core processes

This C++ project contains the processes running in the operation layer of the [ARTOF](https://artof-ilvo.github.io) project: GPS, navigation, implement control and the communication with the robot PLC.

- [Overview](#overview)
- [Install and update a robot](#install-and-update-a-robot)
- [Running a robot](#running-a-robot)
- [Configuration](#configuration)
- [Development](#development)
- [Releases and CI](#releases-and-ci)
- [Docker setup](#docker-setup)
- [Debug](#debug)
- [Licence](#licence)

## Overview

The processes exchange all their data through Redis; none of them talk to each other directly.

| Process | Role |
|---|---|
| `ilvo-system-manager` | Starts and monitors the other processes (heartbeat), manages the Docker add-ons (system web UI, Node-RED) and fills an empty Redis from `redis.init.json`. This is the process the `ilvo-robotics` service runs. |
| `ilvo-gps` | Reads the GPS receiver (Septentrio over serial, Stonex over Ethernet, or a socket) and keeps the NTRIP connection for RTK corrections. |
| `ilvo-navigation` | Follows the field path (pure pursuit) and computes the velocity commands. |
| `ilvo-operation` | Controls the hitches and implements along the task map (continuous, discrete and intermittent tasks). |
| `ilvo-robot-plc` | Exchanges control and monitor variables with the robot PLC (Siemens S7 or UDP). |
| `ilvo-simulation` | Simulates the robot when `pc.simulation.active` is set, so the stack runs without hardware. |

## Install and update a robot

Requirements: **Ubuntu 24.04, amd64**, connected to the internet. The packages are built against the Ubuntu 24.04 libraries and will not install on other releases.

Set up a new robot once:
```
curl -fsSL https://minio.ilvo.be:9000/tv115-ilvo-robotics/bootstrap.sh | sudo sh            # stable channel
curl -fsSL https://minio.ilvo.be:9000/tv115-ilvo-robotics/bootstrap.sh | sudo sh -s -- dev  # dev channel
```
The bootstrap installs Docker if needed, adds the signed ARTOF package repository and installs `artof-core`.

Update a robot:
```
artof-update            # newest version
artof-update --list     # available versions
artof-update <version>  # install a specific version, also to roll back
```
An update restarts the `ilvo-robotics` service and empties Redis, so do not update during a field job.

The package installs the `ilvo-robotics` service and a pinned Redis container, and sets `ILVO_PATH=/var/lib/ilvo`.
Redis is flushed on every upgrade so the system manager reloads `redis.init.json`.

Every install and update also takes care of the robot settings in `/var/lib/ilvo`:
1. **Backup** - before anything changes, `/var/lib/ilvo` (without `logs/`) is archived to `/var/lib/ilvo-backups/ilvo-<date>-<previous version>.tar.gz`. The 10 most recent backups are kept. If the backup fails, the update is stopped.
2. **Check** - every entry of the defaults in `/usr/share/artof-core/defaults` is checked:
   - a missing or empty file (`config.json`, `settings.json`, ...) gets the default file,
   - a missing or empty folder (`field/`, `implement/`) gets the default contents,
   - an entry of the wrong type (e.g. a file named `field`) is renamed to `<name>.invalid-<date>` and replaced by the default.

   Existing settings are never overwritten, and folders with content are left as they are. The install output lists every repair (`artof-core: ...`).

Restore a backup:
```
sudo systemctl stop ilvo-robotics
sudo rm -rf /var/lib/ilvo/{config.json,settings.json,types.json,redis.init.json,field,implement}
sudo tar -xzf /var/lib/ilvo-backups/<backup>.tar.gz -C /var/lib
redis-cli FLUSHALL
sudo systemctl start ilvo-robotics
```

### Uninstall

Each step goes further; stop after the one you need.

1. Remove the software. This stops and disables the service, but keeps the settings in `/var/lib/ilvo`, so a later `sudo apt install artof-core` continues where it left off (`apt purge` does the same).
```
sudo apt remove artof-core
```
2. Remove the Redis container and the addon containers started by the system manager (not all of them may exist). Add `docker image prune -a` to also free the image disk space.
```
docker rm -f redis-stack-server system node-red
```
3. Remove the robot data: settings, fields, implements and logs. This cannot be undone, so make a backup first.
```
sudo tar czf ~/ilvo-backup.tgz /var/lib/ilvo
sudo rm -rf /var/lib/ilvo /var/lib/ilvo-backups
```
4. Remove the ARTOF package repository.
```
sudo rm /etc/apt/sources.list.d/artof.sources /etc/apt/keyrings/artof.gpg
sudo apt update
```

Docker, `curl` and `gpg` installed by the bootstrap are left in place because other software may use them.

## Running a robot

```
systemctl status ilvo-robotics             # is the stack running?
sudo systemctl restart ilvo-robotics       # restart all processes
sudo systemctl stop ilvo-robotics          # stop all processes
journalctl -u ilvo-robotics -f             # service output
ls /var/lib/ilvo/logs                      # one log file per process
dpkg-query -W artof-core                   # installed version
```

Troubleshooting:
- **Service does not start**: it waits up to 60 s for Redis. Check `redis-cli ping` (expects `PONG`) and `docker ps` for the `redis-stack-server` container.
- **Changes to `redis.init.json` are not applied**: the file is only read into an empty Redis. Stop the service, run `redis-cli FLUSHALL` and start it again.
- **No PLC data**: check that the PLC answers on the IP in `config.json` (`ping 192.168.1.1` by default) and see [Debug](#debug).

## Configuration

All configuration lives in `$ILVO_PATH` (`/var/lib/ilvo` on a robot):

| File | Contents |
|---|---|
| `config.json` | Connections: the S7 PLC (IP, rack, slot, data blocks) and Redis, plus the mapping of PLC and PC variables to their types. |
| `types.json` | Structure of every variable type used in Redis (e.g. `drive_monitor`, `hitch_monitor`). |
| `settings.json` | The robot itself: dimensions, steering configuration, velocity and acceleration limits, navigation modes, hitches and the GPS device. |
| `redis.init.json` | Processes and Docker add-ons the system manager runs, and the initial values of the Redis variables. |
| `field/<name>/` | A field: `info.json` with its tasks, plus shapefiles for the geofence, the traject and each task. |
| `implement/<name>.json` | An implement: its types, sections and their position relative to the hitch. |
| `logs/` | Log files written by the processes. |

The defaults for a new robot are in `artof-core/usr/share/artof-core/defaults`. The `config-master` folder holds the configuration for the [Docker setup](#docker-setup) and mirrors `ilvo/data`.

## Development

### Build

Install the build dependencies and the S7 library once:
```
sudo apt-get install -y cmake g++ curl libcurl4-openssl-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev
sudo mkdir -p /usr/lib/ilvo
sudo cp dependencies/x86_64/shared/libsnap7.so /usr/lib/ilvo/
```

Debug build, with the address sanitizer enabled by default (prefix `cmake` with `NO_ASAN=1` to disable it):
```
mkdir -p build/debug && cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DINSTALL_FOLDER="$PWD/install" ../..
make -j8 install
```
The binaries end up in `build/debug/install`. Do not install a development build to `/usr/bin` on a robot: it overwrites the packaged binaries, and the next `artof-update` puts them back.

To run the processes locally, start Redis and point `ILVO_PATH` to a configuration folder whose `config.json` uses Redis on `127.0.0.1`, for example a copy of the package defaults:
```
docker run -d --name redis-stack-server -p 6379:6379 redis/redis-stack-server:7.4.0-v8
cp -r artof-core/usr/share/artof-core/defaults /tmp/ilvo && mkdir -p /tmp/ilvo/logs
export ILVO_PATH=/tmp/ilvo
build/debug/install/ilvo-system-manager
```

### Tests

Configure with `-DUTEST=1` to also build the unit tests. They read their input files from `TEST_ILVO_PATH`:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DUTEST=1 -DINSTALL_FOLDER="$PWD/install" ../..
make -j8
export TEST_ILVO_PATH=$(git rev-parse --show-toplevel)/src/UnitTest/files
./bin/test-shape   # one executable per test file, see src/UnitTest/CMakeLists.txt
```

### Build a Debian package locally

```
mkdir -p build/release && cd build/release
NO_ASAN=1 cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_FOLDER="$(git rev-parse --show-toplevel)/artof-core/usr/bin" ../..
make -j8 install
cd ../..
scripts/build-deb.sh artof-core "$(scripts/package-version.sh)" artof-core-local.deb
sudo apt install ./artof-core-local.deb   # on an Ubuntu 24.04 test machine
```

## Releases and CI

The workflows in `.github/workflows` build every pull request and publish on pushes:

| Trigger | Debian package | Docker image |
|---|---|---|
| Pull request | built, not published | built, not pushed |
| Push to `main` | `dev` APT channel | `:dev` |
| Tag `vX.Y.Z` | `stable` APT channel | `:latest` and `:vX.Y.Z` |

Every push to `main` publishes a new `dev` package, also for documentation-only changes.
Publishing jobs wait for approval on the `staging` (dev) and `production` (stable) environments.

Release a new version:
```
git tag v1.2.0
git push --tags
```

Package versions come from git (`scripts/package-version.sh`): a tag gives `1.2.0`, later commits give `1.2.0+3.g1a2b3c4`.

The APT repository lives in the MinIO bucket `tv115-ilvo-robotics` under `apt/` and is published by `scripts/publish-apt.sh` with the AWS CLI. CI needs these secrets:

| Secret | Purpose |
|---|---|
| `MINIO_ENDPOINT`, `MINIO_ACCESS_KEY`, `MINIO_SECRET_KEY` | Write access to the MinIO bucket. |
| `APT_SIGNING_KEY` | Armored private GPG key that signs the repository. |
| `DOCKER_HUB_USERNAME`, `DOCKER_HUB_TOKEN` | Pushing the Docker image. |

Keep a backup of the APT signing key outside GitHub. If it is lost, a new key must be created and every robot must run the bootstrap again to trust it.

## Docker setup

`ilvo/` runs the whole stack in Docker, without installing anything on the host:
```
cd ilvo
docker compose up -d
```
`ilvo/data` is mounted as `ILVO_PATH`. It contains the same files as the package defaults, with these differences:
- `config.json`: Redis is reached at `"ip": "redis"`, the compose service name, instead of `127.0.0.1`.
- `redis.init.json`: the add-on containers mount the host data folder in `HostConfig.Binds`, join the compose network through `HostConfig.NetworkMode` (`ilvo_artof_network`) and publish their ports through `ExposedPorts` and `HostConfig.PortBindings`.

When the package defaults change, copy the fields, implements, `settings.json` and `types.json` to `ilvo/data`:
```
cp -r artof-core/usr/share/artof-core/defaults/{field,implement,settings.json,types.json} ilvo/data/
```

## Debug

### Robot PLC

Write and read UDP messages from the terminal:
```
echo "Hello UDP3" | nc -u 127.0.0.1 5001   # write
nc -ul 5001                                 # read
```

Show the raw bytes exchanged with the PLC:
```
ncat -ulk 5001 | hexdump -C   # bytes read from the PLC
ncat -ulk 5000 | hexdump -C   # bytes written to the PLC
```

## Licence

This project is under the ``ILVO LICENCE``.

```
Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

When the Software, including modifications and extensions, is used for:
- commercial or non-commercial machinery: the ILVO logo has to be clearly
   visible on the machine or on any promotion material which may be used in any
   agricultural fair or conference, in a way it is clear that ILVO contributed
   to the development of the software for the machine.
- a scientific or vulgarising publication: a reference to ILVO must be made as
   well as to the website of the living lab Agrifood Technology of ILVO:
   https://www.agrifoodtechnology.be

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```