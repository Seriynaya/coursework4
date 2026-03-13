# Coursework 4 — Mail System

C++ implementation of a messaging/mail system inspired by the Django mail application.

## Data Structures & Complexity

| Operation | Time | Structure |
|---|---|---|
| Add message | O(log n) | vector + heap push |
| Inbox lookup | O(k) | unordered_map -> vector of indices |
| Outbox lookup | O(k) | unordered_map -> vector of indices |
| Unread count | O(k) | linear scan of user's inbox |
| Mark read | O(n) worst | linear scan by id |
| Pop urgent | O(log n) | max-heap by priority |

## Build & Run

```bash
g++ -std=c++17 -O2 -o coursework4 main.cpp
./coursework4
```

## Description

- Message struct with id, sender, recipient, subject, body, timestamp, read flag, priority
- Mailbox class with O(1) inbox/outbox lookup via hash map indices
- Priority queue (max-heap) for urgent message processing
- Demonstrates inbox, outbox, unread count, mark read, and priority-based dispatch
