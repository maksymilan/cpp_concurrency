#include <list>
#include <mutex>
#include <algorithm>
#include <thread>
#include <iostream>

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int value){
    std::lock_guard<std::mutex> guard(some_mutex);
    std::cout<<"add to list: "<< value << std::endl;
    some_list.push_back(value);
}

bool list_contains(int value){
    std::lock_guard<std::mutex> guard(some_mutex);
    bool has = std::find(some_list.begin(),some_list.end(),value) != some_list.end();
    std::cout<< "list has value " << value << ": " << has << std::endl;
    return has;
}

int main(){
    std::vector<std::thread> writers;
    std::vector<std::thread> readers;
    for(int i=0;i<100;++i){
        writers.emplace_back(std::thread(add_to_list,i));
        readers.emplace_back(std::thread(list_contains,i));
    }
    for(int i=0;i<100;++i){
        writers[i].join();
        readers[i].join();
    }
    return 0;
}