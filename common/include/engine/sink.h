//
// Created by niooi on 8/3/25.
//

#pragma once

#include <defs.h>
#include <functional>
#include <string>
#include <taskflow/taskflow.hpp>
#include <unordered_dense.h>
#include <vector>

struct TaskDefinition {
    std::string              name;
    std::function<void()>    func;
    std::vector<std::string> after; // Tasks this one should run AFTER
    std::vector<std::string> before; // Tasks this one should run BEFORE
};

/// Task dependency manager w/ TaskFlow
/// HORRIBLE NAME BTW
class DependentSink {
public:
    /// Connect a task with dependency specifications
    /// @param after Tasks that this task should run AFTER
    /// @param before Tasks that this task should run BEFORE
    /// @param name Unique name for this task
    /// @param func Function to execute
    /// @note ALL tasks should be thread-safe
    FORCEINLINE void connect(
        const std::vector<std::string>& after, const std::vector<std::string>& before,
        const std::string& name, std::function<void()> func)
    {
        TaskDefinition task_def;
        task_def.name   = name;
        task_def.func   = std::move(func);
        task_def.after  = after;
        task_def.before = before;

        registered_tasks_[name] = std::move(task_def);
        rebuild_graph();
    }

    /// Disconnect a task by name
    /// @param name Name of the task to remove
    FORCEINLINE void disconnect(const std::string& name)
    {
        registered_tasks_.erase(name);
        rebuild_graph();
    }

    /// Execute the task graph
    void execute();

private:
    /// Rebuild the TaskFlow graph when tasks are added or removed
    void rebuild_graph()
    {
        taskflow_.clear();

        if (registered_tasks_.empty())
        {
            return;
        }

        ankerl::unordered_dense::map<std::string, tf::Task> handles;

        // Create all task nodes
        for (const auto& [name, def] : registered_tasks_)
        {
            handles[name] = taskflow_.emplace(def.func).name(name);
        }

        // Set up all relations/contraints
        for (const auto& [name, def] : registered_tasks_)
        {
            tf::Task& current_task = handles.at(name);

            // deps
            for (const std::string& dep_name : def.after)
            {
                if (handles.contains(dep_name))
                {
                    handles.at(dep_name).precede(current_task);
                }
            }

            // run after these
            for (const std::string& succ_name : def.before)
            {
                if (handles.contains(succ_name))
                {
                    current_task.precede(handles.at(succ_name));
                }
            }
        }
    }

    ankerl::unordered_dense::map<std::string, TaskDefinition> registered_tasks_;
    tf::Taskflow                                              taskflow_;
};
