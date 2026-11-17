#include "Knapsack.h"
#include <cassert>
#include <initializer_list>
#include <random>
#include <iostream>
#include <limits>

void CheckResult(const DynamicArray<Item>& items, int capacity,
                 const KnapsackResult& result, long long expected) {
    assert(result.totalCost == expected);
    DynamicArray<bool> selected(items.GetLength());
    long long volume = 0;
    long long cost = 0;
    for (int i = 0; i < result.itemIndices.GetLength(); ++i) {
        int index = result.itemIndices[i];
        assert(index >= 0);
        assert(index < items.GetLength());
        assert(!selected[index]);
        selected[index] = true;
        volume += items[index].volume;
        cost += items[index].value;
    }
    assert(volume == result.usedVolume);
    assert(cost == result.totalCost);
    assert(volume <= capacity);
}

void TestKnownAnswers() {
    struct TestCase {
        const char* name;
        std::initializer_list<Item> items;
        int capacity;
        long long expected;
    };
    int maximum = std::numeric_limits<int>::max();
    int minimum = std::numeric_limits<int>::min();
    TestCase cases[] = {
        {"Пример с вместимостью 7", {{3, 4}, {4, 5}, {5, 8}}, 7, 9},
        {"Пример с вместимостью 5", {{3, 4}, {4, 5}, {5, 8}}, 5, 8},
        {"Все предметы помещаются", {{3, 4}, {4, 5}, {5, 8}}, 12, 17},
        {"Два меньших предмета выгоднее одного", {{6, 12}, {5, 10}, {5, 10}}, 10, 20},
        {"Пустой набор", {}, 10, 0},
        {"Пустой набор и нулевая вместимость", {}, 0, 0},
        {"Нулевая вместимость", {{1, 5}, {2, 9}}, 0, 0},
        {"Ни один предмет не помещается", {{5, 8}, {6, 20}}, 4, 0},
        {"Предмет помещается в точности", {{3, 7}}, 3, 7},
        {"Предмет не помещается", {{3, 7}}, 2, 0},
        {"Предмет нельзя взять дважды", {{2, 3}}, 6, 3},
        {"Отрицательные стоимости", {{1, -5}, {2, -1}}, 3, 0},
        {"Положительные и отрицательные стоимости", {{1, -10}, {2, 5}, {3, 8}}, 5, 13},
        {"Нулевые стоимости", {{1, 0}, {2, 0}}, 3, 0},
        {"Предметы с нулевым объёмом", {{0, 5}, {0, 7}, {1, 2}}, 0, 12},
        {"Нулевой объём с разными стоимостями", {{0, -4}, {0, 6}, {2, 5}}, 2, 11},
        {"Одинаковые предметы можно выбирать отдельно", {{2, 3}, {2, 3}, {2, 3}}, 4, 6},
        {"Улучшение стоимости для того же ключа", {{2, 1}, {2, 10}, {1, 2}}, 3, 12},
        {"Одинаковая стоимость для того же ключа", {{2, 5}, {2, 5}, {1, 2}}, 3, 7},
        {"Улучшение за счёт предмета с нулевым объёмом", {{0, 1}, {1, 10}}, 1, 11},
        {"Несколько оптимальных наборов", {{1, 5}, {1, 5}}, 1, 5},
        {"Большая верхняя оценка", {{1, maximum}, {1, 1}}, 1, maximum},
        {"Стоимость больше диапазона int", {{1, maximum}, {1, 1}}, 2, static_cast<long long>(maximum) + 1},
        {"Предмет с минимальной стоимостью int", {{1, minimum}, {1, 10}}, 1, 10},
    };
    for (const TestCase& test : cases) {
        std::cout << test.name << std::endl;
        DynamicArray<Item> items(test.items.begin(), static_cast<int>(test.items.size()));
        KnapsackResult brute = SolveBruteForce(items, test.capacity);
        KnapsackResult optimized = SolveOptimized(items, test.capacity);
        CheckResult(items, test.capacity, brute, test.expected);
        CheckResult(items, test.capacity, optimized, test.expected);
    }
    std::cout << "[OK] 24 задачи с известным ответом\n";
}

void TestRandomCases() {
    std::mt19937 random(42);
    for (int test = 0; test < 2000; ++test) {
        int count = static_cast<int>(random() % 13);
        int capacity = static_cast<int>(random() % 26);
        DynamicArray<Item> items(count);
        for (int i = 0; i < count; ++i) {
            int volume = static_cast<int>(random() % 11);
            int value = static_cast<int>(random() % 31) - 10;
            items[i] = Item(volume, value);
        }
        KnapsackResult brute = SolveBruteForce(items, capacity);
        long long expected = brute.totalCost;
        KnapsackResult optimized = SolveOptimized(items, capacity);
        CheckResult(items, capacity, optimized, expected);
        CheckResult(items, capacity, brute, expected);
    }
    std::cout << "[OK] 2000 случайных задач\n";
}

