#include "tasksys.h"
#include <stdio.h>


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
    this->started = true;
    this->count = 0;
    this->created = 0;
    for (int i = 0; i < num_threads; i++) {
       threads.push_back(std::thread([=]{
			       while (true) {
			       // join point: thread ready
			       {
			       	  std::unique_lock<std::mutex> lck(m2);
				  created++;
				  if (created == num_threads) {
					  done = 0;
					  created = 0;
					  ready_worker.wait(lck, [=]{ return !(started && ready.empty());  });
					  cond_worker.notify_all();
					  //printf("thread %d wake up all\n", i);
				  }
				  else {
				  	//printf("thread %d waiting...\n", i);
					// avoid waiting forever when notify task executed before it
				  	if (started) cond_worker.wait(lck);
					//printf("thread %d woken up\n", i);
				  }
				  if (!started) break;
			       }
			       if (!ready.empty()) {
				       Task &t = ready.front();
				       //printf("thread %d executing task %d\n", i, t.id);
				       for (int j = i; j < t.num_total_tasks; j += num_threads) {
					       t.runnable->runTask(j, t.num_total_tasks);
				       }
			       }
			       
			       {
			       std::unique_lock<std::mutex> lck(m2);
			       done++;
			       //printf("done %d\n", done);
			       if (done == num_threads) {
				        if (!ready.empty()) {
				        	finish.push_back(ready.front().id);
						ready.pop();
					}
				       	std::queue<Task> pending;
					while (!waiting.empty()) {
						Task &t = waiting.front();
						// TODO: is t.depends subset of finish?
						unsigned int finish_cnt = 0;
						for (auto k = t.depends.begin(); k != t.depends.end(); ++k) {
							auto it = std::find(finish.begin(), finish.end(), *k);
							if (it != finish.end()) finish_cnt++;

						}
						if (finish_cnt == t.depends.size()) {
							ready.push(t);
						}
						else {
							pending.push(t);
						}
						waiting.pop();
					}
					while (!pending.empty()) {
						waiting.push(pending.front());
						pending.pop();
					}
			       		if (ready.empty() && waiting.empty()) main_done.notify_all();
			       }
			       }
			       }
			}));
    } 
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    {
    std::unique_lock<std::mutex> lck(m2);
    started = false;
    ready_worker.notify_all();
    }
    for (int i = 0; i < num_threads; i++) {
	    threads[i].join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    int task_id;

    // TODO: if depends on done task?
    {
    std::unique_lock<std::mutex> lck(m2);
    task_id = count++;
    Task t = {
		    .runnable = runnable,
		    .num_total_tasks = num_total_tasks,
		    .id = task_id,
		    .depends = deps
		    };
    if (deps.empty()) {
	    ready.push(t);
    }
    else {
	    waiting.push(t);
    }
    ready_worker.notify_all();
    }

    return task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    std::unique_lock<std::mutex> lck(m2);
    if (!(ready.empty() && waiting.empty())) main_done.wait(lck);
}
