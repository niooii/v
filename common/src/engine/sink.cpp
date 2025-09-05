//
// Created by niooi on 8/5/25.
//

#include <engine/sink.h>

void DependentSink::execute()
{
    static tf::Executor executor_(1);
    if (!registered_tasks_.empty())
    {
        executor_.run(taskflow_).wait();
    }
}
