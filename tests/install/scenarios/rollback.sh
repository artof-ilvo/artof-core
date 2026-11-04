# artof-update <version> goes back to an older release, and forward again.
publish 0.5.0 0.6.0
run_bootstrap
check_installed 0.6.0
check_healthy
mark_settings

log "Rolling back to 0.5.0"
run_logged artof-update 0.5.0
check_installed 0.5.0
check "backup is named after the version rolled back from" newest_backup_from 0.6.0
check "settings edit is kept" settings_marked
check_healthy

log "Updating to the newest version again"
run_logged artof-update
check_installed 0.6.0
check_healthy
