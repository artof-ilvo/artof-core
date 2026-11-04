# Entries of the wrong type are moved aside, missing or empty ones get defaults.
publish 0.6.0
run_bootstrap
check_installed 0.6.0

rm -rf "$ILVO_PATH/implement"
echo "not a folder" > "$ILVO_PATH/implement"
rm -f "$ILVO_PATH/config.json"
mkdir "$ILVO_PATH/config.json"
touch "$ILVO_PATH/config.json/inside"
: > "$ILVO_PATH/settings.json"
rm -rf "${ILVO_PATH:?}/field/"*

log "Reinstalling over the broken settings"
reinstall
check "implement/ is a folder with the defaults" diff -r "$DEFAULTS/implement" "$ILVO_PATH/implement"
check "the implement file was moved aside" sh -c "ls -d $ILVO_PATH/implement.invalid-* | xargs test -f"
check "config.json is the default" cmp -s "$DEFAULTS/config.json" "$ILVO_PATH/config.json"
check "the config.json folder was moved aside" sh -c "ls -d $ILVO_PATH/config.json.invalid-* | xargs test -d"
check "empty settings.json got the default" cmp -s "$DEFAULTS/settings.json" "$ILVO_PATH/settings.json"
check "empty field/ got the defaults" diff -r "$DEFAULTS/field" "$ILVO_PATH/field"
check_healthy
