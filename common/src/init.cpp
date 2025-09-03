//
// Created by niooi on
// 7/24/25.
//

#include <prelude.h>
#include <time/time.h>

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <absl/container/btree_set.h>
#include "spdlog/common.h"

void init_loggers();

void v::init()
{
    // absl::FailureSignalHandlerOptions fail_opts = {
    //
    // };
    //
    // absl::InstallFailureSignalHanWdler(fail_opts);

    init_loggers();

    time::init();
}

void init_loggers()
{
    // Init loggers

    // Console logger
    const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Daily logger - a new file is created every day at 12 am
    const auto file_sink =
        std::make_shared<spd::sinks::daily_file_sink_mt>("./logs/log", 0, 0);

    const spd::sinks_init_list sinks = { stdout_sink, file_sink };

    const auto logger = std::make_shared<spd::logger>("", sinks.begin(), sinks.end());

    logger->set_level(spd::level::trace);

    logger->flush_on(spd::level::err);

    set_default_logger(logger);

    LOG_INFO("hi starting up...");
}
