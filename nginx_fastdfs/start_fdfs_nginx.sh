#!/bin/bash

# 启动 fastdfs（按你的配置）

echo "启动tracker"
/usr/bin/fdfs_trackerd /etc/fdfs/tracker.conf start

sleep 5
/usr/bin/fdfs_storaged /etc/fdfs/storage.conf start

echo "查看tracker是否启动:"
lsof -i:22122
echo "查看storage是否启动:"
lsof -i:23000
echo "启动monitor监控查看情况:"
# 等待 fastdfs 启动完成
/usr/bin/fdfs_monitor /etc/fdfs/storage.conf

# 启动 nginx
chmod +x /usr/local/nginx/sbin/nginx
echo "启动nginx:"
/usr/local/nginx/sbin/nginx -g "daemon off;"
