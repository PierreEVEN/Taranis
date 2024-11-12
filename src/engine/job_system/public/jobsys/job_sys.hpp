#pragma once

#include <concurrentqueue/moodycamel/blockingconcurrentqueue.h>

class IJob
{
public:
    virtual void run() = 0;
};

template <typename Ret> class TJobRet : public IJob
{
public:
    bool finished() const
    {
        return ready.load();
    }

    Ret await()
    {
        std::unique_lock lk(wait_mutex);
        wait_cond.wait(lk,
                       [&]
                       {
                           return ready.load();
                       });

        if constexpr (!std::is_same_v<Ret, void>)
            return *ret;
        else
            return;
    }

    virtual ~TJobRet()
    {
        delete ret;
    }

protected:
    std::mutex              wait_mutex;
    std::condition_variable wait_cond;
    std::atomic_bool        ready = false;
    Ret*                    ret   = nullptr;
};

template <typename Lambda, typename Ret>
class TJob : public TJobRet<Ret>
{
public:
    TJob(Lambda callback) : cb(callback)
    {
    }

    void run() override
    {
        if constexpr (!std::is_same_v<Ret, void>)
        {
            TJobRet<Ret>::ret  = static_cast<Ret*>(std::calloc(1, sizeof(Ret)));
            *TJobRet<Ret>::ret = std::move(cb());
        }
        else
            cb();

        TJobRet<Ret>::ready = true;
        TJobRet<Ret>::wait_cond.notify_all();
    }

private:
    Lambda cb;
};

template <typename Ret> class JobHandle
{
public:
    JobHandle(std::shared_ptr<TJobRet<Ret>> in_job) : job(std::move(in_job))
    {
    }

    bool finished() const
    {
        return job->finished();
    }

    Ret await() const
    {
        return job->await();
    }

    Ret operator*() const
    {
        return job->await();
    }

    operator bool() const
    {
        return job->finished();
    }

private:
    std::shared_ptr<TJobRet<Ret>> job;
};

class JobSystem final
{
public:
    JobSystem(size_t num_tasks);

    template <typename Ret = void, typename Lambda> JobHandle<Ret> schedule(Lambda job)
    {
        std::shared_ptr<TJob<Lambda, Ret>> task = std::make_shared<TJob<Lambda, Ret>>(job);
        jobs.enqueue(task);
        return JobHandle<Ret>(std::dynamic_pointer_cast<TJobRet<Ret>>(task));
    }

    ~JobSystem();

private:
    friend class Worker;
    std::vector<std::unique_ptr<Worker>>                       workers;
    std::mutex                                                 stop_mutex;
    std::condition_variable                                    stop_cv;
    moodycamel::BlockingConcurrentQueue<std::shared_ptr<IJob>> jobs;
};

class Worker
{
public:
    Worker(JobSystem* job_system);
    void stop();

private:
    friend class JobSystem;
    JobSystem*       js          = nullptr;
    std::atomic_bool b_need_stop = false;
    std::atomic_bool b_stopped   = false;
    std::thread      thread;
};