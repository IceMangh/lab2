#include "Knapsack.h"
#include "BinaryHeap.h"
#include <charconv>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

std::string ReadText() {
    std::size_t length;
    if (!(std::cin >> length) || length > 4096 || std::cin.get() != '\n') {
        throw std::invalid_argument("Некорректный запрос");
    }
    std::string text(length, '\0');
    if (!std::cin.read(text.data(), static_cast<std::streamsize>(length)) || std::cin.get() != '\n') {
        throw std::invalid_argument("Неполный запрос");
    }
    return text;
}

int ReadInt() {
    std::string text = ReadText();
    int value;
    auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc() || result.ptr != text.data() + text.size()) {
        throw std::invalid_argument("Ожидается целое число в диапазоне int");
    }
    return value;
}

std::string Quote(const std::string& text) {
    std::ostringstream output;
    output << '"';
    for (unsigned char ch : text) {
        if (ch == '"' || ch == '\\') output << '\\' << ch;
        else if (ch < 32) output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
        else output << ch;
    }
    output << '"';
    return output.str();
}

template<typename T>
std::string Text(const T& value) {
    std::ostringstream output;
    output << std::setprecision(15) << value;
    return output.str();
}

void WriteStrings(const std::vector<std::string>& values) {
    std::cout << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << Quote(values[i]);
    }
    std::cout << ']';
}

template<typename T>
void WriteNumbers(const T& values) {
    std::cout << '[';
    bool first = true;
    for (const auto& value : values) {
        if (!first) std::cout << ',';
        first = false;
        std::cout << value;
    }
    std::cout << ']';
}

void WriteResult(const KnapsackResult& result) {
    std::cout << "{\"cost\":" << result.totalCost << ",\"volume\":" << result.usedVolume << ",\"indices\":[";
    for (int i = 0; i < result.itemIndices.GetLength(); ++i) {
        if (i) std::cout << ',';
        std::cout << result.itemIndices[i];
    }
    std::cout << "]}";
}

struct TimedResult {
    KnapsackResult result;
    double medianMs;
};

TimedResult Measure(const DynamicArray<Item>& items, int capacity, bool optimized) {
    std::vector<double> times;
    KnapsackResult result;
    for (int i = 0; i < 5; ++i) {
        auto start = std::chrono::steady_clock::now();
        result = optimized ? SolveOptimized(items, capacity) : SolveBruteForce(items, capacity);
        auto end = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }
    std::sort(times.begin(), times.end());
    return {result, times[2]};
}

void WriteMeasurement(const TimedResult& measured) {
    std::ostringstream buffer;
    std::streambuf* previous = std::cout.rdbuf(buffer.rdbuf());
    WriteResult(measured.result);
    std::cout.rdbuf(previous);
    std::string result = buffer.str();
    result.pop_back();
    std::cout << result << ",\"timeMs\":" << std::setprecision(15) << measured.medianMs << ",\"repetitions\":5}";
}

void RunKnapsack(const std::string& action) {
    std::string method = ReadText();
    int capacity = ReadInt();
    int count = ReadInt();
    if (count < 0 || count > 200 || capacity < 0) throw std::invalid_argument("Некорректные исходные данные");
    if (method != "optimized" && method != "brute" && method != "compare") throw std::invalid_argument("Неизвестный метод поиска");
    if (method == "brute" && count > 28) throw std::invalid_argument("Для полного перебора выберите не более 28 предметов");
    DynamicArray<Item> items(count);
    for (int i = 0; i < count; ++i) {
        int volume = ReadInt();
        int value = ReadInt();
        items[i] = Item(volume, value);
    }
    ValidateKnapsackInput(items, capacity);
    if (action == "trace") {
        if (method == "compare") throw std::invalid_argument("Для показа шагов выберите один алгоритм");
        SearchTrace trace;
        KnapsackResult result = method == "optimized" ? SolveOptimized(items, capacity, &trace) : SolveBruteForce(items, capacity, &trace);
        const char* actions[] = {"start", "visit", "best", "skip", "take", "capacity", "duplicate", "bound", "stale", "leaf", "backtrack", "finish"};
        std::cout << "{\"trace\":{\"steps\":[";
        bool first = true;
        for (const SearchStep& step : trace.steps) {
            if (!first) std::cout << ',';
            first = false;
            std::cout << "{\"type\":" << Quote(actions[static_cast<int>(step.action)])
                << ",\"node\":" << step.nodeId << ",\"parent\":" << step.parentId << ",\"take\":" << (step.take ? "true" : "false")
                << ",\"item\":" << (step.itemIndex < count ? step.itemIndex : -1) << ",\"index\":" << step.state.index
                << ",\"volume\":" << step.state.usedVolume << ",\"cost\":" << step.state.totalCost
                << ",\"bestCost\":" << step.bestCost << ",\"bound\":" << (trace.optimized ? Text(step.bound) : "null")
                << ",\"storedCost\":" << step.storedCost << ",\"number\":" << step.number
                << ",\"visited\":" << step.visited << ",\"pruned\":" << step.pruned
                << ",\"pending\":" << (trace.optimized ? Text(step.pending) : "null") << ",\"path\":";
            WriteNumbers(step.chosen);
            std::cout << '}';
        }
        std::cout << "],\"method\":" << Quote(method) << ",\"truncated\":" << (trace.truncated ? "true" : "false")
            << ",\"totalEvents\":" << trace.totalEvents << ",\"visited\":" << trace.visited << ",\"pruned\":" << trace.pruned
            << ",\"limit\":" << trace.limit << ",\"result\":";
        WriteResult(result);
        std::cout << "}}";
        return;
    }
    TimedResult optimized, brute;
    if (method != "brute") optimized = Measure(items, capacity, true);
    if (method != "optimized" && count <= 28) brute = Measure(items, capacity, false);
    if (method == "compare" && count <= 28 && optimized.result.totalCost != brute.result.totalCost)
        throw std::runtime_error("Алгоритмы вернули разные ответы");
    std::cout << "{\"results\":{";
    if (method != "brute") { std::cout << "\"optimized\":"; WriteMeasurement(optimized); }
    if (method != "optimized") {
        if (method == "compare") std::cout << ',';
        std::cout << "\"brute\":";
        if (count > 28) std::cout << "{\"skipped\":true}";
        else WriteMeasurement(brute);
    }
    std::cout << "}}";
}

