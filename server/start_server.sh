#!/bin/bash
# 启动 redis
redis-server --daemonize yes
# 启动你的服务
./tc_http_server