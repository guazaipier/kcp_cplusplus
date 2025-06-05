# 架构

# 编译
server：  
cd server && mkdir build && cd build && cmake.. && make -j4  
client：  
cd client && mkdir build && cd build && cmake .. && make -j4  

# 运行
## 本地运行
编译好之后，本地可以直接运行，先运行 server 端，再运行 client 端。
## Docker 运行
先编译好 server 和 client 的执行文件：  
### 方式一 server 和 client 同时构建镜像并运行：
在根目录下执行：
docker-compose build && docker-compose up
### 方式二 server 和 client 分别构建镜像后运行：
在 server 目录下执行：
cd server && docker-compose build && docker-compose up
在 client 目录下执行：
cd client && docker build -t kcp_client . && docker run --network host -it kcp_client

## 默认端口号：12345

### 一些想法：
server 端目前主要三个线程：
1. 监听线程：监听客户端的连接请求，并为每个客户端分配一个新的线程处理请求。
2. 定时器线程：定期刷新 kcp 的发送缓冲区，将 kcp 数据发送给客户端
3. 业务线程：处理客户端的消息，对 kcp 消息进行解包并给回调函数处理
其中线程2和3（3中的回调函数，例子中将客户端消息又发送回客户端）涉及到线程安全问题，这里直接在每次使用 kcp 的缓冲区时进行加锁，保证线程安全。
一旦客户端很多，加锁解锁很耗 cpu，目前想到的无锁方案是：
创建一个线程池，一个线程负责制定的 kcp 连接，每次 kcp 收发消息到绑定的那个线程中添加任务，线程在处理完任务，再 update flush kcp 发送缓冲区，这个就不需要锁了，因为相当于对 kcp 的操作只在一个线程中进行，不涉及线程安全问题了