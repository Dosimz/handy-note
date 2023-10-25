## TCP in handy lib
### server 
```cpp
#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char *argv[]) {
    Logger::getLogger().setLogLevel(Logger::LTRACE);
    EventBase base;
    Signal::signal(SIGINT, [&] { base.exit(); });
    // 启动服务端， 绑定地址 host 和对应的端口 port。 保持监听 20
    TcpServerPtr echo = TcpServer::startServer(&base, "", 2099);
    exitif(echo == NULL, "start tcp server failed");
    // 服务端正常运行后，实现 tcp 链接回调函数
    echo->onConnCreate([] {
        TcpConnPtr con(new TcpConn);
        con->onMsg(new LengthCodec, [](const TcpConnPtr &con, Slice msg) {
            info("recv msg: %.*s", (int) msg.size(), msg.data());
            con->sendMsg(msg);
        });
        return con;
    });    // createcb = cb
    base.loop();
    info("program exited");
}
```
以上为使用 handy 库的简易服务端 tcp 的代码。 其中主要实现了两个关键功能：
- 正常运行的等待 tcp 请求的服务端程序
- 在 tcp 请求连接到服务器后的回调函数
  
### 正常运行的等待 tcp 请求的服务端程序
```cpp
int TcpServer::bind(const std::string &host, unsigned short port, bool reusePort) {
    addr_ = Ip4Addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int r = net::setReuseAddr(fd);
    fatalif(r, "set socket reuse option failed");
    r = net::setReusePort(fd, reusePort);
    fatalif(r, "set socket reuse port option failed");
    r = util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(r, "addFdFlag FD_CLOEXEC failed");
    // success: 0 ; failed: -1
    r = ::bind(fd, (struct sockaddr *) &addr_.getAddr(), sizeof(struct sockaddr));
    // check 套接字是否成功绑定到指定的本地地址上    非零时会触发 if 语句
    if (r) {
        close(fd);
        error("bind to %s failed %d %s", addr_.toString().c_str(), errno, strerror(errno));
        return errno;
    }
    r = listen(fd, 20);
    fatalif(r, "listen failed %d %s", errno, strerror(errno));
    info("fd %d listening at %s", fd, addr_.toString().c_str());
    listen_channel_ = new Channel(base_, fd, kReadEvent);
    // readcb_ = 
    listen_channel_->onRead([this] { handleAccept(); });
    return 0;
}

TcpServerPtr TcpServer::startServer(EventBases *bases, const std::string &host, unsigned short port, bool reusePort) {
    TcpServerPtr p(new TcpServer(bases));
    int r = p->bind(host, port, reusePort);
    if (r) {
        error("bind to %s:%d failed %d %s", host.c_str(), port, errno, strerror(errno));
    }
    return r == 0 ? p : NULL;
}
```
这段代码实现与原始 cpp 实现的主要不同在于，其在成功开始监听 20 端口后，创建了一个 listen_channel 事件用于方便处理回调函数。
在 `onRead` 回调函数中调用了 `handleAccept()` 用于处理来自客户端的 Tcp 连接请求。

