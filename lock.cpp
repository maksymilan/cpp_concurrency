#include <mutex>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

class some_obj{
    private:
        std::string name_;
    public:
        some_obj(){name_ = "NULL";};
        some_obj(const std::string& name){
            name_ = name;
        }
        std::string get_name(){
            return name_;
        }
        friend void swap(some_obj& obj1,some_obj& obj2){
            std::string obj1_name = obj1.name_;
            obj1.name_ = obj2.name_;
            obj2.name_ = obj1_name;
        }
};


class X{
    private:
        some_obj obj_;
        std::mutex m_;
    public:
        X(some_obj obj){
            obj_ = std::move(obj);
        }
        friend void swap_with_mutex(X& obj1, X& obj2);
        friend void swap_with_lock(X& obj1, X& obj2);
};

void swap_with_mutex(X& obj1, X& obj2){
    std::lock_guard<std::mutex> obj1_guard(obj1.m_);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> obj2_guard(obj2.m_);
    swap(obj1.obj_,obj2.obj_);
}
void swap_with_lock(X& obj1, X& obj2){
    std::lock(obj1.m_,obj2.m_);
    std::lock_guard<std::mutex> obj1_guard(obj1.m_, std::adopt_lock);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> obj2_guard(obj2.m_, std::adopt_lock);
    swap(obj1.obj_,obj2.obj_);
}

template <typename Func>
void run_test(Func function, X& x1, X& x2){
    std::vector<std::thread> threads;
    for(int i=0;i<10;++i){
        if(i%2 == 0){
            threads.emplace_back(function,std::ref(x1),std::ref(x2));
        }else{
            threads.emplace_back(function,std::ref(x2),std::ref(x1));
        }
    }
    for(auto& t:threads){
        t.join();
    }

}

int main(){
    some_obj ob1("this is the first obj");
    some_obj ob2("this is the second obj");
    X x1(ob1);
    X x2(ob2);
    // --------- mutex lock test <--- will trigger a deadlock
    run_test(swap_with_mutex, x1, x2);
    // ---------- end mutex lock test

    // ---------- lock test
    run_test(swap_with_lock, x1, x2);
    // ---------- end lock test
    std::cout<<"after swap:\n" << "obj1 name:\n"<< ob1.get_name() << "\n" << "obj2 name:\n"<<ob2.get_name()<<std::endl;
    return 0;
}