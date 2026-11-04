# Without Docker Hub the update still succeeds and keeps the system image.
publish 0.6.0
run_bootstrap
check_installed 0.6.0
check_healthy

cp /etc/hosts "$WORK/hosts"
echo "127.0.0.1 registry-1.docker.io auth.docker.io index.docker.io production.cloudflare.docker.com" >> /etc/hosts
reinstall
cp "$WORK/hosts" /etc/hosts
check "the failed pull is reported" last_log_contains "could not pull $SYSTEM_IMAGE"
check "the system image is still there" docker image inspect "$SYSTEM_IMAGE"
check_healthy