```cpp
void TcpServer::handleAccept() {
    struct sockaddr_in raddr; // 定义远程地址结构体
    socklen_t rsz = sizeof(raddr); // 定义远程地址结构体的长度
    int lfd = listen_channel_->fd(); // 获取监听 socket 的文件描述符
    int cfd;
    // 循环接受客户端连接
    while (lfd >= 0 && (cfd = accept(lfd, (struct sockaddr *) &raddr, &rsz)) >= 0) { 
        sockaddr_in peer, local;
        socklen_t alen = sizeof(peer);
        int r = getpeername(cfd, (sockaddr *) &peer, &alen); // 获取客户端地址
        if (r < 0) {
            error("get peer name failed %d %s", errno, strerror(errno));
            continue;
        }
        r = getsockname(cfd, (sockaddr *) &local, &alen); // 获取本地地址
        if (r < 0) {
            error("getsockname failed %d %s", errno, strerror(errno));
            continue;
        }
        r = util::addFdFlag(cfd, FD_CLOEXEC); // 设置文件描述符标志位
        fatalif(r, "addFdFlag FD_CLOEXEC failed");
        EventBase *b = bases_->allocBase(); // 从 EventBase 对象池中获取一个 EventBase 对象
        auto addcon = [=] { // 定义一个 lambda 表达式，用于添加连接
            TcpConnPtr con = createcb_(); // 创建一个 TcpConn 对象
            con->attach(b, cfd, local, peer); // 将 TcpConn 对象与 EventBase 对象和 socket 文件描述符关联
            if (statecb_) {
                con->onState(statecb_); // 设置连接状态回调函数
            }
            if (readcb_) {
                con->onRead(readcb_); // 设置读事件回调函数
            }
            if (msgcb_) {
                con->onMsg(codec_->clone(), msgcb_); // 设置消息回调函数
            }
        };
        if (b == base_) { // 如果当前 EventBase 对象是主 EventBase 对象，则直接添加连接
            addcon();
        } else { // 否则，将添加连接的操作放到当前 EventBase 对象的任务队列中
            b->safeCall(move(addcon));
        }
    }
    if (lfd >= 0 && errno != EAGAIN && errno != EINTR) { // 如果 accept 函数出错，则输出警告信息
        warn("accept return %d  %d %s", cfd, errno, strerror(errno));
    }
}
```
其中 `getpeername` 和 `getsockname` 为系统调用的函数，用于获取远程地址信息和本地地址信息，成功时返回 0.

### 在 tcp 请求连接到服务器后的回调函数
```cpp
    // input: 要求为一个 TcpConnPtr 类型的 function
    void onConnCreate(const std::function<TcpConnPtr()> &cb) { createcb_ = cb; }
```


下面这段 Lambda 函数将作为回调函数的实现存入 `createcb_`，并等待时机执行
```cpp
[] {
        TcpConnPtr con(new TcpConn);
        con->onMsg(new LengthCodec, [](const TcpConnPtr &con, Slice msg) {
            info("recv msg: %.*s", (int) msg.size(), msg.data());
            con->sendMsg(msg);
        });
        return con;
    }
```


### client 
```cpp
#include <handy/handy.h>

using namespace std;
using namespace handy;

int main(int argc, const char *argv[]) {
    setloglevel("TRACE");
    EventBase base;
    Signal::signal(SIGINT, [&] { base.exit(); });
    // 建立 socket 套接字 fd， 创建 channel 对象，便于事件循环和通讯
    TcpConnPtr con = TcpConn::createConnection(&base, "127.0.0.1", 2099, 3000);
    // 设置重连的毫秒数 reconnectInterval_  = 3000
    con->setReconnectInterval(3000);
    // 打印输入的传递信息
    con->onMsg(new LengthCodec, [](const TcpConnPtr &con, Slice msg) { info("recv msg: %.*s", (int) msg.size(), msg.data()); });
    // 将状态相关的回调函数存入  statecb_, 当检测到成功连接的状态时，使用 sendMsg 传递信息
    con->onState([=](const TcpConnPtr &con) {
        info("onState called state: %d", con->getState());
        if (con->getState() == TcpConn::Connected) {
            con->sendMsg("hello");
        }
    });
    base.loop();
    info("program exited");
}
```
其主要实现了三个功能
- 建立套接字并封装到 channel 
- 设置消息传递的回调函数
- 设置连接状态检测的回调，并传递消息

