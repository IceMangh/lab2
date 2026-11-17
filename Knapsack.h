#ifndef LAB2_KNAPSACK_H
#define LAB2_KNAPSACK_H

#include "Item.h"
#include "DynamicArray.h"
#include "HashTable.h"
#include "PriorityQueue.h"
#include <functional>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <vector>

struct StateKey {
    int index;
    int usedVolume;

    bool operator==(const StateKey& other) const {
        return ((index == other.index) && (usedVolume == other.usedVolume));
    }
};

template<>
struct std::hash<StateKey> {
    std::size_t operator()(const StateKey& key) const noexcept {
        return static_cast<std::size_t>(key.index) * 67
               + static_cast<std::size_t>(key.usedVolume);
    }
};

struct State {
    int index;
    int usedVolume;
    long long totalCost;

    State() : index(0), usedVolume(0), totalCost(0) {}
};

inline long long EstimateBound(const DynamicArray<Item>& items, const State& state) {
    long long cost = state.totalCost;

    for (int i = state.index; i < items.GetLength(); ++i) {
        cost += std::abs(static_cast<long long>(items[i].value));
    }

    return cost;
};

struct DecisionNode {
    State state;
    int id = 0;
    DecisionNode* parent = nullptr;
    int takenItem = -1;

    std::unique_ptr<DecisionNode> take;
    std::unique_ptr<DecisionNode> skip;
};

struct KnapsackResult {
    DynamicArray<int> itemIndices;
    int usedVolume = 0;
    long long totalCost = 0;
};

enum class SearchAction {
    Start, Visit, Best, Skip, Take, Capacity, Duplicate, Bound, Stale, Leaf, Backtrack, Finish
};

struct SearchStep {
    SearchAction action;
    State state;
    int nodeId;
    int parentId;
    int itemIndex;
    bool take = false;
    long long bound = 0;
    long long bestCost = 0;
    long long storedCost = 0;
    std::size_t number = 0;
    std::size_t visited = 0;
    std::size_t pruned = 0;
    std::size_t pending = 0;
    std::vector<int> chosen;
};

struct SearchTrace {
    std::vector<SearchStep> steps;
    std::size_t limit;
    std::size_t totalEvents = 0;
    std::size_t visited = 0;
    std::size_t pruned = 0;
    std::size_t pending = 0;
    int nextId = 1;
    int bestNodeId = 0;
    int bestParentId = -1;
    bool optimized = true;
    bool truncated = false;

    explicit SearchTrace(std::size_t limit = 2000) : limit(limit) {}

    void Reset(bool useOptimized) {
        steps.clear();
        totalEvents = visited = pruned = pending = 0;
        nextId = 1;
        bestNodeId = 0;
        bestParentId = -1;
        optimized = useOptimized;
        truncated = false;
    }

    SearchStep* Add(SearchAction action, const State& state, int nodeId, int parentId,
                    int itemIndex, long long bestCost) {
        ++totalEvents;
        if (action == SearchAction::Visit) {
            ++visited;
            if (optimized) --pending;
        }
        if (optimized && (action == SearchAction::Start || action == SearchAction::Skip || action == SearchAction::Take)) {
            ++pending;
        }
        if (action == SearchAction::Capacity || action == SearchAction::Duplicate
            || action == SearchAction::Bound || action == SearchAction::Stale) {
            ++pruned;
        }
        if (action == SearchAction::Best) {
            bestNodeId = nodeId;
            bestParentId = parentId;
        }
        if (steps.size() >= limit && action != SearchAction::Finish) {
            truncated = true;
            return nullptr;
        }
        steps.push_back({action, state, nodeId, parentId, itemIndex});
        SearchStep& step = steps.back();
        step.bestCost = bestCost;
        step.number = totalEvents;
        step.visited = visited;
        step.pruned = pruned;
        step.pending = pending;
        return &step;
    }

    void RecordNode(SearchAction action, const DecisionNode* node, const DynamicArray<Item>& items,
                    int itemIndex, long long bestCost, long long storedCost = 0) {
        SearchStep* step = Add(action, node->state, node->id, node->parent ? node->parent->id : -1,
                               itemIndex, bestCost);
        if (step == nullptr) return;
        step->bound = EstimateBound(items, node->state);
        step->storedCost = storedCost;
        step->take = node->takenItem >= 0;
        for (const DecisionNode* current = node; current != nullptr; current = current->parent) {
            if (current->takenItem >= 0) step->chosen.push_back(current->takenItem);
        }
        std::reverse(step->chosen.begin(), step->chosen.end());
    }

