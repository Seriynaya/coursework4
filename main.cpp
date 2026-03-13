#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <optional>
#include <ranges>

struct Message {
    int id;
    int sender_id;
    int recipient_id;
    std::string subject;
    std::string body;
    std::string timestamp; // "YYYY-MM-DD HH:MM"
    bool read;
    int priority; // 0=low, 1=normal, 2=high, 3=urgent
};

// max-heap by priority, tie-break by id desc
struct MsgPriorityCmp {
    [[nodiscard]] constexpr bool operator()(const Message& a, const Message& b) const {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.id < b.id;
    }
};

class Mailbox {
    std::vector<Message> messages_;
    // inbox_idx[u] = {i : messages_[i].recipient_id = u}
    std::unordered_map<int, std::vector<int>> inbox_idx_;
    // outbox_idx[u] = {i : messages_[i].sender_id = u}
    std::unordered_map<int, std::vector<int>> outbox_idx_;
    std::priority_queue<Message, std::vector<Message>, MsgPriorityCmp> urgent_queue_;

public:
    // T_add = O(log n) (heap push)
    void send_message(Message&& msg) {
        const int idx = static_cast<int>(messages_.size());
        if (msg.priority >= 2) {
            urgent_queue_.push(msg);
        }
        inbox_idx_[msg.recipient_id].push_back(idx);
        outbox_idx_[msg.sender_id].push_back(idx);
        messages_.push_back(std::move(msg));
    }

    void send_message(const Message& msg) {
        auto copy = msg;
        send_message(std::move(copy));
    }

    // inbox(u) = {m in M : recipient(m) = u}
    [[nodiscard]] auto inbox(int user_id) const -> std::vector<Message> {
        std::vector<Message> result;
        const auto it = inbox_idx_.find(user_id);
        if (it == inbox_idx_.end()) return result;
        result.reserve(it->second.size());
        for (const int idx : it->second)
            result.push_back(messages_[idx]);
        return result;
    }

    // outbox(u) = {m in M : sender(m) = u}
    [[nodiscard]] auto outbox(int user_id) const -> std::vector<Message> {
        std::vector<Message> result;
        const auto it = outbox_idx_.find(user_id);
        if (it == outbox_idx_.end()) return result;
        result.reserve(it->second.size());
        for (const int idx : it->second)
            result.push_back(messages_[idx]);
        return result;
    }

    // unread(u) = |{m in M : recipient(m)=u ^ ~read(m)}|
    [[nodiscard]] auto unread_count(int user_id) const -> int {
        const auto it = inbox_idx_.find(user_id);
        if (it == inbox_idx_.end()) return 0;
        return static_cast<int>(std::ranges::count_if(
            it->second, [&](int idx) { return !messages_[idx].read; }));
    }

    // find message by id, nullopt if not found
    [[nodiscard]] auto find_message(int msg_id) const -> std::optional<Message> {
        for (const auto& m : messages_) {
            if (m.id == msg_id) return m;
        }
        return std::nullopt;
    }

    // mark_read: set read(m) = true
    auto mark_read(int msg_id) -> bool {
        for (auto& m : messages_) {
            if (m.id == msg_id) {
                m.read = true;
                return true;
            }
        }
        return false;
    }

    // |affected| = unread(u)
    auto mark_all_read(int user_id) -> int {
        int cnt = 0;
        const auto it = inbox_idx_.find(user_id);
        if (it == inbox_idx_.end()) return 0;
        for (const int idx : it->second) {
            if (!messages_[idx].read) {
                messages_[idx].read = true;
                ++cnt;
            }
        }
        return cnt;
    }

    // pop highest priority: O(log n)
    [[nodiscard]] auto pop_urgent() -> std::optional<Message> {
        if (urgent_queue_.empty()) return std::nullopt;
        auto top = urgent_queue_.top();
        urgent_queue_.pop();
        return top;
    }

    [[nodiscard]] auto has_urgent() const -> bool {
        return !urgent_queue_.empty();
    }

    [[nodiscard]] auto total() const -> int {
        return static_cast<int>(messages_.size());
    }
};

void print_message(const Message& m) {
    std::cout << "  [" << m.id << "] "
              << "from:" << m.sender_id << " to:" << m.recipient_id
              << " pri:" << m.priority
              << " read:" << (m.read ? "Y" : "N")
              << " subj:\"" << m.subject << "\""
              << " @" << m.timestamp << "\n";
}

int main() {
    Mailbox mb;

    // |M| = 12
    std::vector<Message> sample = {
        {1, 10, 20, "Meeting tomorrow",     "Let's meet at 10am",        "2026-03-10 09:00", false, 1},
        {2, 10, 30, "Project update",       "Phase 1 complete",          "2026-03-10 09:15", false, 1},
        {3, 20, 10, "Re: Meeting",          "Confirmed",                 "2026-03-10 09:30", false, 0},
        {4, 30, 20, "Invoice #42",          "Please review attached",    "2026-03-10 10:00", false, 2},
        {5, 10, 20, "URGENT: Server down",  "DB connection lost",        "2026-03-10 10:05", false, 3},
        {6, 20, 30, "Budget report",        "Q1 numbers attached",       "2026-03-10 11:00", false, 1},
        {7, 30, 10, "Deadline reminder",    "Submit by Friday",          "2026-03-10 12:00", false, 2},
        {8, 10, 20, "Lunch?",               "Cafeteria at noon?",        "2026-03-10 11:30", false, 0},
        {9, 40, 20, "Welcome aboard",       "Onboarding docs inside",    "2026-03-10 08:00", false, 1},
        {10, 20, 40, "Re: Welcome",         "Thanks!",                   "2026-03-10 08:30", true,  0},
        {11, 40, 10, "Security alert",      "Unusual login detected",    "2026-03-10 13:00", false, 3},
        {12, 30, 20, "Weekly sync",         "Agenda in body",            "2026-03-10 14:00", false, 1},
    };

    for (auto& m : sample) mb.send_message(std::move(m));
    std::cout << "Total messages: " << mb.total() << "\n\n";

    // inbox(20)
    int user = 20;
    const auto inb = mb.inbox(user);
    std::cout << "Inbox for user " << user << " (|inbox|=" << inb.size() << "):\n";
    for (const auto& m : inb) print_message(m);

    // unread(20) = |{m in M : recipient(m)=20 ^ ~read(m)}|
    std::cout << "\nUnread count for user " << user << ": " << mb.unread_count(user) << "\n";

    // outbox(10)
    user = 10;
    const auto outb = mb.outbox(user);
    std::cout << "\nOutbox for user " << user << " (|outbox|=" << outb.size() << "):\n";
    for (const auto& m : outb) print_message(m);

    // find_message(5)
    if (const auto found = mb.find_message(5)) {
        std::cout << "\nfind_message(5):\n";
        print_message(*found);
    }

    // mark_read(1)
    mb.mark_read(1);
    std::cout << "\nAfter mark_read(1), unread(20) = " << mb.unread_count(20) << "\n";

    // mark_all_read(20)
    const int marked = mb.mark_all_read(20);
    std::cout << "mark_all_read(20): " << marked << " messages marked\n";
    std::cout << "unread(20) = " << mb.unread_count(20) << "\n";

    // T_push = O(log n), T_top = O(1)
    std::cout << "\nUrgent messages (priority >= 2), popped in order:\n";
    while (auto m = mb.pop_urgent()) {
        print_message(*m);
    }

    return 0;
}