### 建立套接字并封装到 channel 
```cpp
    //可传入连接类型，返回智能指针
    template <class C = TcpConn>
    static TcpConnPtr createConnection(EventBase *base, const std::string &host, unsigned short port, int timeout = 0, const std::string &localip = "") {
        TcpConnPtr con(new C);
        con->connect(base, host, port, timeout, localip);
        return con;
    }
```
调用该类函数将创建一个 TcpConn 对象，并调用该对象的  connect 函数绑定 host、port等。
```cpp
void TcpConn::connect(EventBase *base, const string &host, unsigned short port, int timeout, const string &localip) {
    fatalif(state_ != State::Invalid && state_ != State::Closed && state_ != State::Failed, "current state is bad state to connect. state: %d", state_);
    destHost_ = host;
    destPort_ = port;
    connectTimeout_ = timeout;
    connectedTime_ = util::timeMilli();
    localIp_ = localip;
    Ip4Addr addr(host, port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fatalif(fd < 0, "socket failed %d %s", errno, strerror(errno));
    net::setNonBlock(fd);
    int t = util::addFdFlag(fd, FD_CLOEXEC);
    fatalif(t, "addFdFlag FD_CLOEXEC failed %d %s", t, strerror(t));
    int r = 0;
    if (localip.size()) {
        Ip4Addr addr(localip, 0);
        r = ::bind(fd, (struct sockaddr *) &addr.getAddr(), sizeof(struct sockaddr));
        error("bind to %s failed error %d %s", addr.toString().c_str(), errno, strerror(errno));
    }
    if (r == 0) {
        r = ::connect(fd, (sockaddr *) &addr.getAddr(), sizeof(sockaddr_in));
        if (r != 0 && errno != EINPROGRESS) {
            error("connect to %s error %d %s", addr.toString().c_str(), errno, strerror(errno));
        }
    }

    sockaddr_in local;
    socklen_t alen = sizeof(local);
    if (r == 0) {
        r = getsockname(fd, (sockaddr *) &local, &alen);
        if (r < 0) {
            error("getsockname failed %d %s", errno, strerror(errno));
        }
    }
    // 创建套接字，将目标 host 和 port 传入 Ip4Addr 对象保存
    state_ = State::Handshaking;
    // 将 tcp 连接的准备工作封装进 事件 中
    attach(base, fd, Ip4Addr(local), addr);
    if (timeout) {
        TcpConnPtr con = shared_from_this();
        timeoutId_ = base->runAfter(timeout, [con] {
            if (con->getState() == Handshaking) {
                con->channel_->close();
            }
        });
    }
}
```
常规的 socket 套接字创建，保存参数中的 host 和 port 到变量中.

```cpp
void TcpConn::attach(EventBase *base, int fd, Ip4Addr local, Ip4Addr peer) {
    fatalif((destPort_ <= 0 && state_ != State::Invalid) || (destPort_ >= 0 && state_ != State::Handshaking),
            "you should use a new TcpConn to attach. state: %d", state_);
    base_ = base;
    state_ = State::Handshaking;
    local_ = local;
    peer_ = peer;
    delete channel_;
    channel_ = new Channel(base, fd, kWriteEvent | kReadEvent);
    trace("tcp constructed %s - %s fd: %d", local_.toString().c_str(), peer_.toString().c_str(), fd);
    TcpConnPtr con = shared_from_this();
    con->channel_->onRead([=] { con->handleRead(con); });
    con->channel_->onWrite([=] { con->handleWrite(con); });
}
```
使用 channel 的 read、write 事件处理器进行监听.  channel 的参数类型为 `std::function<void()>`。`    void onRead(const Task &readcb) { readcb_ = readcb; }` 其将当前 TcpConn 的handleRead 方法存入 channel 的 readcb_ 成员变量中。

```cpp
void TcpConn::handleRead(const TcpConnPtr &con) {
    if (state_ == State::Handshaking && handleHandshake(con)) {
        return;
    }
    while (state_ == State::Connected) {
        input_.makeRoom();
        int rd = 0;
        // 套接字返回值正常时
        if (channel_->fd() >= 0) {
            rd = readImp(channel_->fd(), input_.end(), input_.space());
            trace("channel %lld fd %d readed %d bytes", (long long) channel_->id(), channel_->fd(), rd);
        }
        if (rd == -1 && errno == EINTR) {
            continue;
        } else if (rd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            for (auto &idle : idleIds_) {
                handyUpdateIdle(getBase(), idle);
            }
            if (readcb_ && input_.size()) {
                readcb_(con);
            }
            break;
        } else if (channel_->fd() == -1 || rd == 0 || rd == -1) {
            cleanup(con);
            break;
        // rd 读取未完成时
        } else {  // rd > 0
            input_.addSize(rd);
        }
    }
}
```
当 state 为连接时，通过 `readImp` 不断读取套接字中的 `input_` 数据。`input_.makeRoom` 和 `input_.addSize(rd)` 来进行控制更新读取内容。

`readImp` 函数实现`virtual int readImp(int fd, void *buf, size_t bytes) { return ::read(fd, buf, bytes); }` ，其中 `buf` 和 `bytes` 为 `input_.end()` 和 `input_.space()`
〉::read(fd, buf, bytes) 函数的返回结果是读取的字节数。该函数用于从文件描述符 fd 指定的文件中读取最多 bytes 个字节的数据，并将数据存储到 buf 指向的缓冲区中。
如果读取成功，::read 函数返回实际读取的字节数，可能小于请求的字节数 bytes。如果读取失败，::read 函数返回 -1，并设置 errno 变量表示错误类型。
总的来说，::read(fd, buf, bytes) 函数的返回结果是读取的字节数，可能小于请求的字节数 bytes。如果读取失败，返回 -1，并设置 errno 变量表示错误类型。

