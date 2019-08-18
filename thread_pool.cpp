#include <thread>
#include <iostream>
#include <queue>
#include <string>
#include <mutex>        // std::mutex, std::lock_guard
#include <atomic>       // std::atomic_bool
#include <condition_variable> // std::condition_variable
#include <chrono>


void wait_for()
{
    std::cout<<"Going to sleep....\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout<<"Waking up...\n";
}

class Task
{
    unsigned m_number;
    bool m_completed;

  public:
    Task(unsigned task_number): m_number(task_number), m_completed(0) {}
};


class SafeQueue
{
    std::queue <Task> tasksQueue;
    std::mutex m_mutex;

  public:
    Task getTask()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return tasksQueue.front();
    }
    void pushTask(unsigned taskNumber)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        tasksQueue.push(Task(taskNumber));
    }

    bool check_if_empty()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return tasksQueue.empty();
    }
};

class TasksManager
{
    std::atomic<bool> isStopped;
    std::mutex m_mutex;
    std::condition_variable cond_var;
    int a=0;    // shared variable
    std::vector <std::thread> threadsSet;
    SafeQueue m_safeQueue;

    void addTask(unsigned taskNumber)
    {
        m_safeQueue.pushTask(taskNumber);
        cond_var.notify_one();
    }
    void disableThreadpool()
    {
        isStopped=true;
        cond_var.notify_all();
        for (auto &th : threadsSet) th.join(); // synchronising after setting flag as true
    }

  public:
    TasksManager()
    {
        isStopped = false;
        for (unsigned i=0; i<10; i++)
        {
            std::thread t1(wait_for);
            threadsSet.push_back(std::move(t1));  // push back the thread
        }
        for (unsigned i=0; i<200; i++)
        {
            addTask(i);  // push back the thread
        }
    }
    void runManager()
    {
        while(!isStopped)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            std::cout<<"HERE\n";
            //cond_var.wait(m_safeQueue.check_if_empty()); // if queue is empty
            disableThreadpool();
            m_safeQueue.getTask();
        }
        std::cout<<"Unlocked\n";
    }
};


int main()
{
    TasksManager manager;
    manager.runManager();
    return 0;
}



// concurent queue
// mutexes around addTask and popping task from queue // common mutex