void TestLargerCases() {
    for (int test = 0; test < 30; ++test) {
        std::mt19937 random(100 + test);
        DynamicArray<Item> items(16 + test % 5);
        for (int i = 0; i < items.GetLength(); ++i) {
            int volume = 1 + static_cast<int>(random() % 15);
            int value = 1 + static_cast<int>(random() % 30);
            items[i] = Item(volume, value);
        }
        int capacity = 30 + test;
        KnapsackResult brute = SolveBruteForce(items, capacity);
        long long expected = brute.totalCost;
        KnapsackResult optimized = SolveOptimized(items, capacity);
        CheckResult(items, capacity, optimized, expected);
        CheckResult(items, capacity, brute, expected);
    }
    std::cout << "[OK] 30 задач на 16–20 предметах\n";
}

void TestSearchTrace() {
    Item values[] = {{2, 3}, {2, 8}, {3, 9}, {1, 4}};
    DynamicArray<Item> items(values, 4);
    for (bool optimized : {true, false}) {
        SearchTrace trace;
        KnapsackResult result = optimized ? SolveOptimized(items, 4, &trace) : SolveBruteForce(items, 4, &trace);
        CheckResult(items, 4, result, 13);
        assert(!trace.truncated);
        assert(trace.steps.front().action == SearchAction::Start);
        assert(trace.steps.back().action == SearchAction::Finish);
        assert(trace.steps.back().bestCost == 13);
        assert(trace.totalEvents == trace.steps.size());
        std::size_t visited = 0;
        std::size_t pruned = 0;
        long long bestCost = 0;
        for (const SearchStep& step : trace.steps) {
            assert(step.bestCost >= bestCost);
            bestCost = step.bestCost;
            DynamicArray<bool> seen(4);
            long long volume = 0;
            long long cost = 0;
            for (int index : step.chosen) {
                assert(index >= 0 && index < 4);
                assert(!seen[index]);
                seen[index] = true;
                volume += items[index].volume;
                cost += items[index].value;
            }
            assert(volume == step.state.usedVolume);
            assert(cost == step.state.totalCost);
            assert(volume <= 4);
            if (step.action == SearchAction::Visit) ++visited;
            if (step.action == SearchAction::Capacity || step.action == SearchAction::Duplicate
                || step.action == SearchAction::Bound || step.action == SearchAction::Stale) ++pruned;
            if (step.action == SearchAction::Capacity) {
                assert(volume + items[step.itemIndex].volume > 4);
            }
            if (step.action == SearchAction::Bound) assert(step.bound <= step.bestCost);
            if (step.action == SearchAction::Duplicate) assert(step.storedCost >= cost);
            if (step.action == SearchAction::Stale) assert(step.storedCost > cost);
            if (step.action == SearchAction::Best) assert(cost == step.bestCost);
        }
        assert(visited == trace.visited);
        assert(pruned == trace.pruned);
        if (optimized) assert(trace.pending == 0);

        SearchTrace shortTrace(2);
        KnapsackResult limited = optimized ? SolveOptimized(items, 4, &shortTrace) : SolveBruteForce(items, 4, &shortTrace);
        CheckResult(items, 4, limited, result.totalCost);
        assert(shortTrace.truncated);
        assert(shortTrace.steps.size() == 3);
        assert(shortTrace.steps.back().action == SearchAction::Finish);
        assert(shortTrace.steps.back().bestCost == 13);
        assert(shortTrace.visited == trace.visited);
        assert(shortTrace.pruned == trace.pruned);

        DynamicArray<Item> empty;
        KnapsackResult emptyResult = optimized ? SolveOptimized(empty, 0, &trace) : SolveBruteForce(empty, 0, &trace);
        assert(emptyResult.totalCost == 0);
        assert(trace.visited == 1);
        assert(trace.pruned == 0);
        assert(trace.steps.size() == 4);
        assert(!trace.truncated);
    }
    Item duplicateValues[] = {{0, 0}, {1, 2}};
    DynamicArray<Item> duplicates(duplicateValues, 2);
    SearchTrace duplicateTrace;
    SolveOptimized(duplicates, 1, &duplicateTrace);
    bool rejectedDuplicate = false;
    for (const SearchStep& step : duplicateTrace.steps) {
        if (step.action == SearchAction::Duplicate) {
            rejectedDuplicate = true;
            assert(step.storedCost >= step.state.totalCost);
        }
    }
    assert(rejectedDuplicate);
    std::cout << "[OK] Реальные шаги поиска и ограничение записи\n";
}

void TestInvalidInputs() {
    DynamicArray<Item> items(1);
    items[0] = Item(1, 5);
    bool caught = false;
    try {
        SolveOptimized(items, -1);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    caught = false;
    try {
        SolveBruteForce(items, -1);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);

    items[0].volume = -1;
    caught = false;
    try {
        SolveOptimized(items, 5);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    caught = false;
    try {
        SolveBruteForce(items, 5);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    std::cout << "[OK] Неверная вместимость и объём отклоняются\n";
}

int main() {
    try {
        TestKnownAnswers();
        TestRandomCases();
        TestLargerCases();
        TestSearchTrace();
        TestInvalidInputs();
        std::cout << "Все проверки неверных данных пройдены.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr  << error.what() << '\n';
        return 1;
    }
}
