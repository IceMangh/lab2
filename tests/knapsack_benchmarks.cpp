#include "Knapsack.h"
#include <cstddef>
#include <random>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <vector>

namespace Heap {
    struct alignas(std::max_align_t) Header {
        std::size_t bytes;
    };

    std::size_t live = 0;
    std::size_t peak = 0;
    std::size_t allocations = 0;

    void* Allocate(std::size_t bytes) {
        if (bytes > std::numeric_limits<std::size_t>::max() - sizeof(Header)) {
            throw std::bad_alloc();
        }
        auto* header = static_cast<Header*>(std::malloc(sizeof(Header) + bytes));
        if (!header) {
            throw std::bad_alloc();
        }
        header->bytes = bytes;
        live += bytes;
        peak = std::max(peak, live);
        ++allocations;
        return header + 1;
    }

    void Free(void* memory) noexcept {
        if (memory) {
            auto* header = static_cast<Header*>(memory) - 1;
            live -= header->bytes;
            std::free(header);
        }
    }
}

void* operator new(std::size_t bytes) { return Heap::Allocate(bytes); }
void* operator new[](std::size_t bytes) { return Heap::Allocate(bytes); }
void operator delete(void* memory) noexcept { Heap::Free(memory); }
void operator delete[](void* memory) noexcept { Heap::Free(memory); }
#if defined(__cpp_sized_deallocation)
void operator delete(void* memory, std::size_t) noexcept { Heap::Free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { Heap::Free(memory); }
#endif

struct Measurement {
    long long answer;
    double medianMs;
};

Measurement Measure(const DynamicArray<Item>& items, int capacity,
                    int repetitions, bool optimized) {
    std::string name = optimized ? "optimized" : "brute force";
    std::vector<double> times(repetitions);
    std::size_t peakBytes = 0;
    std::size_t allocationCount = 0;
    long long firstAnswer = 0;

    for (int run = 0; run < repetitions; ++run) {
        std::size_t baseline = Heap::live;
        Heap::peak = baseline;
        Heap::allocations = 0;
        auto start = std::chrono::steady_clock::now();
        long long answer;
        {
            KnapsackResult solution = optimized
                ? SolveOptimized(items, capacity)
                : SolveBruteForce(items, capacity);
            answer = solution.totalCost;
        }
        auto end = std::chrono::steady_clock::now();
        std::size_t used = Heap::peak - baseline;
        bool memoryReleased = Heap::live == baseline;
        allocationCount = std::max(allocationCount, Heap::allocations);
        peakBytes = std::max(peakBytes, used);
        times[run] = std::chrono::duration<double, std::milli>(end - start).count();

        if (run == 0) {
            firstAnswer = answer;
        } else if (answer != firstAnswer) {
            throw std::runtime_error(name + ": ответы повторных запусков различаются");
        }
        if (!memoryReleased) {
            throw std::runtime_error(name + ": не вся учтённая память освобождена");
        }
    }

    std::sort(times.begin(), times.end());
    double median = times[repetitions / 2];
    if (repetitions % 2 == 0) {
        median = (times[repetitions / 2 - 1] + median) / 2;
    }
    std::cout << std::left << std::setw(12) << name
              << " value=" << firstAnswer
              << " median_ms=" << std::fixed << std::setprecision(3) << median
              << " peak_heap_KiB=" << static_cast<double>(peakBytes) / 1024
              << " allocations=" << allocationCount << " retained_bytes=0\n" << std::flush;
    return {firstAnswer, median};
}

int ReadNumber(const std::string& text) {
    std::size_t end;
    int value = std::stoi(text, &end);
    if (end != text.size() || value < 0) {
        throw std::invalid_argument("Ожидалось неотрицательное целое число: " + text);
    }
    return value;
}

int main(int argc, char** argv) {
    try {
        int numbers[3] = {0, 0, 5};
        int numberCount = 0;
        bool forceBrute = false;
        bool skipBrute = false;
        for (int i = 1; i < argc; ++i) {
            std::string argument = argv[i];
            if (argument == "--help") {
                std::cout << "Usage: knapsack_benchmarks ITEMS [CAPACITY] [REPETITIONS]"
                             " [--brute-force|--no-brute]\n"
                             "Example: knapsack_benchmarks 40\n"
                             "Defaults: capacity = ITEMS * 4, repetitions = 5.\n"
                             "Without arguments, enter the item count in the console.\n"
                             "Brute force runs automatically for up to 32 items.\n";
                return 0;
            } else if (argument == "--brute-force") {
                forceBrute = true;
            } else if (argument == "--no-brute") {
                skipBrute = true;
            } else {
                if (numberCount == 3) {
                    throw std::invalid_argument("Нужны только количество предметов, вместимость и число повторов");
                }
                numbers[numberCount] = ReadNumber(argument);
                ++numberCount;
            }
        }
        if (forceBrute && skipBrute) {
            throw std::invalid_argument("Нельзя одновременно указать --brute-force и --no-brute");
        }

        if (numberCount == 0) {
            std::cout << "Количество предметов: " << std::flush;
            std::string input;
            if (!(std::cin >> input)) {
                throw std::invalid_argument("Введите количество предметов, например 40");
            }
            numbers[0] = ReadNumber(input);
        }
        int count = numbers[0];
        if (count < 1 || count > std::numeric_limits<int>::max() / 4) {
            throw std::invalid_argument("Недопустимое количество предметов");
        }
        int capacity = numberCount >= 2 ? numbers[1] : count * 4;
        int repetitions = numbers[2];
        if (repetitions < 1) {
            throw std::invalid_argument("Число повторов должно быть положительным");
        }
        bool compareBruteForce = !skipBrute && (count <= 32 || forceBrute);

        std::mt19937 random(2026);
        DynamicArray<Item> items(count);
        for (int i = 0; i < count; ++i) {
            int volume = 1 + static_cast<int>(random() % 15);
            int value = 1 + static_cast<int>(random() % 30);
            items[i] = Item(volume, value);
        }
        std::cout << "items=" << count << " capacity=" << capacity
                  << " repetitions=" << repetitions << '\n' << std::flush;
        Measurement optimized = Measure(items, capacity, repetitions, true);
        if (compareBruteForce) {
            std::cout << "Running brute force...\n" << std::flush;
            Measurement brute = Measure(items, capacity, repetitions, false);
            if (optimized.answer != brute.answer) {
                throw std::runtime_error("Ответы методов различаются");
            }
            std::cout << "Answers match.\n";
            if (optimized.medianMs > 0) {
                std::cout << "brute_force_ms / optimized_ms = " << brute.medianMs / optimized.medianMs << '\n';
            }
        } else if (skipBrute) {
            std::cout << "brute force: skipped (--no-brute)\n";
        } else {
            std::cout << "brute force: skipped for more than 32 items; add --brute-force to include it\n";
        }
        if (!compareBruteForce) {
            std::cout << "No comparison with brute force was performed.\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
