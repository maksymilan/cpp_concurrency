#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <thread>
#include <format> // c++20
#include <barrier> // c++20 for synchronization
#include <chrono>
#include "utils/timer.h"

class DNSEntity{
    private:
        std::string domain;
        std::vector<int> ip;
    public:
        DNSEntity(){}
        DNSEntity(std::string domain_,std::vector<int> ip_):domain(domain_),ip(ip_){}
        std::string get_domain() const {
            return domain;
        }
        std::string get_ip() const {
            if(ip.size() == 4){
                return std::format("{}.{}.{}.{}",ip[0],ip[1],ip[2],ip[3]);
            }else{
                return "0.0.0.0";
            }
        }
        void update_ip(std::vector<int> new_ip){
            ip = std::move(new_ip);
        }
};

class DNSCache{
    private:
        std::map<std::string,DNSEntity> DNS;
        std::shared_mutex shared_m;
        std::mutex m;
        std::string read_(const std::string& domain){
            auto it = DNS.find(domain);
            return it != DNS.end() ? it->second.get_ip() : DNSEntity().get_ip(); 
        }
        void update_(const std::string& domain, const std::vector<int>& ip){
            if(ip.size() != 4){
                std::cout<<"error ip addr\n";
                return;
            }
            auto it = DNS.find(domain);
            if(it != DNS.end()){
                it->second.update_ip(ip);
            }else{
                auto res = DNS.emplace(domain,DNSEntity(domain,ip));
                if(!res.second){
                    std::cout<<"update failed\n";
                }
            }
        }

    public:
        DNSCache(){};
        std::string exclusive_read(const std::string& domain){
            std::lock_guard<std::mutex> guard(m);
            return read_(domain);
        }
        void update_mutex(const std::string& domain, const std::vector<int>& ip){
            std::lock_guard<std::mutex> guard(m);
            update_(domain,ip);
        }

        std::string shared_read(const std::string& domain){
            std::shared_lock<std::shared_mutex> shared_guard(shared_m);
            return read_(domain);
        }
        void update_shared(const std::string& domain, const std::vector<int>& ip){
            std::lock_guard<std::shared_mutex> guard(shared_m);
            update_(domain,ip);
        }
};

void read_k_times(DNSCache& cache, int k, std::barrier<>& start_barrier, bool shared){
    start_barrier.arrive_and_wait();
    for(int i=0;i<k;++i){
        if(shared){
            cache.shared_read(std::to_string(i%100));
        }else{
            cache.exclusive_read(std::to_string(i%100));
        }
    }
}

int main(){
    DNSCache DNS_mutex;
    DNSCache DNS_shared;
    std::vector<int> ip = {127,0,0,1};
    for(int i=0;i<100;++i){
        DNS_mutex.update_mutex(std::to_string(i), ip);
        DNS_shared.update_shared(std::to_string(i), ip);
    }
    std::vector<std::thread> mutex_threads;
    std::vector<std::thread> shared_threads;
    int num_threds = 100;
    // test for mutex read
    {
        std::barrier mutex_barrier(num_threds);
        auto time = timer("mutex read DNS");
        for(int i=0;i<num_threds;++i){
            mutex_threads.emplace_back(read_k_times,std::ref(DNS_mutex),1000,std::ref(mutex_barrier),false);
        }
        for(auto& t:mutex_threads){
            t.join();
        }
    }
    // test for shared read
    {
        std::barrier shared_barrier(num_threds);
        auto time = timer("shared read DNS");
        for(int i=0;i<num_threds;++i){
            shared_threads.emplace_back(read_k_times,std::ref(DNS_shared),1000,std::ref(shared_barrier),true);
        }
        for(auto& t:shared_threads){
            t.join();
        }
    }
    return 0;
}