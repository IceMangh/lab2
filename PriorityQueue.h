#ifndef LAB2_PRIORITYQUEUE_H
#define LAB2_PRIORITYQUEUE_H

#include "BinarySearchTree.h"
#include <cstddef>

template <typename T>
class PriorityQueue {
private:
    struct Entry {
        T value;
        long long priority;
        std::size_t id;

        bool operator<(const Entry& other) const {
            if (priority == other.priority){
                return (id < other.id);
            } else {
                return (priority < other.priority);
            }
        }
        bool operator>(const Entry& other) const {
            if (priority == other.priority){
                return (id > other.id);
            } else {
                return (priority > other.priority);
            }
        }
    };

    BinarySearchTree<Entry> tree;
    std::size_t nextId = 0;

public:

    void Push(const T& value, long long priority) {
        Entry entry(value, priority, nextId);
        tree.Insert(entry);
        nextId += 1;
    }

    const T& Top() const {
        return tree.GetMax().value;
    }

    T Pop() {
        return tree.ExtractMax().value;
    }

    bool IsEmpty() const {
        return tree.IsEmpty();
    }

};

#endif
