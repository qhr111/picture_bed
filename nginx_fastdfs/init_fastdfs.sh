#!/bin/bash
# 自动替换 FastDFS 配置文件中 tracker_server 的 IP

# 获取本容器在 Docker 网络中的 IP（推荐两种方式任选其一）
# 方式1：通过 hostname 获取（适用于大多数情况）
CONTAINER_IP=$(hostname -i | awk '{print $1}')

# 方式2：通过 eth0 接口获取（更精确）
# CONTAINER_IP=$(ip -4 addr show eth0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}')

# ---------- 获取 tc_server 容器的实际IP ----------
# 通过 Docker DNS 解析服务名（关键改动）
TC_SERVER_IP=$(getent hosts tc_server | awk '{print $1}')

# 配置文件路径
CONF_FILE="/etc/fdfs/mod_fastdfs.conf"

# 配置文件storage.conf路径
STORAGE_CONF_FILE="/etc/fdfs/storage.conf"

# 配置文件nginx.conf路径
NGINX_CONF_FILE="/usr/local/nginx/conf/nginx.conf"

# 执行替换操作（保留原端口号）
sed -i "s/^tracker_server=.*:\([0-9]\+\)$/tracker_server=${CONTAINER_IP}:\1/" "${CONF_FILE}"

# 执行替换操作（保留原端口号）
sed -i "s/^tracker_server =.*:\([0-9]\+\)$/tracker_server=${CONTAINER_IP}:\1/" "${STORAGE_CONF_FILE}"

# ---------- Nginx 配置替换（关键新增部分）----------
# 替换所有 proxy_pass 中的 tc_server 为实际IP（保留端口号）
sed -i "s/\btc_server\b/${TC_SERVER_IP}/g" "${NGINX_CONF_FILE}"


# 验证修改结果
echo "修改后的 tracker_server 配置："
grep "^tracker_server" "${CONF_FILE}"

echo -e "\n==== Nginx 代理后端IP ===="
grep "proxy_pass" "${NGINX_CONF_FILE}"

# 启动Tracker服务
/etc/init.d/fdfs_trackerd start

# 启动Storage服务
/etc/init.d/fdfs_storaged start

# 启动Nginx（必须前台运行）
/usr/local/nginx/sbin/nginx -g "daemon off;"

# 保持容器运行（备用命令）
# tail -f /dev/null