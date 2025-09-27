#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <chrono>
#include <functional>

using Task = std::function<void()>;
using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::milliseconds;

class ThreadPool{
    private:
        size_t size_;
        std::vector<std::thread> pool_; // 添加工作线程
        std::queue<Task> tasks_;
        std::atomic<bool> stop_;
        std::mutex mtx_;
        std::condition_variable cv_;
        void WorkerLoop(){
            while(true){
                Task task;
                {
                    std::unique_lock lk(mtx_);
                    cv_.wait(lk,[this](){return stop_ || !tasks_.empty();});
                    if(stop_&&!tasks_.empty()){
                        return;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                if(task){
                    task();
                }else{
                    throw std::runtime_error("call empty function object");
                }
            }
        }
    public:
        ThreadPool(size_t size):size_(size),stop_(false){
            for(size_t i=0;i<size_;++i){
                pool_.emplace_back(&ThreadPool::WorkerLoop,this);
            }
        }
        ~ThreadPool(){
            {
                std::unique_lock lk(mtx_);
                stop_ = true;
            }
            cv_.notify_all();
            for(auto& t:pool_){
                if(t.joinable()){
                    t.join();
                }
            }
        }
        void PushTask(Task t){
            std::unique_lock lk(mtx_);
            if(stop_){
                throw std::runtime_error("put task after pool deconstruct");
            }
            tasks_.push(std::move(t));
            cv_.notify_one();
        }
};  

class TimerThreadScheduler{
    private:
    struct ScheduleTask
    {
        TimePoint excute_at;
        Task task;
        bool operator>(const ScheduleTask& other) const {
            return this->excute_at > other.excute_at;
        }
    };
        std::mutex mtx_;
        std::condition_variable cv_;
        std::atomic<bool> stop_;
        std::priority_queue<ScheduleTask,std::vector<ScheduleTask>,std::greater<ScheduleTask>> run_q_;
        ThreadPool worker_pool_;
        std::thread worker_;
        
    public:
        void AddTask(Task t, Duration delay){
            std::unique_lock lk(mtx_);
            TimePoint ex_time = std::chrono::steady_clock::now() + delay;
            run_q_.push({ex_time,t});
            cv_.notify_one();
        }
        void WorkerThread(){
            while (!stop_)
            {
                std::vector<Task> tasks;
                {
                    std::unique_lock lk(mtx_);
                    if(run_q_.empty()){
                        cv_.wait(lk,[this](){return stop_ || !run_q_.empty();});
                    }else{
                        TimePoint nxt_ex = run_q_.top().excute_at;
                        cv_.wait_until(lk,nxt_ex);
                    }
                    if(stop_){
                        return;
                    }
                    TimePoint now = std::chrono::steady_clock::now();
                    while(!run_q_.empty() && run_q_.top().excute_at <= now){
                        tasks.emplace_back(std::move(run_q_.top().task));
                        run_q_.pop();
                    }
                    for(auto& t:tasks){
                        worker_pool_.PushTask(std::move(t));
                    }
                }
            }
        }
        TimerThreadScheduler(size_t pool_size):stop_(false),worker_pool_(pool_size){
            worker_ = std::thread(&TimerThreadScheduler::WorkerThread,this);
        }
        ~TimerThreadScheduler(){
            {
                std::unique_lock lk(mtx_);
                stop_ = true;
            }
            cv_.notify_one();
            if(worker_.joinable()){
                worker_.join();
            }
        }
};

// 用于日志打印的辅助函数
long long get_current_ms() {
    static const auto start_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();
}

int main() {
    using namespace std::chrono_literals;

    std::cout << "定时器调度器最终测试..." << std::endl;
    std::cout << "当前时间戳 (ms): " << get_current_ms() << std::endl;

    // 创建一个调度器实例
    TimerThreadScheduler scheduler(4);

    // 添加任务1：一个快速任务
    std::cout << "添加任务A，延迟 800ms" << std::endl;
    scheduler.AddTask([]() {
        std::cout << "--> [" << get_current_ms() << "ms] 任务A执行 (线程ID: "
                  << std::this_thread::get_id() << ")" << std::endl;
    }, 800ms);

    // 添加任务2和3：两个应该同时开始的并发任务
    std::cout << "添加任务B，延迟 1500ms" << std::endl;
    scheduler.AddTask([]() {
        std::cout << "--> [" << get_current_ms() << "ms] 任务B执行 (线程ID: "
                  << std::this_thread::get_id() << ")" << std::endl;
    }, 1500ms);

    std::cout << "添加任务C，延迟 1500ms" << std::endl;
    scheduler.AddTask([]() {
        std::cout << "--> [" << get_current_ms() << "ms] 任务C执行 (线程ID: "
                  << std::this_thread::get_id() << ")" << std::endl;
    }, 1500ms);
    
    // 添加任务4：一个耗时任务
    std::cout << "添加任务D，延迟 500ms (此任务耗时1秒)" << std::endl;
    scheduler.AddTask([]() {
        std::cout << "--> [" << get_current_ms() << "ms] 任务D 开始 (线程ID: "
                  << std::this_thread::get_id() << ")" << std::endl;
        std::this_thread::sleep_for(1000ms);
        std::cout << "--> [" << get_current_ms() << "ms] 任务D 结束" << std::endl;
    }, 500ms);

    // 让主线程保持运行，等待所有任务被调度
    std::cout << "主线程将休眠 3 秒..." << std::endl;
    std::this_thread::sleep_for(3000ms);

    std::cout << "主线程结束。调度器将析构，并关闭所有线程。" << std::endl;

    return 0;
}