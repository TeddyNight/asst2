#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <vector>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <algorithm>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

typedef struct Task {
	IRunnable *runnable;
	int num_total_tasks;
	TaskID id;
	std::vector<TaskID> depends;
} Task;

// tasks added to list sequentially
// assuming that: 
// 1. all tasks added later depends only on the task previously added
// 2. accesses to this list are serialized
// TODO: if there's need to wait when task waiting & notify threads waiting for tasks to be done
class TaskList {
	private:
		std::vector<Task> tasks;
		std::atomic<size_t>* threads_index;
		std::mutex m1;
		std::condition_variable cond_empty;
		int num_threads;
		bool terminated;
	public:
		TaskList(int num_threads) {
			this->num_threads = num_threads;
			this->threads_index = new std::atomic<size_t>[num_threads];
			for (int i = 0; i < num_threads; i++) {
				threads_index[i] = 0;
			}
			terminated = false;
		};
		void set_terminated() {
			terminated = true;
			cond_empty.notify_all();
		};
		void push_back(Task& task) {
			std::unique_lock<std::mutex> lck(m1);
			tasks.push_back(task);
			cond_empty.notify_all();
		};
		bool is_terminated() {
			return terminated;
		}
		bool is_empty(int thread) {
			std::unique_lock<std::mutex> lck(m1);
			return threads_index[thread] >= tasks.size();
		}
		void wait() {
			std::unique_lock<std::mutex> lck(m1);
			cond_empty.wait(lck);
		}
		// must check empty first
		bool is_ready(std::vector<TaskID> &deps) {
			for (size_t i = 0; i < deps.size(); i++) {
				if (!is_done(deps[i])) return false;
			}
			return true;
		};
		Task front(int thread) {
			std::unique_lock<std::mutex> lck(m1);
			return tasks[threads_index[thread]];
		};
		void pop_front(int thread) {
			threads_index[thread]++;
			cond_empty.notify_all();
		};
		bool is_done(size_t taskID) {
			for (int i = 0; i < num_threads; i++) {
				if (threads_index[i] <= taskID) {
					return false;
				}
			}
			return true;
		};
		void wait_threads_done() {
			std::unique_lock<std::mutex> lck(m1);
			cond_empty.wait(lck, [=]{
						for (int i = 0; i < num_threads; i++) {
							if (threads_index[i] < tasks.size()) return false;
						}
						return true;
					});
		}
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
	std::vector<std::thread> threads;
	int num_threads;
	TaskList *task_list;
	int count;
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
