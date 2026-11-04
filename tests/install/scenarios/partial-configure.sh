# If Redis does not come up the package stays half-configured, and finishing
# the configuration later recovers it.
publish 0.6.0
run_bootstrap
check_installed 0.6.0

# Right image, but Redis never listens
docker rm -f "$REDIS_CONTAINER" >/dev/null
docker create --name "$REDIS_CONTAINER" --entrypoint sleep "$REDIS_IMAGE" infinity >/dev/null

log "Reinstalling while Redis is down (takes 30 s)"
if apt-get install --reinstall -y artof-core >"$WORK/last.log" 2>&1; then
    cat "$WORK/last.log"
    fail "the install should fail when Redis does not come up"
fi
cat "$WORK/last.log"
check "the failure names Redis" last_log_contains "Redis did not come up"
check "package is half-configured" test "$(package_status)" = "install ok half-configured"

log "Recovering"
docker rm -f "$REDIS_CONTAINER" >/dev/null
run_logged dpkg --configure -a
check_installed 0.6.0
check_healthy
