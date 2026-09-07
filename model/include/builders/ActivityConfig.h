#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <utility>

#include "core/Activity.h"
#include "core/CommonTypes.h"
#include "core/DateGenerator.h"
#include "generators/SingleGenerator.h"
#include "domain/Meeting.h"
#include "domain/Task.h"

namespace events {

struct ActivityConfig {
    String title = "";
    TimePoint start = TimePoint{};
    Duration duration = Duration::zero();
    TimePoint end = TimePoint::max();
    std::shared_ptr<const DateGenerator> generator = nullptr;
    std::vector<TimePoint> exceptions = {};
};

struct TaskConfig : public ActivityConfig {
    Priority priority = Priority::Medium;
    bool done = false;

    TaskConfig(ActivityConfig base = {}, Priority p = Priority::Medium, bool d = false)
        : ActivityConfig(std::move(base)), priority(p), done(d) {}
};

struct MeetingConfig : public ActivityConfig {
    String location = "";
    std::vector<String> attendees = {};

    MeetingConfig(ActivityConfig base = {}, String loc = "", std::vector<String> att = {})
        : ActivityConfig(std::move(base)), location(std::move(loc)), attendees(std::move(att)) {}
};

inline std::unique_ptr<Activity> makeActivity(ActivityConfig cfg) {
    auto activity = std::make_unique<Activity>(
        std::move(cfg.title),
        cfg.start,
        cfg.duration,
        std::move(cfg.generator),
        cfg.end
    );
    for (const TimePoint tp : cfg.exceptions) {
        activity->addException(tp);
    }
    return activity;
}

inline std::unique_ptr<Task> makeTask(TaskConfig cfg) {
    auto task = std::make_unique<Task>(
        std::move(cfg.title),
        cfg.start,
        cfg.duration,
        cfg.priority,
        std::move(cfg.generator),
        cfg.end
    );
    if (cfg.done) {
        task->setDone(cfg.start, true);
    }
    for (const TimePoint tp : cfg.exceptions) {
        task->addException(tp);
    }
    return task;
}

inline std::unique_ptr<Meeting> makeMeeting(MeetingConfig cfg) {
    auto meeting = std::make_unique<Meeting>(
        std::move(cfg.title),
        cfg.start,
        cfg.duration,
        std::move(cfg.location),
        std::move(cfg.generator),
        cfg.end
    );
    for (const String& attendee : cfg.attendees) {
        meeting->addAttendee(attendee);
    }
    for (const TimePoint tp : cfg.exceptions) {
        meeting->addException(tp);
    }
    return meeting;
}

} // namespace events