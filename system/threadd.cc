#include <iostream>
#include <vector>
#include <thread>


using namespace std;
mutex mtx;

void work(int i){
    // mtx.lock(); 加锁后，就不会出现同时占用标准输出的情况
    cout<<"this work is ..."<<this_thread::get_id()<< "args:"<<i<<endl;
    // mtx.unlock();
}


int main(){
    vector<thread> works;
    for(int i=0; i<=14; i++){
        works.push_back(thread(work, i));
    }
    for(int i=0; i<=14; i++){
        works[i].join();
    }
    cout<<"this is the main Thread"<<endl;
    return 1;
}