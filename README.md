## 深入理解·(网络-系统-数据结构算法-数据库)·--计算机网络篇

- 一个完整的 http 连接的过程是怎样的？
- DNS(Domain System Name) 是什么?
- DNS 解析的过程
- 为什么 DNS 使用 UDP 协议
- 什么是区域传送？ 为什么区域传送用 TCP 协议
>指的是 DNS 服务器之间同步数据的过程
- 什么是TCP粘包/拆包？发生的原因？
>TCP 粘包
>粘包现象发生在发送方连续发送了多个包，而接收方接收时这些包被合并为一个或几个大包。这通常发生在以下情况：

>发送方连续快速发送小数据包：如果发送方连续发送多个小数据包，TCP 协议可能会将这些小包合并成一个大包发送，以提高网络效率和降低头部开销。
>接收方接收缓冲区不及时处理：如果接收方的应用层没有及时处理接收缓冲区的数据，多个包可能在接收缓冲区中累积，形成粘包。
>TCP 拆包
>拆包是指一个较大的数据包在发送过程中被分割成多个小包。这可能由于几个原因导致：

>MSS（最大段大小）限制：TCP 协议会根据网络条件（如 MTU，最大传输单元）和对方的接收能力，将大数据包分割为合适大小的小包进行发送。
>流控制和拥塞控制：TCP 协议中的流控制和拥塞控制机制可能会导致大包被分割，以适应网络状况。

### TCP
#### 三次握手 🤝
<a href="https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg" alt="alt" width="600"/>
</a>

> 客户端使用 `int connect(int, const struct sockaddr *, socklen_t)` 命令连接服务器时，三次握手建立连接。

我们首先点开第一行由客户端发向服务端的连接，可以看到下面图片中所展示的TCP报文段更详细的信息。
<a href="https://github.com/Dosimz/handy-note/blob/main/img/step1.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/step1.jpg" alt="alt" width="600"/>
</a> 
 
可以看到其中包含 *源端口、目标端口*、*序列号*、*确认序列号*、*TCP标识位* 

<a href="https://github.com/Dosimz/handy-note/blob/main/img/step2.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/step2.jpg" alt="alt" width="600"/>
</a> 


<a href="https://github.com/Dosimz/handy-note/blob/main/img/step3.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/step3.jpg" alt="alt" width="600"/>
</a>   

> TCP Flag
> TCP 首部中有 6 个标志比特，它们中的多个可同时被设置为 1，主要是用于操控 TCP 的状态机的，依次为URG，ACK，PSH，RST，SYN，FIN。
> 当然只介绍三个：
> ACK：这个标识可以理解为发送端发送数据到接收端，发送的时候 ACK 为 0，标识接收端还未应答，一旦接收端接收数据之后，就将 ACK 置为 1，发送端接收到之后，就知道了接收端已经接收了数据。
> SYN：表示「同步序列号」，是 TCP 握手的发送的第一个数据包。用来建立 TCP 的连接。SYN 标志位和 ACK 标志位搭配使用，当连接请求的时候，SYN=1，ACK=0连接被响应的时候，SYN=1，ACK=1；这个标志的数据包经常被用来进行端口扫描。扫描者发送一个只有 SYN 的数据包，如果对方主机响应了一个数据包回来 ，就表明这台主机存在这个端口。
> FIN：表示发送端已经达到数据末尾，也就是说双方的数据传送完成，没有数据可以传送了，发送FIN标志位的 TCP 数据包后，连接将被断开。这个标志的数据包也经常被用于进行端口扫描。发送端只剩最后的一段数据了，同时要告诉接收端后边没有数据可以接受了，所以用FIN标识一下，接收端看到这个FIN之后，哦！这是接受的最后的数据，接受完就关闭了；TCP四次分手必然问。

三次握手的变化如下：
刚开始客户端处于 Closed 的状态，服务端处于 Listen 状态，进行三次握手：

第一次握手：客户端给服务端发一个 SYN 报文，并指明客户端的初始化序列号 ISN(c)。此时客户端处于 SYN_SEND 状态。

首部的同步位SYN=1，初始序号seq=x，SYN=1的报文段不能携带数据，但要消耗掉一个序号。 
<a href="https://github.com/Dosimz/handy-note/blob/main/img/3times1.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/3times1.jpg" alt="alt" width="600"/>
</a>  

第二次握手：服务器收到客户端的 SYN 报文之后，会以自己的 SYN 报文作为应答，并且也是指定了自己的初始化序列号 ISN(s)。同时会把客户端的 ISN + 1 作为ACK 的值，表示自己已经收到了客户端的 SYN，此时服务器处于 SYN_RCVD 的状态。

在确认报文段中SYN=1，ACK=1，确认号ack=x+1，初始序号seq=y。 
<a href="https://github.com/Dosimz/handy-note/blob/main/img/3times2.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/3times2.jpg" alt="alt" width="600"/>
</a>   

第三次握手：客户端收到 SYN 报文之后，会发送一个 ACK 报文，当然，也是一样把服务器的 ISN + 1 作为 ACK 的值，表示已经收到了服务端的 SYN 报文，此时客户端处于 ESTABLISHED 状态。服务器收到 ACK 报文之后，也处于 ESTABLISHED 状态，此时，双方已建立起了连接。

确认报文段ACK=1，确认号ack=y+1，序号seq=x+1（初始为seq=x，第二个报文段所以要+1），ACK报文段可以携带数据，不携带数据则不消耗序号。

发送第一个SYN的一端将执行主动打开（active open），接收这个SYN并发回下一个SYN的另一端执行被动打开（passive open）。 

<a href="https://github.com/Dosimz/handy-note/blob/main/img/3times3.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/3times3.jpg" alt="alt" width="600"/>
</a>  
在socket编程中，客户端执行connect()时，将触发三次握手。

#### 四次挥手 🙋 
<a href="https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg" alt="alt" width="600"/>
</a>  

> 客户端与服务器之间连接断开时，四次挥手。 `close(sockfd)` 


#### TCP 四大拥塞控制总结

<a href="https://github.com/Dosimz/handy-note/blob/main/img/tcp4Alogrim.jpg">
  <img src="https://github.com/Dosimz/handy-note/blob/main/img/tcp4Alogrim.jpg" alt="alt" width="600"/>
</a>    

> cwnd 大小为 1， 意味着 1 个 MSS 大小的数据  
> 轮次的时间即： 往返延迟时间RTT(Round-Trip Time)  


#### TCP 流量控制原理


#### 服务端建立 TCP 连接的系统调用过程  







## 深入理解·(网络-系统-数据结构算法-数据库)·--操作系统