    void RecordState(SearchAction action, const State& state, const DynamicArray<int>& chosen,
                     int chosenCount, int nodeId, int parentId, int itemIndex, long long bestCost) {
        SearchStep* step = Add(action, state, nodeId, parentId, itemIndex, bestCost);
        if (step == nullptr) return;
        step->take = chosenCount > 0 && chosen[chosenCount - 1] == state.index - 1;
        for (int i = 0; i < chosenCount; ++i) step->chosen.push_back(chosen[i]);
    }
};

inline void ValidateKnapsackInput(const DynamicArray<Item>& items, int capacity) {
    if (capacity < 0) {
        throw std::invalid_argument("Capacity negative");
    }
    for (int i = 0; i < items.GetLength(); ++i) {
        if (items[i].volume < 0) {
            throw std::invalid_argument("Item volume negative");
        }
    }
}

inline KnapsackResult SolveOptimized(const DynamicArray<Item>& items, int capacity, SearchTrace* trace = nullptr) {
    ValidateKnapsackInput(items, capacity);
    auto root = std::make_unique<DecisionNode>();
    PriorityQueue<DecisionNode*> queue;
    HashTable<StateKey, long long> table;
    DecisionNode* bestNode = root.get();
    long long max_res = 0;

    queue.Push(root.get(), EstimateBound(items, root->state));
    table.Add({0, 0}, 0);
    if (trace) {
        trace->Reset(true);
        trace->RecordNode(SearchAction::Start, root.get(), items, -1, 0);
    }

    while (!queue.IsEmpty()) {
        DecisionNode *node = queue.Pop();
        if (trace) trace->RecordNode(SearchAction::Visit, node, items, node->state.index, max_res);

        if (node->state.totalCost > max_res) {
            max_res = node->state.totalCost;
            bestNode = node;
            if (trace) trace->RecordNode(SearchAction::Best, node, items, node->takenItem, max_res);
        }

        StateKey now_key;
        now_key.index = node->state.index;
        now_key.usedVolume = node->state.usedVolume;

        if (node->state.index >= items.GetLength()) {
            if (trace) trace->RecordNode(SearchAction::Leaf, node, items, -1, max_res);
            continue;
        }
        if (max_res >= EstimateBound(items, node->state)) {
            if (trace) trace->RecordNode(SearchAction::Bound, node, items, node->state.index, max_res);
            continue;
        }
        if (table.Get(now_key) > node->state.totalCost) {
            if (trace) trace->RecordNode(SearchAction::Stale, node, items, node->state.index, max_res, table.Get(now_key));
            continue;
        }

        node->skip = std::make_unique<DecisionNode>();
        node->skip->state = node->state;
        node->skip->state.index += 1;
        node->skip->parent = node;
        if (trace) node->skip->id = trace->nextId++;

        StateKey skip_state;
        skip_state.index = node->skip->state.index;
        skip_state.usedVolume = node->skip->state.usedVolume;

        if (!table.ContainsKey(skip_state)) {
            table.Add(skip_state, node->skip->state.totalCost);
            queue.Push(node->skip.get(), EstimateBound(items, node->skip->state));
            if (trace) trace->RecordNode(SearchAction::Skip, node->skip.get(), items, node->state.index, max_res);
        } else if (node->skip->state.totalCost > table.Get(skip_state)) {
            table.Update(skip_state, node->skip->state.totalCost);
            queue.Push(node->skip.get(), EstimateBound(items, node->skip->state));
            if (trace) trace->RecordNode(SearchAction::Skip, node->skip.get(), items, node->state.index, max_res);
        } else {
            if (trace) trace->RecordNode(SearchAction::Duplicate, node->skip.get(), items, node->state.index, max_res, table.Get(skip_state));
        }

        const Item &item = items[node->state.index];

        if (item.volume <= capacity - node->state.usedVolume) {
            node->take = std::make_unique<DecisionNode>();

            node->take->takenItem = node->state.index;
            node->take->state = node->state;
            node->take->state.index += 1;

            node->take->state.usedVolume += item.volume;
            node->take->state.totalCost += item.value;
            node->take->parent = node;
            if (trace) node->take->id = trace->nextId++;

            StateKey take_state;
            take_state.index = node->take->state.index;
            take_state.usedVolume = node->take->state.usedVolume;

            if (!table.ContainsKey(take_state)) {
                table.Add(take_state, node->take->state.totalCost);
                queue.Push(node->take.get(), EstimateBound(items, node->take->state));
                if (trace) trace->RecordNode(SearchAction::Take, node->take.get(), items, node->state.index, max_res);
            } else if (node->take->state.totalCost > table.Get(take_state)) {
                table.Update(take_state, node->take->state.totalCost);
                queue.Push(node->take.get(), EstimateBound(items, node->take->state));
                if (trace) trace->RecordNode(SearchAction::Take, node->take.get(), items, node->state.index, max_res);
            } else {
                if (trace) trace->RecordNode(SearchAction::Duplicate, node->take.get(), items, node->state.index, max_res, table.Get(take_state));
            }
        } else {
            if (trace) trace->RecordNode(SearchAction::Capacity, node, items, node->state.index, max_res);
        }

    }

    KnapsackResult result;
    result.totalCost = max_res;
    result.usedVolume = bestNode->state.usedVolume;
    int count = 0;
    for (DecisionNode* node = bestNode; node != nullptr; node = node->parent) {
        if (node->takenItem >= 0) {
            ++count;
        }
    }
    result.itemIndices.Resize(count);
    for (DecisionNode* node = bestNode; node != nullptr; node = node->parent) {
        if (node->takenItem >= 0) {
            result.itemIndices[--count] = node->takenItem;
        }
    }
    if (trace) trace->RecordNode(SearchAction::Finish, bestNode, items, -1, max_res);
    return result;
}

