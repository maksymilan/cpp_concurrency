#include <iostream>
#include <chrono>
#include <string>

class timer{
    private:
        std::string name;
        std::chrono::time_point<std::chrono::steady_clock> start_point;
    
    public:
    timer(const std::string & name_ = ""):name(name_),start_point(std::chrono::steady_clock::now()){}
    ~timer(){
        auto end_point = std::chrono::steady_clock::now();
        auto duration = end_point - start_point;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        if(!name.empty()){
            std::cout<<"["<<name<<"] ";
        }
        std::cout<<"Elapsed time: "<<ms.count()<<" ms"<<std::endl;
    }
};