## 深入理解·(网络-系统-数据结构算法-数据库)·--计算机网络篇

### TCP
#### 三次握手 🤝
[![alt](https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg)](https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg)
> 客户端使用 `int connect(int, const struct sockaddr *, socklen_t)` 命令连接服务器时，三次握手建立连接。

[![alt](https://github.com/Dosimz/handy-note/blob/main/img/step1.jpg)](https://github.com/Dosimz/handy-note/blob/main/img/step1.jpg)

[![alt](https://github.com/Dosimz/handy-note/blob/main/img/step2.jpg)](https://github.com/Dosimz/handy-note/blob/main/img/step2.jpg)

[![alt](https://github.com/Dosimz/handy-note/blob/main/img/step3.jpg)](https://github.com/Dosimz/handy-note/blob/main/img/step3.jpg)


#### 四次挥手 🙋
[![alt](https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg)](https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg)
> 客户端与服务器之间连接断开时，四次挥手。 `close(sockfd)`