inline void SearchBruteForce(const DynamicArray<Item>& items, int index, int remainingVolume, int usedVolume, long long totalCost, DynamicArray<int>& chosen,
                            int chosenCount, KnapsackResult& best, SearchTrace* trace = nullptr,
                            int nodeId = 0, int parentId = -1) {
    State state;
    if (trace) {
        state.index = index;
        state.usedVolume = usedVolume;
        state.totalCost = totalCost;
        trace->RecordState(SearchAction::Visit, state, chosen, chosenCount, nodeId, parentId, index, best.totalCost);
    }
    if (index >= items.GetLength()) {
        if (totalCost > best.totalCost) {
            best.totalCost = totalCost;
            best.usedVolume = usedVolume;
            best.itemIndices.Resize(chosenCount);
            for (int i = 0; i < chosenCount; ++i) {
                best.itemIndices[i] = chosen[i];
            }
            if (trace) trace->RecordState(SearchAction::Best, state, chosen, chosenCount, nodeId, parentId, -1, best.totalCost);
        }
        if (trace) trace->RecordState(SearchAction::Leaf, state, chosen, chosenCount, nodeId, parentId, -1, best.totalCost);
        return;
    }

    int skipId = 0;
    if (trace) {
        skipId = trace->nextId++;
        State child = state;
        ++child.index;
        trace->RecordState(SearchAction::Skip, child, chosen, chosenCount, skipId, nodeId, index, best.totalCost);
    }
    SearchBruteForce(items, index + 1, remainingVolume, usedVolume, totalCost,
                     chosen, chosenCount, best, trace, skipId, nodeId);
    if (trace) trace->RecordState(SearchAction::Backtrack, state, chosen, chosenCount, nodeId, parentId, index, best.totalCost);
    const Item& item = items[index];
    if (item.volume <= remainingVolume) {
        chosen[chosenCount] = index;
        int takeId = 0;
        if (trace) {
            takeId = trace->nextId++;
            State child = state;
            ++child.index;
            child.usedVolume += item.volume;
            child.totalCost += item.value;
            trace->RecordState(SearchAction::Take, child, chosen, chosenCount + 1, takeId, nodeId, index, best.totalCost);
        }
        SearchBruteForce(items, index + 1, remainingVolume - item.volume,
                         usedVolume + item.volume, totalCost + item.value,
                         chosen, chosenCount + 1, best, trace, takeId, nodeId);
        if (trace) trace->RecordState(SearchAction::Backtrack, state, chosen, chosenCount, nodeId, parentId, index, best.totalCost);
    } else {
        if (trace) trace->RecordState(SearchAction::Capacity, state, chosen, chosenCount, nodeId, parentId, index, best.totalCost);
    }
}

inline KnapsackResult SolveBruteForce(const DynamicArray<Item>& items, int capacity, SearchTrace* trace = nullptr) {
    ValidateKnapsackInput(items, capacity);
    DynamicArray<int> chosen(items.GetLength());
    KnapsackResult best;
    if (trace) {
        trace->Reset(false);
        State state;
        trace->RecordState(SearchAction::Start, state, chosen, 0, 0, -1, -1, 0);
    }
    SearchBruteForce(items, 0, capacity, 0, 0, chosen, 0, best, trace);
    if (trace) {
        State state;
        state.index = trace->bestNodeId == 0 ? 0 : items.GetLength();
        state.usedVolume = best.usedVolume;
        state.totalCost = best.totalCost;
        trace->RecordState(SearchAction::Finish, state, best.itemIndices, best.itemIndices.GetLength(),
                           trace->bestNodeId, trace->bestParentId, -1, best.totalCost);
    }
    return best;
}

#endif
