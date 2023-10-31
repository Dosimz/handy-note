### The OSI (Open Systems Interconnection) model consists of seven layers
- Physical Layer : Bits
- Data Link Layer : Frames
- Network Layer : Packages
- Transport Layer : Segments
- Seesion Layer()
- Presentation Layer()
- Application Layer

### The four-layer network model typically refers to the TCP/IP model, which is a simplified and widely used model for understanding and implementing network protocols.

- Link Layer : Frames
- Network Layer: Packages
- Transport Layer: Segments
- Application Layer: Message/Data

### General Knowledge

- TCP 和 UDP 通常使用两个字节来存放端口信息。1 个字节(byte) 通常由 8 个比特(bit)构成，所以端口范围为 0~65535((2^8)-1)

- MTU：maximum transmission unit，最大传输单元，由硬件规定，如以太网的MTU为1500字节。

- MSS：maximum segment size，最大分节大小，为TCP数据包每次传输的最大数据分段大小，一般由发送端向对端TCP通知对端在每个分节中能发送的最大TCP数据。MSS值为MTU值减去IPv4 Header（20 Byte）和TCP header（20 Byte）得到

MTU（Maximum Transmission Unit）：MTU 是网络链路层（通常是数据链路层）的一个概念，表示在特定网络链路上能够传输的最大数据包的大小。MSS 的计算通常基于 MTU 的值。

IP 头部大小：MSS 的计算还可能考虑 IP 头部的大小，因为 IP 头部也会占用数据包的一部分。

TCP 头部大小：TCP 头部包含了一些控制信息，也会占用数据包的一部分。

通常情况下，MSS 的计算不需要涉及 MAC 地址，因为 MAC 地址是用于局域网内设备之间的通信，而 MSS 主要用于 TCP 层的控制。计算 MSS 主要涉及确定 TCP 数据包的最大大小，以便它们可以正确地在网络链路上传输，而不会导致分段或丢失数据。MAC 地址在更低的数据链路层中使用，用于设备之间的帧传输。


IP 头部和TCP 头部通常都大致是20字节的主要原因是设计和历史因素以及网络协议的需要。让我们分别看看这两个头部的原因：

- IP 头部（IPv4）：

地址信息：IP 头部包含源IP地址和目标IP地址，每个地址通常占4字节，共8字节。
服务质量（Quality of Service）：用于指定数据包的优先级和服务质量的字段。
生存时间（Time to Live）：指示数据包在网络中的最大生存时间，以防止数据包在网络中永远循环。
协议字段：指示上层协议，例如TCP、UDP或ICMP。
头部校验和：用于校验IP头部的完整性。
其他标志和字段：还包括一些其他字段，如标识、标志位和片偏移等。
综合以上信息，IPv4 头部通常占用20字节。

- TCP 头部：
〉较准确参考链接： https://blog.csdn.net/ruiguang21/article/details/126893150
端口信息：TCP 头部包含源端口和目标端口，每个端口通常占用2字节，共4字节。
序列号和确认号：用于 TCP 连接的序列号和确认号字段，通常占4字节。
标志位和窗口大小：包括标志位字段，用于控制 TCP 连接的状态，以及窗口大小字段，用于流量控制。
紧急指针：用于指示紧急数据。
选项字段：包括可选的选项字段，用于协商最大段大小（MSS）等。
校验和：用于校验TCP头部和数据的完整性。
综合以上信息，TCP 头部通常占用20字节。

- 常见TCP的连接状态有哪些

1. **CLOSED**：连接初始状态，表示未建立连接。

2. **LISTEN**：服务器处于监听状态，等待来自客户端的连接请求。

3. **SYN_SENT**：客户端在发起连接请求时，发送SYN包，进入此状态。

4. **SYN_RECEIVED**：服务器收到客户端的连接请求（SYN包）并发送响应（SYN-ACK包），进入此状态。

5. **ESTABLISHED**：连接建立状态，表示双方成功建立连接。在这个状态下，数据可以在客户端和服务器之间双向传输。

6. **FIN_WAIT_1**：连接的一方（通常是客户端）发送了FIN包，等待对方的ACK响应。

7. **CLOSE_WAIT**：连接的一方（通常是服务器）接收到对方的FIN包，等待自己的应用程序决定是否关闭连接。在这个状态下，仍然可以发送数据给对方。

8. **FIN_WAIT_2**：客户端接收到服务器的ACK包，等待对方的FIN包。

9. **LAST_ACK**：服务器发送最后的FIN包，等待客户端的ACK响应。

10. **TIME_WAIT**：在连接关闭后，双方都会进入TIME_WAIT状态，以确保任何延迟的数据包都不会在网络中迷失。这个状态在一段时间后会自动结束。

总的来说，这些状态描述了TCP连接在建立、使用和关闭过程中的不同阶段。每个状态都有特定的行为和条件。需要注意的是，具体的实现和细节可能会因TCP栈的不同而略有不同，但上述描述是一种通用的表达方式。


### Wireshark 抓包分析
#### Wireshark 基本知识
当你在Wireshark中捕获数据时，你会遇到多个网络接口。例如：`WI-FI:en0` 和 `Loopback:lo0`。选择哪个取决于你想捕获哪种类型的流量：

1. **WI-FI:en0**:
   - 这通常代表你的无线网络接口。选择此接口意味着你将捕获通过Wi-Fi适配器发送和接收的所有数据包。如果你的C++ TCP程序通过Wi-Fi与外部服务器或其他计算机通信，那么你应该选择此接口。

