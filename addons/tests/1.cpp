#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

std::mutex mut;
int counter = 0;
std::condition_variable cond_var;

void Process() {
    int c;
#if defined(OOK)                                                                         //
    std::cout << "OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK!" << std::endl;  //
    std::cout << "OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK!" << std::endl;  //
    std::cout << "OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK!" << std::endl;  //
    std::cout << "OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK!" << std::endl;  //
    std::cout << "OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK! OOK!" << std::endl;  //
#endif                                                                                   //
    {
        std::unique_lock<std::mutex> ul(mut);
        cond_var.wait(ul, [] { return counter > 0; });
        c = counter;
    }
    std::cout << std::this_thread::get_id() << "process complete" << std::endl;
}

void Preparation() {
    {
        std::lock_guard<std::mutex> lg(mut);
        counter += 1;
        cond_var.notify_one();
    }
    std::cout << std::this_thread::get_id() << "preparation complete" << std::endl;
}

}  // namespace

int main() {
    std::thread t2(Process);
    std::thread t3(Process);
    std::thread t1(Preparation);
    t1.join();
    t2.join();
    t3.join();
    std::cout << "that's all, folks!" << std::endl;
    return 0;
}
