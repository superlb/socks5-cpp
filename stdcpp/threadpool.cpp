#include "threadpool.h"

ThreadPool::ThreadPool() {
  int thread_size = 10;
  runningStatus = true;
  thread_list.reserve(thread_size);
  for (int i = 0; i < thread_size; ++i) {
    thread_list.push_back(
        new std::thread(std::bind(&ThreadPool::thread_wait, this)));
  }
}

ThreadPool::~ThreadPool() {
  std::unique_lock<std::mutex> lock(task_mutex);
  runningStatus = false;
  task_con.notify_all();
  for (auto& a : thread_list) {
    a->join();
    delete a;
  }
}

void ThreadPool::thread_wait() {
  while (runningStatus) {
    task t = get_task();
    if (t) t();
  }
}

ThreadPool::task ThreadPool::get_task() {
  std::unique_lock<std::mutex> lock(task_mutex);
  while (task_list.empty() && runningStatus) {
    task_con.wait(lock);
  }
  task t;
  if (!task_list.empty() && runningStatus) {
    t = task_list.front();
    task_list.pop_front();
  }
  return t;
}

void ThreadPool::add_task(const task& t) {
  task_list.push_back(t);
  task_con.notify_one();
}