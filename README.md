编译说明
mkdir build
cd build
cmake ..
make
得到执行文件
tc_http_server


需要将修改的tc_http_server.conf的拷贝到执行目录。

v0.2 tuchuang-2
目前该版本为单线程版本，日志也直接打印到控制台，随着课程会不断迭代。


v0.3 tuchuang-3
1. 日志异步处理
2. 文件数量 计数采用redis
3. 使用c++11线程池处理，目前只是小部分函数使用了多线程处理
4. 数据回发
5. 修复一些bug