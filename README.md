# pangolin-frp
内网穿透及端口转发工具，可通过配置端口映射将局域网服务映射到公网。
> 支持TCP HTTP SSH等协议；
> 支持多连接，支持单机多映射、多机多映射；
<br>

## 使用方法
1. 将项目克隆到你的主机并进入pangolin-frp目录；执行如下命令：
```shell
#使用CMake编译
mkdir build && cd build
cmake .. && make
```
2. 编辑配置文件：
> server-cfg.ini<br>

```ini
; 服务端映射配置
; 配置单条或多条映射
; 一个 ini 配置文件由多个 section 组成(即"[]"内容),每个 section 就是一条映射
; section 的名字可以随意命名
; listen : 公网监听端口,用户访问此端口
; forward: 内网端口,公网将用户数据转发至此端口
[mapping]
listen = 0.0.0.0:12345
forward = 0.0.0.0:8080
```
> client-cfg.ini<br>

```ini
; 客户端映射配置 intranet:内网端口, public:公网IP:端口
[mapping]
intranet = 127.0.0.1:22
public = x.x.x.x:8080

```
3. 启动程序
> 服务端运行在公网服务器上，客户端运行在局域网。（也可以在同一台机器）
```shell
#启动服务器
./pangolin -f server-cfg.ini
#启动客户端
./pangolin-cli -f client-cfg.ini
```
4. 测试
```shell
ssh -p 12345 user@publicip
```