```cpp
struct Buffer {
    Buffer() : buf_(NULL), b_(0), e_(0), cap_(0), exp_(512) {}
   private:
    char *buf_;
    size_t b_, e_, cap_, exp_;

    char *end() const { return buf_ + e_; }


    size_t space() const { return cap_ - e_; }
```
可见其初始值皆为 0。

```cpp
    void makeRoom() {
        if (space() < exp_)
            expand(0);
    }

    void Buffer::expand(size_t len) {
        size_t ncap = std::max(exp_, std::max(2 * cap_, size() + len));
        char *p = new char[ncap];
        std::copy(begin(), end(), p);
        e_ -= b_;
        b_ = 0;
        delete[] buf_;
        buf_ = p;
        cap_ = ncap;
    }
```
然而，在使用 `makeRoom` 函数后，其调用 `expand` 扩充 `cap_`。

`void addSize(size_t len) { e_ += len; }` 调整 `e_` 控制读取。


### 设置消息传递的回调函数
```cpp
    con->onMsg(new LengthCodec, [](const TcpConnPtr &con, Slice msg) { info("recv msg: %.*s",(int) msg.size(), msg.data()); }); 
```
该 lambda 函数将作为 `cb` 传入到下面的`onMsg` 函数中。
`onMsg` 函数具体实现如下
```cpp
void TcpConn::onMsg(CodecBase *codec, const MsgCallBack &cb) {
    assert(!readcb_);
    // 将 codec_ 指向的对象释放，并将其指向 codec 所指向的对象
    codec_.reset(codec);
    // 将 lambda 函数赋值给 readcb_
    onRead([cb](const TcpConnPtr &con) {
        int r = 1;
        while (r) {
            Slice msg;
            r = con->codec_->tryDecode(con->getInput(), msg);
            if (r < 0) {
                con->channel_->close();
                break;
            } else if (r > 0) {
                trace("a msg decoded. origin len %d msg len %ld", r, msg.size());
                cb(con, msg);
                con->getInput().consume(r);
            }
        }
    });
}
```
当触发到 `readcb_` 时，调用该 lambda 函数。第一步使用传入 codec 的 `tryDecode` 函数将当前连接对象中 `input_` 读取到 msg 中。第二步，使用前面提到的 `cb` 函数在终端打印信息。
```cpp
int LineCodec::tryDecode(Slice data, Slice &msg) {
    if (data.size() == 1 && data[0] == 0x04) {
        msg = data;
        return 1;
    }
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] == '\n') {
            if (i > 0 && data[i - 1] == '\r') {
                msg = Slice(data.data(), i - 1);
                return static_cast<int>(i + 1);
            } else {
                msg = Slice(data.data(), i);
                return static_cast<int>(i + 1); 
            }
        }
    }
    return 0;
}
```

### 设置连接状态检测的回调，并传递消息
前面的内容提到，关于消息传递的回调函数是从当前连接的 `input_` 成员变量中读取消息信息的。那么这个变量里的内容是从哪儿来的呢。
```cpp
    con->onState([=](const TcpConnPtr &con) {
        info("onState called state: %d", con->getState());
        if (con->getState() == TcpConn::Connected) {
            con->sendMsg("hello");
        }
    });
```

```cpp
void TcpConn::sendMsg(Slice msg) {
    codec_->encode(msg, getOutput());
    sendOutput();
}
```
将 `msg` 内的信息通过 `Buffer &getOutput() { return output_; }` 存储到 `output_`

