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
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
	    threads.push_back(std::thread([=]{
				    for (int j = i; j < num_total_tasks; j += num_threads) {
				    runnable->runTask(j, num_total_tasks);
				    }
				    }));
    }
    for (auto i = threads.begin(); i != threads.end(); i++) {
	    i->join();
    }

}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->works = 0;
    this->m = new std::mutex;
    this->started = true;
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(std::thread([=](){
                    while (this->started) {
                    this->m->lock();
                    while (this->works > 0) {
                    int work = this->works - 1;
                    this->works--;
                    this->m->unlock();
                    this->runnable->runTask(work, this->total);
                    this->m->lock();
                    this->done++;
                    }
                    this->m->unlock();
                    }
                    }));
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    this->started = false;
    for (auto i = threads.begin(); i != threads.end(); ++i) {
        i->join();
    }
    delete this->m;
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    this->m->lock();
    this->works = num_total_tasks;
    this->total = num_total_tasks;
    this->runnable = runnable;
    this->done = 0;
    this->m->unlock();

    this->m->lock();
    while (this->done != num_total_tasks) {
        this->m->unlock();
        this->m->lock();
    }
    this->m->unlock();
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
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
    this->started = true;
    this->total = 0;
    this->num_threads = num_threads;
    
    for (int i = 0; i < num_threads; i++) {
	done.push_back(false);
        threads.push_back(std::thread([=](){
                    std::unique_lock<std::mutex> lck(m1);
                    while (started) {
		    	// make sure threads created
		    	{
			std::lock_guard<std::mutex> lck2(m2);
		    	done[i] = true;
		    	cond_main.notify_one();
			}
			int total;
			IRunnable *runnable;
                        {
                        cond_worker.wait(lck);
			total = this->total;
			runnable = this->runnable;
                        }
			int work = total / num_threads;
			for (int j = work * i; (i == num_threads - 1 || j < work * (i + 1)) && j < total; j++) {
			runnable->runTask(j, total);
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
    this->started = false;
    this->total = 0;
    cond_worker.notify_all();
    for (auto i = threads.begin(); i != threads.end(); ++i) {
        i->join();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

#if 0
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
#endif
    // get threads ready to be notified
    {
    std::lock_guard<std::mutex> lck1(m1);
    this->total = 0;
    }
    std::unique_lock<std::mutex> lck(m2);

    {
    cond_main.wait(lck, [=]{
		    bool result = true;
		    for (int i = 0; i < num_threads; i++) {
		    	result = result && done[i];
		    }
		    return result;
		    });
    std::lock_guard<std::mutex> lck1(m1);
    this->total = num_total_tasks;
    this->runnable = runnable;
    for (int i = 0; i < num_threads; i++) {
	    done[i] = false;
    }
    }
    
    cond_worker.notify_all();
    {
    cond_main.wait(lck, [=]{
		    bool result = true;
		    for (int i = 0; i < num_threads; i++) {
		    	result = result && done[i];
		    }
		    return result;
		    });
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
