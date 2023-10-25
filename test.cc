#include <iostream>
#include <chrono>
using namespace std;

#include <iostream>
#include <chrono>
using namespace std;

int64_t timeMicro() {
    chrono::time_point<chrono::system_clock> p = chrono::system_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}

string timeClock() {
    // 获取当前时间点
    auto p = std::chrono::system_clock::now();
    // 将时间点转换为 time_t 类型的时间
    auto t = std::chrono::system_clock::to_time_t(p);
    // 将 time_t 类型的时间转换为本地时间
    auto tm = std::localtime(&t);
    // 格式化本地时间为字符串
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

int main(){
    int64_t t1 = timeMicro();
    string s1 = timeClock();
    cout << s1 << endl;
    return 0;
}