2. **Loopback:lo0**:
   - 这是一个特殊的网络接口，用于在计算机内部捕获从一个进程到另一个进程的通信。简单地说，它捕获在同一台计算机上的两个程序之间的通信。如果你的C++程序与运行在同一台机器上的另一个服务或应用程序通信（例如，客户端和服务器都在同一台计算机上运行），那么你应该选择此接口。

#### Wireshark 基本过滤规则
在Wireshark中使用过滤器捕获C++ TCP程序的流量，首先你需要知道一些关于你的程序的基本信息，如使用的端口号或与之通信的IP地址。以下是如何设置不同的过滤器：

1. **基于端口过滤**:
   如果你知道你的C++ TCP程序使用的确切端口号，你可以设置一个简单的过滤器。
   ```
   tcp.port == 12345
   ```
   这里，`12345`是你程序使用的端口号。替换为实际的端口号。

2. **基于源或目的IP地址过滤**:
   如果你知道与你的程序通信的特定IP地址，可以使用以下过滤器：
   ```
   ip.src == 192.168.1.100 || ip.dst == 192.168.1.100
   ```
   其中`192.168.1.100`是你要过滤的IP地址。这将显示所有从或到该IP地址的数据包。

3. **组合端口和IP地址过滤**:
   ```
   tcp.port == 12345 && (ip.src == 192.168.1.100 || ip.dst == 192.168.1.100)
   ```

4. **仅显示SYN包**:
   如果你只想看到TCP连接的建立（SYN包），你可以使用以下过滤器：
   ```
   tcp.flags.syn == 1 && tcp.flags.ack == 0
   ```

5. **跟踪特定的TCP流**:
   - 如果在捕获中找到一个你感兴趣的TCP数据包，你可以右键点击那个数据包，然后选择`Follow` > `TCP Stream`。这将过滤并只显示该特定TCP连接的所有数据包。

6. **自定义复杂的过滤器**:
   你可以组合多个条件来创建更复杂的过滤器。例如，你可以结合端口、IP地址和其他条件。

一旦你输入了过滤器，按`Enter`键。Wireshark将立即应用这个过滤器并只显示匹配的数据包。

为了更准确地捕获你的C++程序的流量，最好先知道它使用的端口或与其通信的其他相关信息。如果你的程序动态地选择端口，那么你可能需要先捕获所有流量，然后根据其他标识信息（如协议内容或特定的数据包模式）来找到你的程序的流量。

#### Wireshark 抓取
你提供的这条记录是TCP三次握手的第一步。根据你给出的信息，我们可以分析以下内容：

```
43    22.189229    127.0.0.1    127.0.0.1    TCP    68    50801 → 20749 [SYN] Seq=0 Win=65535 Len=0 MSS=16344 WS=64 TSval=2710313451 TSecr=0 SACK_PERM
```

- `43`: 数据包编号。
- `22.189229`: 时间戳，表示此数据包被捕获的时间。
- `127.0.0.1`: 源IP地址。这是一个loopback地址，表示这是在本地机器上生成的数据包。
- `127.0.0.1`: 目的IP地址。也是一个loopback地址，表示这个数据包是发送给本地机器的。
- `TCP`: 表示这是一个TCP协议的数据包。
- `68`: 数据包的总长度（包括头部和数据）。
- `50801 → 20749`: 源端口号50801到目标端口号20749的通信。
- `[SYN]`: 这是一个SYN包，表示尝试建立一个新的TCP连接。
- `Seq=0`: 序列号是0，这是一个新连接的开始。
- `Win=65535`: 窗口大小。这告诉接收方发送方的缓冲区有多少空间可以接收数据。
- `Len=0`: 数据长度是0，因为这只是一个SYN包，没有携带任何实际的数据。
- `MSS=16344`: 最大分段大小。这是发送方告诉接收方它愿意接收的每个TCP段的最大长度。
- `WS=64`: 窗口扩大因子。
- `TSval=2710313451`: 时间戳值。
- `TSecr=0`: 时间戳回应值，因为这是SYN包，所以它的值为0。
- `SACK_PERM`: 表示发送方支持选择性确认（Selective Acknowledgment）。

要完全了解TCP三次握手的过程，你应该寻找以下三个主要数据包：

1. **SYN**: 由客户端发送，请求建立连接。
2. **SYN-ACK**: 由服务器发送，确认收到SYN，并且自己也发送一个SYN。
3. **ACK**: 由客户端发送，确认收到服务器的SYN。

在你给出的这个例子中，你已经有了第一步（SYN）。为了完全理解这个TCP握手，你需要找到与之匹配的SYN-ACK和ACK数据包。你可以在Wireshark中继续查看接下来的数据包，寻找来自目的端口20749的回应。


[![alt](https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg)](https://github.com/Dosimz/handy-note/blob/main/img/tcp1.jpg)
> 客户端使用 `int connect(int, const struct sockaddr *, socklen_t)` 命令连接服务器时，三次握手建立连接。

[![alt](https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg)](https://github.com/Dosimz/handy-note/blob/main/img/tcp2.jpeg)
> 客户端与服务器之间连接断开时，四次挥手。

〉*Issues-1*: 什么是区域传送？ 为什么区域传送要用 TCP?

〉*Issues-2*: 什么是 TCP 粘包？ TCP 粘包问题的复现与解决?

〉*Issues-3*: TCP 头部报文字段包含哪些内容？

