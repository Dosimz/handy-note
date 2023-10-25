#include "codec.h"

using namespace std;

namespace handy {

// 尝试解码数据，返回解码后的消息和解码后剩余的数据长度
int LineCodec::tryDecode(Slice data, Slice &msg) {
    // 如果数据长度为 1，且数据为 0x04，则将数据作为消息返回
    if (data.size() == 1 && data[0] == 0x04) {
        msg = data;
        return 1;
    }
    // 遍历数据，查找换行符
    for (size_t i = 0; i < data.size(); i++) {
        // 如果找到了换行符
        if (data[i] == '\n') {
            // 如果前一个字符是回车符，则将回车符和换行符之前的数据作为消息返回
            if (i > 0 && data[i - 1] == '\r') {
                msg = Slice(data.data(), i - 1);
                return static_cast<int>(i + 1);
            } else {  // 否则将换行符之前的数据作为消息返回
                msg = Slice(data.data(), i);
                return static_cast<int>(i + 1); 
            }
        }
    }
    return 0;
}

// 将消息编码为带有回车换行符的字符串
void LineCodec::encode(Slice msg, Buffer &buf) {
    buf.append(msg).append("\r\n");
}

// 尝试解码数据，返回解码后的消息和解码后剩余的数据长度
int LengthCodec::tryDecode(Slice data, Slice &msg) {
    // 如果数据长度小于 8，则返回 0
    if (data.size() < 8) {
        return 0;
    }
    // 从数据中读取消息长度
    int len = net::ntoh(*(int32_t *) (data.data() + 4));
    // 如果消息长度超过 1MB，或者消息头不是 "mBdT"，则返回 -1
    if (len > 1024 * 1024 || memcmp(data.data(), "mBdT", 4) != 0) {
        return -1;
    }
    // 如果数据长度大于等于消息长度加上 8，则将消息返回
    if ((int) data.size() >= len + 8) {
        msg = Slice(data.data() + 8, len);
        return len + 8;
    }
    return 0;
}

// 将消息编码为带有长度信息的字符串
void LengthCodec::encode(Slice msg, Buffer &buf) {
    buf.append("mBdT").appendValue(net::hton((int32_t) msg.size())).append(msg);
}

}  // namespace handy