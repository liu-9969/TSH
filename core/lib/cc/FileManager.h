#ifndef _FILE_MANAGER_
#define _FILE_MANAGER_
#include <queue>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <sys/prctl.h>

using File_JobFunction = std::function<void()>;

class FileManager {
public:
    static std::shared_ptr<FileManager> getInstance() {
        if (fileManager_ == nullptr) {
            fileManager_ = std::make_shared<FileManager>();
        }
        return fileManager_;
    }

    void File_pushJob(const File_JobFunction& job);
    FileManager();
    ~FileManager();

private:
    std::vector<std::thread>     threads_;
    std::queue<File_JobFunction> Job_queues_;  // 任务队列

    std::mutex              File_lock_;
    std::condition_variable File_conn_;
    bool                    stop_;

    static std::shared_ptr<FileManager> fileManager_;
};
#endif