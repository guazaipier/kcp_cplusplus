FROM ubuntu:24.04

# 设置工作目录
WORKDIR /opt/kcp_cplusplus
# 复制头文件和main程序到工作目录
COPY config .
COPY include .
COPY build/main .
# 暴露端口号
EXPOSE 12345
# 与主机共享日志文件
# VOLUME /opt/run/
# 添加维护者信息
# LABEL maintainer="guazaipier@github.com"
# 启动命令
CMD ["./main", "config.json"]







#上下文路径，是指 docker 在构建镜像，有时候想要使用到本机的文件（比如复制），docker build 命令得知这个路径后，会将路径下的所有内容打包。
# 解析：由于 docker 的运行模式是 C/S。我们本机是 C，docker 引擎是 S。实际的构建过程是在 docker 引擎下完成的，所以这个时候无法用到我们本机的文件。这就需要把我们本机的指定目录下的文件一起打包提供给 docker 引擎使用。
# 如果未说明最后一个参数，那么默认上下文路径就是 Dockerfile 所在的位置。
# 注意：上下文路径下不要放无用的文件，因为会一起打包发送给 docker 引擎，如果文件过多会造成过程缓慢。