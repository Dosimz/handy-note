## handy 库的事件驱动原理
`handy` 的事件驱动模型是基于 Reactor 模式实现的，主要包括以下几个组件：

1. `EventBase`：事件循环基类，负责事件循环和事件分发。

2. `Channel`：事件通道类，负责事件的注册、删除和分发。

3. `Poller`：事件分发器类，负责事件的分发和处理。

4. `TimerQueue`：定时器队列类，负责定时器的管理和触发。

在 `handy` 中，每个 `EventBase` 对象都有一个 `Poller` 对象和一个 `TimerQueue` 对象，用于处理事件和定时器。当一个事件发生时，`Channel` 对象会将事件添加到 `EventBase` 对象的事件队列中，然后 `EventBase` 对象会调用 `Poller` 对象的 `poll` 函数等待事件的发生。当事件发生时，`Poller` 对象会将事件分发给对应的 `Channel` 对象进行处理。同时，`EventBase` 对象还会定期检查 `TimerQueue` 对象中的定时器，如果有定时器到期，就会触发定时器回调函数。

总的来说，`handy` 的事件驱动模型是基于 Reactor 模式实现的，通过 `EventBase`、`Channel`、`Poller` 和 `TimerQueue` 等组件协同工作，实现了高效的事件驱动和定时器管理。



```cpp
void PollerKqueue::loop_once(int waitMs) {
    // 设置等待时间
    struct timespec timeout;
    timeout.tv_sec = waitMs / 1000;
    timeout.tv_nsec = (waitMs % 1000) * 1000 * 1000;
    // 记录开始时间
    long ticks = util::timeMilli();
    // 调用 kevent 函数等待事件
    lastActive_ = kevent(fd_, NULL, 0, activeEvs_, kMaxEvents, &timeout);
    // 输出调试信息
    trace("kevent wait %d return %d errno %d used %lld millsecond", waitMs, lastActive_, errno, util::timeMilli() - ticks);
    // 如果 kevent 函数返回错误，且错误码不是 EINTR，则输出错误信息
    fatalif(lastActive_ == -1 && errno != EINTR, "kevent return error %d %s", errno, strerror(errno));
    // 遍历所有事件，处理每个事件
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        Channel *ch = (Channel *) activeEvs_[i].udata;
        struct kevent &ke = activeEvs_[i];
        if (ch) {
            // 如果事件是可写事件，且读写都已启用，则处理写事件
            if (!(ke.flags & EV_EOF) && ch->writeEnabled()) {
                trace("channel %lld fd %d handle write", (long long) ch->id(), ch->fd());
                ch->handleWrite();
            // 如果事件是可读事件，或者已经发生了 EOF，则处理读事件
            } else if ((ke.flags & EV_EOF) || ch->readEnabled()) {
                trace("channel %lld fd %d handle read", (long long) ch->id(), ch->fd());
                ch->handleRead();
            // 如果事件类型不是可读可写事件，则输出错误信息
            } else {
                fatal("unexpected epoll events %d", ch->events());
            }
        }
    }
}
```

GitHub Copilot: 下面是对 `PollerKqueue::loop_once` 函数的代码注释和详细解释：

```cpp
void PollerKqueue::loop_once(int waitMs) {
    // 设置等待时间
    struct timespec timeout;
    timeout.tv_sec = waitMs / 1000;
    timeout.tv_nsec = (waitMs % 1000) * 1000 * 1000;
    // 记录开始时间
    long ticks = util::timeMilli();
    // 调用 kevent 函数等待事件
    lastActive_ = kevent(fd_, NULL, 0, activeEvs_, kMaxEvents, &timeout);
    // 输出调试信息
    trace("kevent wait %d return %d errno %d used %lld millsecond", waitMs, lastActive_, errno, util::timeMilli() - ticks);
    // 如果 kevent 函数返回错误，且错误码不是 EINTR，则输出错误信息
    fatalif(lastActive_ == -1 && errno != EINTR, "kevent return error %d %s", errno, strerror(errno));
    // 遍历所有事件，处理每个事件
    while (--lastActive_ >= 0) {
        int i = lastActive_;
        Channel *ch = (Channel *) activeEvs_[i].udata;
        struct kevent &ke = activeEvs_[i];
        if (ch) {
            // 如果事件是可写事件，且读写都已启用，则处理写事件
            if (!(ke.flags & EV_EOF) && ch->writeEnabled()) {
                trace("channel %lld fd %d handle write", (long long) ch->id(), ch->fd());
                ch->handleWrite();
            // 如果事件是可读事件，或者已经发生了 EOF，则处理读事件
            } else if ((ke.flags & EV_EOF) || ch->readEnabled()) {
                trace("channel %lld fd %d handle read", (long long) ch->id(), ch->fd());
                ch->handleRead();
            // 如果事件类型不是可读可写事件，则输出错误信息
            } else {
                fatal("unexpected epoll events %d", ch->events());
            }
        }
    }
}
```

该函数的作用是在 `PollerKqueue` 类中实现事件循环，等待事件的到来并处理事件。具体来说，该函数的实现过程如下：

1. 设置等待时间，等待时间到达后会返回。
2. 调用 `kevent` 函数等待事件，将事件存储到 `activeEvs_` 数组中。
3. 遍历 `activeEvs_` 数组中的所有事件，根据事件类型进行处理。
4. 如果事件是可写事件，且读写都已启用，则处理写事件。
5. 如果事件是可读事件，或者已经发生了 EOF，则处理读事件。
6. 如果事件类型不是可读可写事件，则输出错误信息。

需要注意的是，`kevent` 函数是一个阻塞函数，会一直等待事件的到来，直到等待时间到达或者有事件到来才会返回。在处理事件时，需要根据事件类型进行判断，如果是可读事件则调用 `handleRead` 函数处理读事件，如果是可写事件则调用 `handleWrite` 函数处理写事件。同时，需要注意处理事件的顺序，先处理可写事件再处理可读事件，避免出现写事件一直阻塞读事件的情况。

### kevent 函数



## 前言
C++ 是一门经典的编程语言。将它按一定规则构建成术式，就可以通过编译器作为媒介让其生效，产生出对象物体或者其他结构体，并能凭此为元素赋予人一些其本身无法做到的能力。
### 篇一
> 总结一些 c++ 基本术式和一些理解难点