```cpp
void LineCodec::encode(Slice msg, Buffer &buf) {
    buf.append(msg).append("\r\n");
}
```
接着使用 ` void sendOutput() { send(output_); }`  调用 `send` 函数
```cpp
void TcpConn::send(Buffer &buf) {
    if (channel_) {
        // 检验是否可写
        if (channel_->writeEnabled()) {  // just full
            output_.absorb(buf);
        }
        if (buf.size()) {
            // 写入
            ssize_t sended = isend(buf.begin(), buf.size());
            // 消耗缓冲区已被写入的数据
            buf.consume(sended);
        }
        // 检测是数据是否写入完成
        if (buf.size()) {
            output_.absorb(buf);
            if (!channel_->writeEnabled()) {
                channel_->enableWrite(true);
            }
        }
    } else {
        warn("connection %s - %s closed, but still writing %lu bytes", local_.toString().c_str(), peer_.toString().c_str(), buf.size());
    }
}
```
这里给出检验是否可写的代码 `bool Channel::writeEnabled() {return events_ & kWriteEvent;}`

```cpp
Buffer &Buffer::absorb(Buffer &buf) {
    if (&buf != this) {
        if (size() == 0) {
            char b[sizeof buf];
            memcpy(b, this, sizeof b);
            memcpy(this, &buf, sizeof b);
            memcpy(&buf, b, sizeof b);
            std::swap(exp_, buf.exp_);  // keep the origin exp_
        } else {
            append(buf.begin(), buf.size());
            buf.clear();
        }
    }
    return *this;
}
```
这段代码实现了 `Buffer` 类中的 `absorb` 函数，用于将另一个 `Buffer` 对象中的数据合并到当前对象中。

具体来说，`absorb` 函数的参数是一个 `Buffer` 类型的引用 `buf`，表示要合并的另一个 `Buffer` 对象。在函数中，首先判断 `buf` 是否与当前对象相同，如果相同则不进行任何操作，直接返回当前对象的引用。如果不同，则根据当前对象是否为空进行不同的操作。

如果当前对象为空，即 `size()` 返回值为 0，说明当前对象中没有数据。此时，将当前对象和 `buf` 中的数据进行交换，即将当前对象的数据存储到临时变量 `b` 中，然后将 `buf` 中的数据存储到当前对象中，最后将 `b` 中的数据存储到 `buf` 中。这样，`buf` 中的数据就被合并到了当前对象中。同时，使用 `std::swap` 函数交换了当前对象和 `buf` 中的 `exp_` 成员变量，以保留当前对象的 `exp_` 值。

如果当前对象不为空，即 `size()` 返回值不为 0，说明当前对象中已经有数据。此时，使用 `append` 函数将 `buf` 中的数据追加到当前对象的末尾，然后使用 `clear` 函数清空 `buf` 中的数据。这样，`buf` 中的数据就被合并到了当前对象中。

最后，函数返回当前对象的引用 `*this`，以便支持链式调用。

总的来说，这段代码实现了 `Buffer` 类中的 `absorb` 函数，用于将另一个 `Buffer` 对象中的数据合并到当前对象中。

合并后，下一步进行写入操作。

```cpp
ssize_t TcpConn::isend(const char *buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        // virtual int writeImp(int fd, const void *buf, size_t bytes) { return ::write(fd, buf, bytes); }
        ssize_t wd = writeImp(channel_->fd(), buf + sended, len - sended);
        trace("channel %lld fd %d write %ld bytes", (long long) channel_->id(), channel_->fd(), wd);
        if (wd > 0) {
            sended += wd;
            continue;
        } else if (wd == -1 && errno == EINTR) {
            continue;
        } else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!channel_->writeEnabled()) {
                channel_->enableWrite(true);
            }
            break;
        } else {
            error("write error: channel %lld fd %d wd %ld %d %s", (long long) channel_->id(), channel_->fd(), wd, errno, strerror(errno));
            break;
        }
    }
    return sended;
}
```

写入完成后会进一步对缓冲区进行检测，并消耗指定长度的数据`buf.consume(sended);`。
```cpp
    Buffer &consume(size_t len) {
        b_ += len;
        if (size() == 0)
            clear();
        return *this;
    }
```
具体来说，consume 函数的参数是一个 size_t 类型的整数 len，表示要消耗的数据长度。在函数中，将 b_ 成员变量的值增加 len，表示从缓冲区中消耗了 len 个字节的数据。然后，判断当前缓冲区中是否还有数据，即 size() 返回值是否为 0。如果当前缓冲区中没有数据，即 size() 返回值为 0，说明已经消耗了所有数据，此时调用 clear 函数清空缓冲区。最后，函数返回当前对象的引用 *this，以便支持链式调用。