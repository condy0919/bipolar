#include <cstdint>
#include <map>

#include "bipolar/futures/promise.hpp"
#include "bipolar/futures/suspended_task.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

enum class Disposition {
    PENDING,
    RESUMED,
    RELEASED,
};

class FakeResolver : public SuspendedTask::Resolver {
public:
    std::uint64_t num_tickets_issued() const {
        return next_ticket_ - 1;
    }

    SuspendedTask::Ticket obtain_ticket() {
        SuspendedTask::Ticket ticket = next_ticket_++;
        tickets_.emplace(ticket, Disposition::PENDING);
        return ticket;
    }

    Disposition get_disposition(SuspendedTask::Ticket ticket) {
        auto iter = tickets_.find(ticket);
        EXPECT_NE(iter, tickets_.end());
        return iter->second;
    }

    SuspendedTask::Ticket
    duplicate_ticket(SuspendedTask::Ticket ticket) override {
        auto iter = tickets_.find(ticket);
        EXPECT_NE(iter, tickets_.end());
        EXPECT_EQ(iter->second, Disposition::PENDING);
        return obtain_ticket();
    }

    void resolve_ticket(SuspendedTask::Ticket ticket,
                        bool resume_task) override {
        auto iter = tickets_.find(ticket);
        EXPECT_NE(iter, tickets_.end());
        EXPECT_EQ(iter->second, Disposition::PENDING);

        iter->second =
            resume_task ? Disposition::RESUMED : Disposition::RELEASED;
    }

private:
    SuspendedTask::Ticket next_ticket_ = 1;
    std::map<SuspendedTask::Ticket, Disposition> tickets_;
};

TEST(SuspendedTask, test) {
    FakeResolver resolver;
    {
        SuspendedTask empty1;
        EXPECT_FALSE(empty1);

        SuspendedTask empty2(nullptr, 42);
        EXPECT_FALSE(empty2);

        SuspendedTask empty_copy(empty1);
        EXPECT_FALSE(empty_copy);
        EXPECT_FALSE(empty1);

        SuspendedTask empty_move(std::move(empty2));
        EXPECT_FALSE(empty_move);
        EXPECT_FALSE(empty2);

        SuspendedTask task(&resolver, resolver.obtain_ticket());
        EXPECT_TRUE(task);
        EXPECT_EQ(resolver.num_tickets_issued(), 1);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);

        SuspendedTask task_copy(task);
        EXPECT_TRUE(task_copy);
        EXPECT_TRUE(task);
        EXPECT_EQ(resolver.num_tickets_issued(), 2);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);

        SuspendedTask task_move(std::move(task));
        EXPECT_TRUE(task_move);
        EXPECT_FALSE(task);
        EXPECT_EQ(resolver.num_tickets_issued(), 2);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);

        SuspendedTask x;
        x = empty1;
        EXPECT_FALSE(x);

        x = task_copy;
        EXPECT_TRUE(x);
        EXPECT_TRUE(task_copy);
        EXPECT_EQ(resolver.num_tickets_issued(), 3);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(3), Disposition::PENDING);

        x = std::move(empty_move); // x's ticket is released here
        EXPECT_FALSE(x);
        EXPECT_FALSE(empty_move);
        EXPECT_EQ(resolver.num_tickets_issued(), 3);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(3), Disposition::RELEASED);

        x = std::move(empty_move); // x's ticket is released here
        EXPECT_FALSE(x);
        EXPECT_FALSE(empty_move);
        EXPECT_EQ(resolver.num_tickets_issued(), 3);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(3), Disposition::RELEASED);

        x = task_copy;            //  assign x a duplicate ticket
        x = std::move(task_move); // x's ticket is released here
        EXPECT_TRUE(x);
        EXPECT_TRUE(task_copy);
        EXPECT_FALSE(task_move);
        EXPECT_EQ(resolver.num_tickets_issued(), 4);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(3), Disposition::RELEASED);
        EXPECT_EQ(resolver.get_disposition(4), Disposition::RELEASED);

        x.resume_task(); // x's ticket is resumed here
        EXPECT_FALSE(x);
        EXPECT_EQ(resolver.num_tickets_issued(), 4);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::RESUMED);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(3), Disposition::RELEASED);
        EXPECT_EQ(resolver.get_disposition(4), Disposition::RELEASED);

        x.resume_task(); // already resued so has no effect
        EXPECT_FALSE(x);

        x.reset(); // already resumed so has no effect
        EXPECT_FALSE(x);

        // NOTE: task_copy still has a ticket here which will be
        // released when the scope exits
    }
    EXPECT_EQ(resolver.num_tickets_issued(), 4);
    EXPECT_EQ(resolver.get_disposition(1), Disposition::RESUMED);
    EXPECT_EQ(resolver.get_disposition(2), Disposition::RELEASED);
    EXPECT_EQ(resolver.get_disposition(3), Disposition::RELEASED);
    EXPECT_EQ(resolver.get_disposition(4), Disposition::RELEASED);
}

TEST(SuspendedTask, swap) {
    FakeResolver resolver;
    {
        SuspendedTask a(&resolver, resolver.obtain_ticket());
        SuspendedTask b(&resolver, resolver.obtain_ticket());
        SuspendedTask c;
        EXPECT_EQ(resolver.num_tickets_issued(), 2);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);

        a.swap(c);
        EXPECT_FALSE(a);
        EXPECT_TRUE(c);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);

        swap(c, b);
        EXPECT_TRUE(b);
        EXPECT_TRUE(c);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::PENDING);

        c.resume_task();
        EXPECT_FALSE(c);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::PENDING);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::RESUMED);

        b.reset();
        EXPECT_FALSE(b);
        EXPECT_EQ(resolver.get_disposition(1), Disposition::RELEASED);
        EXPECT_EQ(resolver.get_disposition(2), Disposition::RESUMED);
    }
}
