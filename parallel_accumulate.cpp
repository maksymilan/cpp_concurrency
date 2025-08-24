#include <thread>
#include <vector>
#include <iostream>
#include <algorithm>

template <typename Iterator, typename T>
void accumulate_block(Iterator start, Iterator end, T& result){
    for(auto itr = start;itr != end; ++itr){
        result += *itr;
    }
}

template <typename Iterator,typename T>
void accumulate(Iterator start, Iterator end, T& init){
    // 确定需要的线程数量
    unsigned int size_per_thread = 25;
    unsigned int total_size = std::distance(start,end);
    if(!total_size)
        return;
    unsigned int max_thread_num = (total_size + size_per_thread - 1) / size_per_thread;
    unsigned int thread_num = std::min(std::thread::hardware_concurrency()!=0 ? std::thread::hardware_concurrency():2, max_thread_num);
    unsigned int block_size = total_size / thread_num;
    // 分配线程任务
    std::vector<std::thread> workers(thread_num);
    std::vector<T> results(thread_num);
    for(int i=0;i<thread_num-1;++i){
        auto task_end = start;
        std::advance(task_end,block_size);
        workers[i] = std::thread(accumulate_block<Iterator,T>,start,task_end,std::ref(results[i]));
        start = task_end;
    }
    workers[thread_num-1] = std::thread(accumulate_block<Iterator,T>,start,end,std::ref(results[thread_num-1]));
    for(auto& w:workers){
        w.join();
    }
    accumulate_block(results.begin(),results.end(),init);
}

int main(){
    std::vector<int> i_sum(100,1);
    std::vector<double> d_sum(100,1.0);
    int i_res = 0;
    accumulate<std::vector<int>::iterator,int>(i_sum.begin(),i_sum.end(),i_res);
    double d_res = 0;
    accumulate<std::vector<double>::iterator,double>(d_sum.begin(), d_sum.end(), d_res);
    std::cout<<i_res <<" "<<d_res<<std::endl;
    return 0;
}