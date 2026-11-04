# Only the 10 most recent backups are kept.
publish 0.6.0
run_bootstrap
check_installed 0.6.0

log "Reinstalling 12 times"
for _ in $(seq 12); do
    apt-get install --reinstall -y -qq artof-core >"$WORK/last.log" 2>&1 || { cat "$WORK/last.log"; fail "reinstall failed"; }
    sleep 1  # backups are named per second
done
check "10 backups are kept" test "$(backup_count)" -eq 10
check_healthy
