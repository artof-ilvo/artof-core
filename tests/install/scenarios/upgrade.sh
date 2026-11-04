# A normal update: settings kept, backup made, types.json and Redis refreshed.
publish 0.5.0
run_bootstrap
check_installed 0.5.0
check_healthy

mark_settings
echo "old log line" > "$ILVO_PATH/logs/install-test.log"
echo '{}' > "$ILVO_PATH/types.json"
redis-cli SET install_test_marker 1 >/dev/null

log "Updating to 0.6.0"
publish 0.5.0 0.6.0
run_logged artof-update
check_installed 0.6.0
check "settings edit is kept" settings_marked
check "one backup was made" test "$(backup_count)" -eq 1
check "backup is named after the previous version" newest_backup_from 0.5.0
check "backup holds the edited settings" newest_backup_marked
check "backup leaves out logs" not newest_backup_has_logs
check "types.json follows the release" cmp -s "$DEFAULTS/types.json" "$ILVO_PATH/types.json"
check "types.json replacement is reported" last_log_contains "replaced $ILVO_PATH/types.json"
check "Redis was flushed" test "$(redis-cli EXISTS install_test_marker)" = 0
check_healthy
