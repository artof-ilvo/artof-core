# A Redis container from another image is replaced by the pinned one.
publish 0.6.0
run_bootstrap
check_installed 0.6.0

docker rm -f "$REDIS_CONTAINER" >/dev/null
docker run -d --name "$REDIS_CONTAINER" -p 6379:6379 redis:7.2-alpine >/dev/null
check "Redis runs another image" not redis_image_pinned

reinstall
check_healthy
