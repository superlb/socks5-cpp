#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>


class ThreadPool {
 private:
  typedef std::function<void()> task;
  std::deque<task> task_list;
  std::vector<std::thread*> thread_list;
  std::mutex task_mutex;
  std::condition_variable task_con;
  bool runningStatus;
  void thread_wait();
  ThreadPool::task get_task();

 public:
  ThreadPool();
  void add_task(const task& t);
  ~ThreadPool();
};