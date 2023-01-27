#include "FileManager.h"
std::shared_ptr<FileManager> FileManager::fileManager_ = nullptr;
#define NUM 2
FileManager::FileManager() {
    for (int i = 0; i < NUM; i++) {
        threads_.emplace_back([this] {
            prctl(PR_SET_NAME, "FileManager");

            while (1) {
                File_JobFunction             func;
                std::unique_lock<std::mutex> lock(File_lock_);

                while (Job_queues_.empty() && !stop_) {
                    File_conn_.wait(lock);
                }

                if (Job_queues_.empty() && stop_) {
                    return;
                }

                func = Job_queues_.front();
                Job_queues_.pop();

                if (func) {
                    func();
                }
            }
        });
    }
}

FileManager::~FileManager() {
    {
        std::unique_lock<std::mutex> lock(File_lock_);
        stop_ = true;
    }
    File_conn_.notify_all();  // 唤醒等待该条件的所有线程，若没有什么也不做
    for (auto& thread : threads_) {
        thread.join();  // 让主控线程等待（阻塞）子线程退出
    }
    printf("[FileManager::~FileManager] FileManager is remove\n");
}

void FileManager::File_pushJob(const File_JobFunction& job) {
    {
        std::unique_lock<std::mutex> lock(File_lock_);
        Job_queues_.push(job);
    }
    File_conn_.notify_one();
}
