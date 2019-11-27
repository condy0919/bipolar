#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "bipolar/futures/promise.hpp"
#include "bipolar/futures/single_threaded_executor.hpp"

using namespace bipolar;
using namespace std::literals;

void resume_in_a_little_while(SuspendedTask task) {
    std::thread t([task]() mutable {
        std::this_thread::sleep_for(50ms);
        task.resume_task();
    });
    t.detach();
}

// State for a two player game
//
// Players do battle by simultaneously rolling dice in order to inflict
// damage upon their opponent over the course of several rounds until one
// or both players hit points are depleted to 0.
//
// Players start with 100 hit points. During each round, each player first
// rolls a Damage die (numbered 0 to 9) and an Effect die (numbered 0 to 3).
// If the Effect die comes up 0, the player casts a lighting spell and
// rolls an Effect Multiplier die (numbered 0 to 4) to determine the
// stringeth of the effect
//
// The following calcuation determines the damage dealt to the player's
// opponent:
//
//   if Damage die value is non-zero,
//     then opponent HP -= value of Damage die
//   if Effect die is zero (cast lighting),
//     then opponent HP -= value of Effect die * 2 + 3
//
// Any dice that fly off the table during especially vigorous rolls are
// rerolled before damage is tallied.
struct GameState {
    int red_hp = 100;
    int blue_hp = 100;
};

// Rolls a die and waits for it to settle down then return its value.
// This task might fail so the caller needs to be prepared to re-roll.
//
// This function demonstrates returning pending, error, and ok state
// as well as task suspension.
auto roll_die(std::string player, std::string type, int number_of_sides) {
    return make_promise([player, type, number_of_sides](
                            Context& ctx) -> AsyncResult<int, Void> {
        // Simulate the outcode of rolling a die.
        // Either the die will settle, keel rolling, or fall off the table.
        const int event = random() % 6;
        if (event == 0) {
            std::printf("%s's '%s' die flew right off the table!\n",
                        player.c_str(), type.c_str());
            return AsyncError(Void{});
        } else if (event < 3) {
            // The die is still rolling around. Need to wait for it to settle
            resume_in_a_little_while(ctx.suspend_task());
            return AsyncPending{};
        }

        const int value = random() % number_of_sides;
        std::printf("%s rolled %d for '%s'\n", player.c_str(), value,
                    type.c_str());
        return AsyncOk(value);
    });
}

// Re-rolls a die until it succeeds.
//
// This function demonstrates looping a task using a recursive tail-call.
Promise<int, Void> roll_die_until_successful(std::string player,
                                             std::string type,
                                             int number_of_sides) {
    return roll_die(player, type, number_of_sides)
        .or_else([player, type, number_of_sides](Void&) {
            // An error occurred while rolling the die. Recurse to try again
            return roll_die_until_successful(player, type, number_of_sides);
        });
}

// Rolls an effect and damage die.
// If the effect die comes up 0 then also rolls an effect multiplier die to
// determine the strength of the effect. We can do this while waiting
// for the damage die to settle down.
//
// This function demonstrates the benefits of capturing a task into a
// `Future` so that its result can be retained and repeatedly examined
// while awaiting other tasks.
Promise<int, Void> roll_for_damage(std::string player) {
    return make_promise([player,
                         damage = Future<int, Void>(
                             roll_die_until_successful(player, "damage", 10)),
                         effect = Future<int, Void>(
                             roll_die_until_successful(player, "effect", 4)),
                         effect_multiplier = Future<int, Void>()](
                            Context& ctx) mutable -> AsyncResult<int, Void> {
        // Evaluate the damage die roll future
        const bool damage_ready = damage(ctx);

        // Evaluate the effect die roll future
        // If the player rolled lightning, begin rolling the multiplier
        bool effect_ready = effect(ctx);
        if (effect_ready) {
            if (effect.value() == 0) {
                if (!effect_multiplier) {
                    effect_multiplier =
                        roll_die_until_successful(player, "multiplier", 4);
                }
                effect_ready = effect_multiplier(ctx);
            }
        }

        // If we're still waiting for the dice to settle, return pending
        // The task will be resumed once it can make progress
        if (!effect_ready || !damage_ready) {
            return AsyncPending{};
        }

        // Calculate the result and describe what happened
        if (damage.value() == 0) {
            std::printf(
                "%s swings wildling and completely miss their opponent\n",
                player.c_str());
        } else {
            std::printf("%s hits their opponent for %d damage\n",
                        player.c_str(), damage.value());
        }

        int effect_bonus = 0;
        if (effect.value() == 0) {
            if (effect_bonus == 0) {
                std::printf("%s attempts to cast lignting but the spell "
                            "fizzles without effect\n",
                            player.c_str());
            } else {
                effect_bonus = effect_multiplier.value() * 2 + 3;
                std::printf("%s casts ligntning for %d damage\n",
                            player.c_str(), effect_bonus);
            }
        }

        return AsyncOk(damage.value() + effect_bonus);
    });
}

// Plays one round of the game.
// Both players roll dice simultaneously to determine the damage dealt
// to their opponent.
//
// This function demonstrates joining the results of concurrently executed
// tasks as a new task which produce a tuple
auto play_round(std::shared_ptr<GameState> state) {
    return join_promises(roll_for_damage("Red"), roll_for_damage("Blue"))
        .and_then([state](const std::tuple<AsyncResult<int, Void>,
                                           AsyncResult<int, Void>>& damages) {
            // damage tallies are ready, apply them to the game state
            state->blue_hp =
                std::max(0, state->blue_hp - std::get<0>(damages).value());
            state->red_hp =
                std::max(0, state->red_hp - std::get<1>(damages).value());
            std::printf("Hit-points remaining: red %d, blue %d\n",
                        state->red_hp, state->blue_hp);
            return AsyncOk(Void{});
        });
}

// Plays a little game.
// Red and Blue each start with 100 hit points.
// During each round, they both simultaneously roll dice to determine damage to
// their opponent. If at the end of the round one player's hit-points reaches 0,
// that player loses. If both player's hit-points reach 0, they both lose.
static auto play_game() {
    std::printf("Red and blue are playing a game...\n");
    return make_promise(
        [state = std::make_shared<GameState>(), round = Future<Void, Void>()](
            Context& ctx) mutable -> AsyncResult<Void, Void> {
            while (state->red_hp != 0 && state->blue_hp != 0) {
                if (!round) {
                    round = play_round(state);
                }
                if (!round(ctx)) {
                    return AsyncPending{};
                }
                round = nullptr;
            }

            // Game over
            std::printf("Game over...\n");
            if (state->red_hp == 0 && state->blue_hp == 0) {
                std::printf("Both players lose!\n");
            } else if (state->red_hp != 0) {
                std::printf("Red wins!\n");
            } else {
                std::printf("Blue wins!\n");
            }
            return AsyncOk(Void{});
        });
}

int main() {
    SingleThreadedExecutor executor;

    auto p = play_game();
    executor.schedule_task(PendingTask(std::move(p)));
    executor.run();

    return 0;
}
