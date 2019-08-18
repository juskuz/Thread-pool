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
    Task(unsigned task_number): m_number(task_number), m_completed(0) {std::cout<<m_number<<std::endl;}
    void setCompleted()
    {
        m_completed = 1;
    }
    unsigned getTaskNumber()
    {
        return m_number;
    }
    void runTask()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};


class SafeQueue
{
    std::queue <Task> tasksQueue;
    std::mutex m_mutex;

  public:
    Task getTask()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        Task poppedTask = tasksQueue.front();
        tasksQueue.pop();
        return poppedTask;
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
	void taskCompleted(Task task)
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::cout<<"Task "<<task.getTaskNumber()<<" completed.\n";
        task.setCompleted();
        tasksQueue.push(task);
    }
};

class TasksManager
{
    std::atomic<bool> isStopped;
    std::mutex m_mutex;
    std::condition_variable cond_var;
    std::vector <std::thread> threadsSet;
	SafeQueue m_InputQueue;
    SafeQueue m_OutputQueue;
	
    void addTask(unsigned taskNumber)
    {
        m_InputQueue.pushTask(taskNumber);
        cond_var.notify_one();
    }
    void disableThreadpool()
    {
        isStopped=true;
        cond_var.notify_all();
        //for (auto &th : threadsSet) th.join(); // synchronising after setting flag as true
    }

  public:
    TasksManager()
    {
        isStopped = false;
        for (unsigned i=0; i<5; i++)
        {
            std::thread t1(wait_for);
            threadsSet.push_back(std::move(t1));  // push back the thread
        }
        for (unsigned i=0; i<50; i++)
        {
            addTask(i); 
        }
    }
    void runManager()
    {
        std::cout<<"Running tasks..."<<std::endl;
        while(!isStopped || m_InputQueue.check_if_empty())
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_InputQueue.check_if_empty())
            {
                std::cout<<"No tasks... Going to sleep...\n";
                cond_var.wait(lock); // wait if queue is empty
            }
            else
            {
            Task runningTask = m_InputQueue.getTask();
            runningTask.runTask();
            m_OutputQueue.taskCompleted(runningTask);  // it can be also m_InputQueue
            }
            //if (isStopped)
            {
                disableThreadpool();
            }
        }
        for (auto &th : threadsSet) th.join(); // synchronising after setting flag as true

        std::cout<<"End.\n";
    }
};


int main()
{
    TasksManager manager;
    manager.runManager();
    return 0;
}
