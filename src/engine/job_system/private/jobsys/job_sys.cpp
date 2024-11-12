#include "jobsys/job_sys.hpp"

#include <iostream>

JobSystem::JobSystem(size_t num_tasks)
{
    for (size_t i = 0; i < num_tasks; ++i)
        workers.emplace_back(std::make_unique<Worker>(this));
}

Worker::Worker(JobSystem* job_system) : js(job_system)
{
    thread = std::thread(
        [&]
        {
            while (!b_need_stop)
            {
                std::shared_ptr<IJob> job = nullptr;
                std::unique_lock      lk(js->job_add_mutex);
                js->job_added.wait(lk,
                                   [&]
                                   {
                                       if (b_need_stop)
                                           return true;
                                       js->jobs.try_dequeue(job);
                                       return static_cast<bool>(job);
                                   });
                if (job)
                    job->run();
            }
        });
}

Worker::~Worker()
{
    b_need_stop = true;
    js->job_added.notify_all();
    thread.join();
}

void Worker::stop()
{
    b_need_stop = true;
}