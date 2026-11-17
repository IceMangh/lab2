#ifndef LAB2_HASHTABLE_H
#define LAB2_HASHTABLE_H

#include <memory>
#include <functional>
#include <cstddef>
#include "DynamicArray.h"

template<typename K, typename V>
class HashTable {
private:
    struct Node {
        K key;
        V value;
        std::unique_ptr<Node> next;

        Node(const K& key, const V& value) : key(key), value(value) {}
    };

    std::size_t capacity = 8;
    std::size_t count = 0;
    DynamicArray<std::unique_ptr<Node>> buckets;

    std::size_t GetIndex(const K& key) const {
        return (std::hash<K>{}(key)) % capacity;
    }

    void Rehash(std::size_t newCapacity) {
        DynamicArray<std::unique_ptr<Node>> new_buckets(newCapacity);
        for (int i = 0; i < capacity; i++) {
            while (buckets[i]) {
                std::unique_ptr<Node> tmp_buckets = std::move(buckets[i]);
                buckets[i] = std::move(tmp_buckets->next);
                std::size_t new_index = std::hash<K>{}(tmp_buckets->key) % newCapacity;

                tmp_buckets->next = std::move(new_buckets[new_index]);
                new_buckets[new_index] = std::move(tmp_buckets);
            }
        }
        buckets.Swap(new_buckets);
        capacity = newCapacity;

    }

public:

    HashTable(): buckets(capacity) {}

    bool ContainsKey(const K& key) const {
        std::size_t index = GetIndex(key);
        const Node* node = buckets[index].get();

        while (node != nullptr) {
            if (node->key == key) {
                return true;
            }
            node = node->next.get();
        }
        return false;
    }

    const V& Get(const K& key) const {
        std::size_t index = GetIndex(key);
        const Node* node = buckets[index].get();

        while (node != nullptr) {
            if (node->key == key) {
                return node->value;
            }
            node = node->next.get();
        }
        throw std::out_of_range("Key not found");
    }

    void Add(const K& key, const V& value) {
        if (ContainsKey(key)) {
            throw std::invalid_argument("Key already exists");
        }
        if (count >= capacity) {
            Rehash(capacity * 2);
        }

        std::size_t index = GetIndex(key);
        std::unique_ptr<Node> new_node(new Node(key, value));
        new_node->next = std::move(buckets[index]);
        buckets[index] = std::move(new_node);
        count ++;

        }

    std::size_t GetCount() const {
        return count;
    }

    std::size_t GetCapacity() const {
        return capacity;
    }

    void Remove(const K& key) {
        std::size_t index = GetIndex(key);
        std::unique_ptr<Node>* link = &buckets[index];

        while (*link) {
            if ((*link)->key == key) {
                std::unique_ptr<Node> temp = std::move(*link);
                *link = std::move(temp->next);
                --count;

                if (capacity > 8 && count <= capacity / 4) {
                    Rehash(capacity / 2);
                }

                return;
            }
            link = &((*link)->next);
        }

        throw std::out_of_range("Key not found");
    }

    void Update(const K& key, const V& value) {
        std::size_t index = GetIndex(key);
        Node* node = buckets[index].get();

        while (node != nullptr) {
            if (node->key == key) {
                node->value = value;
                return;
            }
            node = node->next.get();
        }
        throw std::out_of_range("Key not found");
    }

};

#endif
