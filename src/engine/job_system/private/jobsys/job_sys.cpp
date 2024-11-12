#include "jobsys/job_sys.hpp"

JobSystem::JobSystem(size_t num_tasks)
{
    for (size_t i = 0; i < num_tasks; ++i)
        workers.emplace_back(std::make_unique<Worker>(this));
}

JobSystem::~JobSystem()
{
    std::unique_lock lk(stop_mutex);

    for (const auto& worker : workers)
        worker->stop();

    stop_cv.wait(lk,
                 [&]
                 {
                     bool finished = true;
                     for (const auto& worker : workers)
                         if (!worker->b_stopped)
                             finished = false;
                     return finished;
                 });
}

Worker::Worker(JobSystem* job_system) : js(job_system)
{
    thread = std::thread(
        [&]
        {
            while (!b_need_stop)
            {
                std::shared_ptr<IJob> job = nullptr;
                js->jobs.wait_dequeue(job);
                job->run();
            }
            b_stopped = true;
            js->stop_cv.notify_one();
        });
}

void Worker::stop()
{
    b_need_stop = true;
}