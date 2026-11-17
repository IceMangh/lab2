#include "Knapsack.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

int ReadInt(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw std::runtime_error("Ввод завершён.");
        }
        std::istringstream input(line);
        long long value;
        char extra;
        if ((input >> value) && !(input >> extra) && value >= minimum && value <= maximum) {
            return static_cast<int>(value);
        }
        std::cout << "Введите целое число от " << minimum << " до " << maximum << ".\n";
    }
}

Item ReadItem(int number) {
    while (true) {
        std::cout << "Предмет " << number << " — объём и стоимость через пробел: " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw std::runtime_error("Ввод завершён.");
        }
        std::istringstream input(line);
        long long volume;
        long long value;
        char extra;
        if ((input >> volume >> value) && !(input >> extra)
            && volume >= 0 && volume <= std::numeric_limits<int>::max()
            && value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
            return Item(static_cast<int>(volume), static_cast<int>(value));
        }
        std::cout << "Нужны два целых числа: объём >= 0 и стоимость в диапазоне int.\n";
    }
}

void PrintItems(const DynamicArray<Item>& items, int capacity) {
    std::cout << "\nИсходные данные. Предметов: " << items.GetLength()
              << ", вместимость: " << capacity << "\n";
    std::cout << "Номер\tОбъём\tСтоимость\n";
    for (int i = 0; i < items.GetLength(); ++i) {
        std::cout << i + 1 << '\t' << items[i].volume << '\t' << items[i].value << '\n';
    }
    if (items.GetLength() == 0) {
        std::cout << "Список предметов пуст.\n";
    }
}

void PrintResult(const DynamicArray<Item>& items, int capacity, const KnapsackResult& result) {
    std::cout << "Выбранные предметы:\nНомер\tОбъём\tСтоимость\n";
    for (int i = 0; i < result.itemIndices.GetLength(); ++i) {
        int index = result.itemIndices[i];
        std::cout << index + 1 << '\t' << items[index].volume << '\t' << items[index].value << '\n';
    }
    if (result.itemIndices.GetLength() == 0) {
        std::cout << "Пустой набор.\n";
    }
    std::cout << "Общий объём: " << result.usedVolume << " / " << capacity << '\n'
              << "Общая стоимость: " << result.totalCost << '\n';
}

int main() {
    const int maxInt = std::numeric_limits<int>::max();
    DynamicArray<Item> items;
    int capacity = 0;
    bool hasData = false;
    std::cout << "Рюкзак: максимальная стоимость при ограничении объёма.\n";

    while (true) {
        try {
            std::cout << "\n1. Ввести предметы вручную\n"
                         "2. Сгенерировать предметы\n"
                         "3. Показать исходные данные\n"
                         "4. Полный перебор\n"
                         "5. Оптимизированный поиск\n"
                         "6. Сравнить оба метода\n"
                         "0. Выход\n";
            int action = ReadInt("Выбор: ", 0, 6);
            if (action == 0) {
                return 0;
            }
            if (action == 1 || action == 2) {
                int count = ReadInt("Количество предметов: ", 0, maxInt);
                int newCapacity = ReadInt("Вместимость рюкзака: ", 0, maxInt);
                DynamicArray<Item> newItems(count);
                if (action == 1) {
                    for (int i = 0; i < count; ++i) {
                        newItems[i] = ReadItem(i + 1);
                    }
                } else {
                    int maxVolume = ReadInt("Максимальный объём предмета: ", 1, maxInt);
                    int maxValue = ReadInt("Максимальная стоимость предмета: ", 1, maxInt);
                    int seed = ReadInt("Число для генератора: ", 0, maxInt);
                    std::mt19937 random(static_cast<unsigned>(seed));
                    std::uniform_int_distribution<int> volumes(1, maxVolume);
                    std::uniform_int_distribution<int> values(1, maxValue);
                    for (int i = 0; i < count; ++i) {
                        int volume = volumes(random);
                        int value = values(random);
                        newItems[i] = Item(volume, value);
                    }
                }
                items.Swap(newItems);
                capacity = newCapacity;
                hasData = true;
                PrintItems(items, capacity);
                continue;
            }
            if (!hasData) {
                std::cout << "Сначала введите или сгенерируйте предметы.\n";
                continue;
            }
            PrintItems(items, capacity);
            long long bruteCost = 0;
            long long optimizedCost = 0;
            double bruteTime = 0;
            double optimizedTime = 0;
            std::cout << std::fixed << std::setprecision(3);

            if (action == 4 || action == 6) {
                std::cout << "\nПолный перебор...\n" << std::flush;
                auto start = std::chrono::steady_clock::now();
                KnapsackResult result = SolveBruteForce(items, capacity);
                auto finish = std::chrono::steady_clock::now();
                bruteTime = std::chrono::duration<double, std::milli>(finish - start).count();
                bruteCost = result.totalCost;
                PrintResult(items, capacity, result);
                std::cout << "Время выполнения: " << bruteTime << " мс\n";
            }
            if (action == 5 || action == 6) {
                std::cout << "\nОптимизированный поиск...\n" << std::flush;
                auto start = std::chrono::steady_clock::now();
                KnapsackResult result = SolveOptimized(items, capacity);
                auto finish = std::chrono::steady_clock::now();
                optimizedTime = std::chrono::duration<double, std::milli>(finish - start).count();
                optimizedCost = result.totalCost;
                PrintResult(items, capacity, result);
                std::cout << "Время выполнения: " << optimizedTime << " мс\n";
            }
            if (action == 6) {
                if (optimizedCost == bruteCost) {
                    std::cout << "Стоимость совпадает: " << optimizedCost << ".\n";
                } else {
                    std::cout << "Ошибка: методы вернули разную стоимость.\n";
                }
                if (optimizedTime > 0) {
                    std::cout << "Время полного перебора / время оптимизированного поиска: "
                              << bruteTime / optimizedTime << '\n';
                }
            }
        } catch (const std::exception& error) {
            if (std::cin.eof()) {
                std::cout << "\nВвод завершён.\n";
                return 0;
            }
            std::cerr << "Ошибка: " << error.what() << '\n';
        }
    }
}
