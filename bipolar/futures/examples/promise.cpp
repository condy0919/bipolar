#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "bipolar/futures/promise.hpp"
#include "bipolar/futures/single_threaded_executor.hpp"

using namespace bipolar;
using namespace std::literals;

static void resume_in_a_little_while(SuspendedTask task) {
    std::thread t([task]() mutable {
        std::this_thread::sleep_for(50ms);
        task.resume_task();
    });
    t.detach();
}

static Promise<int, std::string> pick_peaches(int hours) {
    return make_promise([hours, time = 0, harvest = 0](
                            Context& ctx) mutable -> Result<int, std::string> {
        if (time == 0) {
            std::printf("Starting the day picking peaches for %d hours...\n",
                        hours);
        } else {
            std::printf("... %d hour(s) elapsed...\n", time);
        }

        if (random() % 7 == 0) {
            return Err("A wild animal ate all the peaches we picked today!"s);
        }
        if (time < hours) {
            // Simulate time passing.
            // Here we call `suspend_task()` to obtain a `SuspendedTask`
            // which acts as a handle wihch will later be used by
            // `resume_in_a_little_while()` to resume the task. In the
            // meantime, we unwind the call stack by returning `Pending`.
            // Once the task is resumed, the promise's handler will restart
            // execution from the top again, However it will have retained
            // state (in `time` and `harvest`) from its prior execution.
            resume_in_a_little_while(ctx.suspend_task());
            ++time;
            harvest += static_cast<int>(random() % 31);
        }
        return Ok(harvest);
    });
}

static Promise<Void, std::string> eat_peaches(int appetite) {
    return make_promise(
        [appetite](Context& ctx) mutable -> Result<Void, std::string> {
            if (appetite > 0) {
                std::printf("... eating a yummy peach...\n");
                resume_in_a_little_while(ctx.suspend_task());
                --appetite;
                if (random() % 11 == 0) {
                    return Err("I ate too many peaches. Urp"s);
                }
                return Pending{};
            }
            std::printf("Ahh. So satisfying\n");
            return Ok(Void{});
        });
}

static Promise<Void, Void> prepare_simulation() {
    const int hours = static_cast<int>(random() % 8);
    return pick_peaches(hours)
        .and_then([](const int& harvest) -> Result<int, std::string> {
            std::printf("We picked %d peaches today!\n", harvest);
            if (harvest == 0) {
                return Err("What will we eat now?"s);
            }
            return Ok(harvest);
        })
        .and_then([](const int& harvest) {
            int appetite = static_cast<int>(random() % 7);
            if (appetite > harvest) {
                appetite = harvest;
            }
            return eat_peaches(appetite);
        })
        .or_else([](const std::string& error) {
            std::printf("Oh no! %s\n", error.c_str());
            return Err(Void{});
        })
        .and_then([](Void&) {
            std::printf("*** Simulation finished ***\n");
            return Ok(Void{});
        })
        .or_else([](Void&) {
            std::printf("*** Restarting simulation ***\n");
            return prepare_simulation();
        });
}

int main() {
    SingleThreadedExecutor executor;

    auto p = prepare_simulation();
    executor.schedule_task(PendingTask(std::move(p)));
    executor.run();

    return 0;
}