template<typename T>
T Value(const std::string& text) {
    if constexpr (std::is_same_v<T, std::string>) return text;
    else if constexpr (std::is_same_v<T, int>) {
        int value;
        auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc() || result.ptr != text.data() + text.size())
            throw std::invalid_argument("Ожидается целое число в диапазоне int");
        return value;
    } else {
        std::size_t end = 0;
        double value;
        try { value = std::stod(text, &end); }
        catch (const std::exception&) { throw std::invalid_argument("Ожидается число double, например 2.5"); }
        if (end != text.size() || !std::isfinite(value)) throw std::invalid_argument("Ожидается конечное число double");
        return value;
    }
}

class CaptureOutput {
    std::ostringstream output;
    std::streambuf* previous = std::cout.rdbuf(output.rdbuf());
public:
    ~CaptureOutput() { std::cout.rdbuf(previous); }
    std::string Text() const {
        std::string text = output.str();
        std::size_t begin = text.find_first_not_of(" \n\r\t");
        return begin == std::string::npos ? "" : text.substr(begin, text.find_last_not_of(" \n\r\t") - begin + 1);
    }
};

struct Operation {
    std::string name, value, key, index, size, priority;
};

template<typename T>
void RunStructures(const std::string& structure) {
    int count = ReadInt();
    if (count < 0 || count > 100) throw std::invalid_argument("Можно выполнить до 100 операций");
    DynamicArray<T> array;
    BinarySearchTree<T> tree;
    HashTable<T, T> table;
    PriorityQueue<T> queue;
    BinaryHeap<int> heap(structure == "minheap");
    std::vector<T> keys;
    std::cout << "{\"outputs\":[";
    for (int i = 0; i < count; ++i) {
        Operation operation{ReadText(), ReadText(), ReadText(), ReadText(), ReadText(), ReadText()};
        const std::string& op = operation.name;
        std::string actual, errorType, detail;
        bool handled = true;
        try {
            if (structure == "array") {
                if (op == "resize") {
                    int size = Value<int>(operation.size);
                    if (size > 128) throw std::invalid_argument("В этой панели можно показать до 128 элементов");
                    array.Resize(size); actual = Text(array.GetLength());
                } else if (op == "set") {
                    int index = Value<int>(operation.index);
                    array.Set(index, Value<T>(operation.value)); actual = Text(array.Get(index));
                } else if (op == "get") actual = Text(array.Get(Value<int>(operation.index)));
                else if (op == "copy") {
                    DynamicArray<T> copy = array;
                    copy.Set(0, Value<T>(operation.value)); actual = Text(array.Get(0));
                } else handled = false;
            } else if (structure == "tree") {
                if (op == "insert") actual = tree.Insert(Value<T>(operation.value)) ? "true" : "false";
                else if (op == "remove") actual = tree.Remove(Value<T>(operation.value)) ? "true" : "false";
                else if (op == "contains") actual = tree.Contains(Value<T>(operation.value)) ? "true" : "false";
                else if (op == "top") actual = Text(tree.GetMax());
                else if (op == "pop") actual = Text(tree.ExtractMax());
                else if (op == "height") actual = Text(tree.GetHeight());
                else if (op == "inorder" || op == "threads" || op == "reverse") {
                    CaptureOutput output;
                    if (op == "inorder") tree.LKP();
                    else if (op == "threads") tree.LKPThreads();
                    else tree.PKLThreads();
                    actual = output.Text();
                } else handled = false;
            } else if (structure == "hash") {
                if (op == "capacity") actual = Text(table.GetCapacity());
                else {
                    T key = Value<T>(operation.key);
                    if (op == "add") {
                        table.Add(key, Value<T>(operation.value));
                        if (std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
                        actual = Text(table.Get(key));
                    } else if (op == "update") {
                        table.Update(key, Value<T>(operation.value)); actual = Text(table.Get(key));
                    } else if (op == "get") actual = Text(table.Get(key));
                    else if (op == "contains") actual = table.ContainsKey(key) ? "true" : "false";
                    else if (op == "remove") { table.Remove(key); actual = "удалено"; }
                    else handled = false;
                }
            } else if (structure == "queue") {
                if (op == "push") { queue.Push(Value<T>(operation.value), Value<int>(operation.priority)); actual = "добавлено"; }
                else if (op == "top") actual = Text(queue.Top());
                else if (op == "pop") actual = Text(queue.Pop());
                else if (op == "empty") actual = queue.IsEmpty() ? "true" : "false";
                else handled = false;
            } else {
                if (op == "push") { heap.Push(Value<double>(operation.value)); actual = "добавлено"; }
                else if (op == "top") actual = Text(heap.Top());
                else if (op == "pop") actual = Text(heap.Pop());
                else if (op == "empty") actual = heap.IsEmpty() ? "true" : "false";
                else handled = false;
            }
            if (!handled) throw std::invalid_argument("Эта операция не поддерживается выбранной структурой");
        } catch (const std::out_of_range& error) { errorType = "std::out_of_range"; detail = error.what(); }
        catch (const std::invalid_argument& error) { errorType = "std::invalid_argument"; detail = error.what(); }
        catch (const std::logic_error& error) { errorType = "std::logic_error"; detail = error.what(); }
        if (!errorType.empty()) actual = errorType;
        if (i) std::cout << ',';
        std::cout << "{\"actual\":" << Quote(actual) << ",\"errorType\":" << Quote(errorType) << ",\"detail\":" << Quote(detail) << '}';
    }
    std::cout << "],\"snapshot\":{";
    std::vector<std::string> values;
    std::string title = "Элементы";
    std::vector<std::pair<std::string, std::size_t>> stats;
    if (structure == "array") {
        for (int i = 0; i < array.GetLength(); ++i) values.push_back(Text(array.Get(i)));
        stats.emplace_back("Размер", array.GetLength());
    } else if (structure == "tree") {
        title = "Значения по возрастанию";
        stats.emplace_back("Высота AVL", tree.GetHeight());
        while (!tree.IsEmpty()) values.insert(values.begin(), Text(tree.ExtractMax()));
        stats.emplace_back("Элементов", values.size());
    } else if (structure == "queue") {
        title = "Порядок извлечения · первый слева";
        while (!queue.IsEmpty()) values.push_back(Text(queue.Pop()));
        stats.emplace_back("Элементов", values.size());
    } else if (structure != "hash") {
        title = "Порядок извлечения · первый слева";
        stats.emplace_back("Размер", heap.GetSize());
        while (!heap.IsEmpty()) values.push_back(Text(heap.Pop()));
    } else {
        title = "Пары по корзинам";
        stats.emplace_back("Пар", table.GetCount());
        stats.emplace_back("Корзин", table.GetCapacity());
    }
    std::cout << "\"values\":";
    WriteStrings(values);
    std::cout << ",\"entries\":[";
    bool first = true;
    for (const T& key : keys) {
        if (!table.ContainsKey(key)) continue;
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"key\":" << Quote(Text(key)) << ",\"value\":" << Quote(Text(table.Get(key)))
            << ",\"bucket\":" << std::hash<T>{}(key) % table.GetCapacity() << '}';
    }
    std::cout << "],\"title\":" << Quote(title) << ",\"stats\":[";
    first = true;
    for (const auto& [label, value] : stats) {
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"label\":" << Quote(label) << ",\"value\":" << value << '}';
    }
    std::cout << "]}}";
}

int main() {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    try {
        std::string action = ReadText();
        if (action == "solve" || action == "trace") RunKnapsack(action);
        else if (action == "structure") {
            std::string structure = ReadText();
            std::string type = ReadText();
            if (structure != "array" && structure != "tree" && structure != "hash" && structure != "queue" && structure != "minheap" && structure != "maxheap")
                throw std::invalid_argument("Неизвестная структура данных");
            if ((structure == "minheap" || structure == "maxheap") && type != "double")
                throw std::invalid_argument("BinaryHeap в этом проекте хранит только double");
            if (type == "int") RunStructures<int>(structure);
            else if (type == "double") RunStructures<double>(structure);
            else if (type == "string") RunStructures<std::string>(structure);
            else throw std::invalid_argument("Неизвестный тип значений");
        } else throw std::invalid_argument("Неизвестный запрос");
    } catch (const std::exception& error) {
        std::cerr << error.what();
        return 1;
    }
}
