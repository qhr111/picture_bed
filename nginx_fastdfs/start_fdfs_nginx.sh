#!/bin/bash

# 启动 fastdfs（按你的配置）
/usr/bin/fdfs_trackerd /etc/fdfs/tracker.conf start
/usr/bin/fdfs_storaged /etc/fdfs/storage.conf start

lsof -i:22122
lsof -i:23000
# 等待 fastdfs 启动完成
/usr/bin/fdfs_monitor /etc/fdfs/storage.conf

# 启动 nginx
/usr/local/nginx/sbin/nginx -g "daemon off;"