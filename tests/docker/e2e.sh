#!/bin/sh
# End-to-end test of the Docker setup: build the image, start the compose stack
# of ilvo/ on a copy of it and check the processes and the add-ons.
#   ARTOF_DOCKER_TEST=1 tests/docker/e2e.sh
# This uses the ports 6379, 8080 and 1880 and the container names system and
# node-red, so run it on a CI runner or a machine without the stack.
set -eu

if [ "${ARTOF_DOCKER_TEST:-}" != 1 ]; then
    echo "This starts containers on fixed names and ports. Set ARTOF_DOCKER_TEST=1." >&2
    exit 1
fi

SRC=$(cd "$(dirname "$0")/../.." && pwd)
IMAGE=axelwillekens/artof-core:latest   # the image docker-compose.yml runs
WORK=$(mktemp -d)
STACK=$WORK/ilvo                        # compose project "ilvo", as in the add-on network name

log() { printf '\n==> %s\n' "$*"; }
fail() { echo "FAIL: $*" >&2; exit 1; }
not() { ! "$@"; }
check() {
    desc=$1; shift
    if "$@"; then echo "ok: $desc"; else fail "$desc"; fi
}
wait_until() {
    timeout=$1; shift
    for _ in $(seq "$timeout"); do
        "$@" >/dev/null 2>&1 && return 0
        sleep 1
    done
    return 1
}

compose() { docker compose --project-directory "$STACK" -f "$STACK/docker-compose.yml" "$@"; }
redis() { compose exec -T redis redis-cli "$@"; }
container_running() { [ "$(docker inspect -f '{{.State.Running}}' "$1" 2>/dev/null)" = true ]; }
artof_running() { container_running "$(compose ps -q artof)"; }
on_network() { docker inspect -f '{{range $n, $_ := .NetworkSettings.Networks}}{{$n}} {{end}}' "$1" | grep -qw "$2"; }
redis_initialized() { [ -n "$(redis JSON.GET system)" ]; }
# Host folders bound into a container
bind_sources() { docker inspect -f '{{range .Mounts}}{{.Source}} {{end}}' "$1"; }
binds_from_stack() {
    for source in $(bind_sources "$1"); do
        case "$source" in "$STACK/data"|"$STACK/data/"*) ;; *) return 1 ;; esac
    done
}
http_ok() { curl -fsS -o /dev/null --max-time 5 "$1"; }

# Command lines of the processes in the artof container
artof_processes() {
    compose exec -T artof sh -c 'for f in /proc/[0-9]*/cmdline; do tr "\000" " " < "$f"; echo; done' 2>/dev/null
}
process_running() { artof_processes | grep -q "$1"; }
all_processes_running() {
    for p in $PROCESSES; do process_running "$p" || return 1; done
}

# ilvo-operation publishes the implements of the loaded field (flame-weeder for
# the example field) every tick
implement_states() { redis JSON.GET implement.states; }
# parse_states <python check>: run the check on the parsed implement.states
parse_states() {
    implement_states | python3 -c "
import json, sys
states = json.load(sys.stdin)
if isinstance(states, str):  # redis-cli quotes the value on a terminal
    states = json.loads(states)
$1"
}
implements_published() { parse_states 'sys.exit(not states)'; }
# NaN coordinates are written as null
implement_coordinates_valid() {
    parse_states '
for implement in states.values():
    for section in implement["sections"]:
        for point in section["xy"] + section["latlng"]:
            if None in point:
                sys.exit(1)'
}

# Values from the generated configuration
read_init() {
    python3 - "$STACK/data/redis.init.json" "$1" <<'EOF'
import json, sys
system = json.load(open(sys.argv[1]))["system"]
if sys.argv[2] == "processes":
    print(" ".join(p["Name"] for p in system["ilvoProcesses"] if p.get("Running")))
elif sys.argv[2] == "addons":
    print(" ".join(a["Name"] for a in system["ilvoAddons"] if a.get("Running")))
elif sys.argv[2] == "network":
    print(system["ilvoAddons"][0]["DockerConfig"]["HostConfig"]["NetworkMode"])
EOF
}

cleanup() {
    status=$?
    if [ "$status" -ne 0 ]; then
        echo "---- diagnostics ----" >&2
        compose ps -a >&2 || true
        compose logs --tail 80 >&2 || true
        echo "implement.states: $(implement_states 2>/dev/null)" >&2
        for addon in ${ADDONS:-}; do docker logs --tail 40 "$addon" >&2 || true; done
        tail -n 20 "$STACK"/data/logs/* >&2 || true
    fi
    compose down -v --remove-orphans >/dev/null 2>&1 || true
    for addon in ${ADDONS:-}; do docker rm -f "$addon" >/dev/null 2>&1 || true; done
    rm -rf "$WORK"
}
trap cleanup EXIT

log "Building $IMAGE"
docker build -q -t "$IMAGE" "$SRC" >/dev/null
for binary in ilvo-system-manager ilvo-gps ilvo-navigation ilvo-operation ilvo-robot-plc ilvo-simulation; do
    check "image contains $binary" docker run --rm "$IMAGE" sh -c "command -v $binary"
done
check "shared libraries resolve" not sh -c "docker run --rm '$IMAGE' sh -c 'ldd /usr/bin/ilvo-*' | grep -q 'not found'"

log "Preparing the stack in $STACK"
cp -r "$SRC/ilvo" "$STACK"
rm -rf "$STACK/data/logs" "$STACK/data/node-red"
mkdir -p "$STACK/data/logs" "$STACK/data/node-red"
chmod 777 "$STACK/data/node-red"   # node-red runs as uid 1000
PROCESSES=$(read_init processes)
ADDONS=$(read_init addons)
NETWORK=$(read_init network)
for addon in $ADDONS; do
    docker container inspect "$addon" >/dev/null 2>&1 && fail "a container named $addon already exists"
done

log "Starting the stack"
compose pull -q redis
compose up -d --pull never
check "artof container is running" wait_until 30 artof_running
check "Redis answers" wait_until 30 redis ping
check "system manager initialized Redis" wait_until 60 redis_initialized
for p in $PROCESSES; do
    check "$p is running" wait_until 60 process_running "$p"
done
for addon in $ADDONS; do
    check "add-on $addon is running" wait_until 180 container_running "$addon"
    check "add-on $addon is on $NETWORK" on_network "$addon" "$NETWORK"
    check "add-on $addon binds from the stack's data folder" binds_from_stack "$addon"
done
check "system add-on sees the settings" docker exec system test -s /var/lib/ilvo/settings.json
check "node-red add-on sees the settings" docker exec node-red test -s /var/lib/ilvo/settings.json
check "system web UI answers on 8080" wait_until 60 http_ok http://localhost:8080/
check "node-red answers on 1880" wait_until 60 http_ok http://localhost:1880/

log "Checking the stack stays up"
sleep 20
check "artof container is still running" artof_running
check "all processes are still running" all_processes_running
check "ilvo-operation publishes the implements" wait_until 60 implements_published
check "implement coordinates are numbers" implement_coordinates_valid
check "processes write logs" sh -c "ls '$STACK/data/logs' | grep -q ."

log "Restarting the artof container"
redis SET docker_test_marker 1 >/dev/null
compose restart artof
check "artof container is running again" wait_until 30 artof_running
check "Redis kept its data" test "$(redis EXISTS docker_test_marker)" = 1
check "all processes are running again" wait_until 60 all_processes_running
for addon in $ADDONS; do
    check "add-on $addon is running after the restart" wait_until 60 container_running "$addon"
done

log "Docker setup test passed"
