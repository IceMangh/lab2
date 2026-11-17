#pragma once

#import <Foundation/Foundation.h>
#include "BinaryHeap.h"
#include "BinarySearchTree.h"
#include "HashTable.h"
#include "PriorityQueue.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

static NSString* LabText(const std::string& value) {
    return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
}

template<typename T>
static NSString* LabText(const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        return LabText(value);
    } else {
        std::ostringstream output;
        output << std::setprecision(15) << value;
        return LabText(output.str());
    }
}

template<typename T>
static T LabValue(id input) {
    if (![input isKindOfClass:NSString.class] || [input length] > 80) {
        throw std::invalid_argument("Введите значение длиной до 80 символов");
    }
    std::string text([input UTF8String], [input lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
    if constexpr (std::is_same_v<T, std::string>) {
        return text;
    } else if constexpr (std::is_same_v<T, int>) {
        int value;
        auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc() || result.ptr != text.data() + text.size()) {
            throw std::invalid_argument("Ожидается целое число в диапазоне int");
        }
        return value;
    } else {
        std::size_t end = 0;
        double value;
        try {
            value = std::stod(text, &end);
        } catch (const std::exception&) {
            throw std::invalid_argument("Ожидается число double, например 2.5");
        }
        if (end != text.size() || !std::isfinite(value)) {
            throw std::invalid_argument("Ожидается конечное число double, например 2.5");
        }
        return value;
    }
}

class LabOutput {
    std::ostringstream output;
    std::streambuf* previous = std::cout.rdbuf(output.rdbuf());
public:
    ~LabOutput() { std::cout.rdbuf(previous); }
    NSString* Text() const {
        return [LabText(output.str()) stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    }
};

static NSDictionary* LabCheck(NSString* op, NSDictionary* arguments, NSString* expected,
                              NSString* error = nil) {
    NSMutableDictionary* result = [arguments mutableCopy];
    result[@"op"] = op;
    result[@"expect"] = expected;
    if (error) result[@"throws"] = error;
    return result;
}

template<typename T>
static NSArray* LabChecks(NSString* structure) {
    T first, second, third;
    if constexpr (std::is_same_v<T, std::string>) {
        first = "apple"; second = "pear"; third = "plum";
    } else {
        first = static_cast<T>(2); second = static_cast<T>(4); third = static_cast<T>(6);
        if constexpr (std::is_same_v<T, double>) {
            first = 1.25; second = 2.5; third = 3.75;
        }
    }
    NSString* a = LabText(first);
    NSString* b = LabText(second);
    NSString* c = LabText(third);
    NSMutableArray* checks = [NSMutableArray array];
    if ([structure isEqualToString:@"array"]) {
        [checks addObjectsFromArray:@[
            LabCheck(@"resize", @{@"size": @"3"}, @"3"),
            LabCheck(@"set", @{@"index": @"0", @"value": a}, a),
            LabCheck(@"set", @{@"index": @"1", @"value": b}, b),
            LabCheck(@"get", @{@"index": @"0"}, a),
            LabCheck(@"resize", @{@"size": @"5"}, @"5"),
            LabCheck(@"get", @{@"index": @"1"}, b),
            LabCheck(@"get", @{@"index": @"4"}, LabText(T{})),
            LabCheck(@"copy", @{@"value": c}, a),
            LabCheck(@"resize", @{@"size": @"1"}, @"1"),
            LabCheck(@"get", @{@"index": @"1"}, @"std::out_of_range", @"std::out_of_range"),
            LabCheck(@"resize", @{@"size": @"-1"}, @"std::invalid_argument", @"std::invalid_argument"),
            LabCheck(@"get", @{@"index": @"0"}, a),
            LabCheck(@"resize", @{@"size": @"0"}, @"0"),
            LabCheck(@"get", @{@"index": @"0"}, @"std::out_of_range", @"std::out_of_range")
        ]];
    } else if ([structure isEqualToString:@"tree"]) {
        NSString* ascending = [@[a, b, c] componentsJoinedByString:@" "];
        NSString* descending = [@[c, b, a] componentsJoinedByString:@" "];
        [checks addObjectsFromArray:@[
            LabCheck(@"insert", @{@"value": a}, @"true"),
            LabCheck(@"insert", @{@"value": b}, @"true"),
            LabCheck(@"insert", @{@"value": c}, @"true"),
            LabCheck(@"height", @{}, @"2"),
            LabCheck(@"insert", @{@"value": b}, @"false"),
            LabCheck(@"contains", @{@"value": b}, @"true"),
            LabCheck(@"inorder", @{}, ascending),
            LabCheck(@"threads", @{}, ascending),
            LabCheck(@"reverse", @{}, descending),
            LabCheck(@"remove", @{@"value": b}, @"true"),
            LabCheck(@"contains", @{@"value": b}, @"false"),
            LabCheck(@"threads", @{}, [@[a, c] componentsJoinedByString:@" "]),
            LabCheck(@"insert", @{@"value": b}, @"true"),
            LabCheck(@"pop", @{}, c),
            LabCheck(@"top", @{}, b),
            LabCheck(@"remove", @{@"value": a}, @"true"),
            LabCheck(@"remove", @{@"value": b}, @"true"),
            LabCheck(@"top", @{}, @"std::out_of_range", @"std::out_of_range"),
            LabCheck(@"threads", @{}, @"")
        ]];
    } else if ([structure isEqualToString:@"hash"]) {
        [checks addObjectsFromArray:@[
            LabCheck(@"add", @{@"key": a, @"value": b}, b),
            LabCheck(@"get", @{@"key": a}, b),
            LabCheck(@"update", @{@"key": a, @"value": c}, c),
            LabCheck(@"get", @{@"key": a}, c),
            LabCheck(@"contains", @{@"key": a}, @"true"),
            LabCheck(@"add", @{@"key": a, @"value": a}, @"std::invalid_argument", @"std::invalid_argument")
        ]];
        std::vector<T> keys;
        for (int i = 0; i < 8; ++i) {
            T key;
            if constexpr (std::is_same_v<T, std::string>) key = "key" + std::to_string(i);
            else key = static_cast<T>(100 + i * 8);
            keys.push_back(key);
            [checks addObject:LabCheck(@"add", @{@"key": LabText(key), @"value": b}, b)];
        }
        [checks addObject:LabCheck(@"capacity", @{}, @"16")];
        [checks addObject:LabCheck(@"get", @{@"key": LabText(keys.front())}, b)];
        for (const T& key : keys) {
            [checks addObject:LabCheck(@"remove", @{@"key": LabText(key)}, @"удалено")];
        }
        [checks addObjectsFromArray:@[
            LabCheck(@"capacity", @{}, @"8"),
            LabCheck(@"get", @{@"key": a}, c),
            LabCheck(@"remove", @{@"key": a}, @"удалено"),
            LabCheck(@"get", @{@"key": a}, @"std::out_of_range", @"std::out_of_range"),
            LabCheck(@"remove", @{@"key": a}, @"std::out_of_range", @"std::out_of_range")
        ]];
    } else if ([structure isEqualToString:@"queue"]) {
        [checks addObjectsFromArray:@[
            LabCheck(@"top", @{}, @"std::out_of_range", @"std::out_of_range"),
            LabCheck(@"push", @{@"value": a, @"priority": @"1"}, @"добавлено"),
            LabCheck(@"push", @{@"value": b, @"priority": @"3"}, @"добавлено"),
            LabCheck(@"push", @{@"value": c, @"priority": @"3"}, @"добавлено"),
            LabCheck(@"top", @{}, c),
            LabCheck(@"pop", @{}, c),
            LabCheck(@"pop", @{}, b),
            LabCheck(@"pop", @{}, a),
            LabCheck(@"pop", @{}, @"std::out_of_range", @"std::out_of_range"),
            LabCheck(@"empty", @{}, @"true")
        ]];
    } else {
        BOOL minHeap = [structure isEqualToString:@"minheap"];
        NSString* low = minHeap ? a : c;
        NSString* high = minHeap ? c : a;
        [checks addObjectsFromArray:@[
            LabCheck(@"top", @{}, @"std::logic_error", @"std::logic_error"),
            LabCheck(@"push", @{@"value": c}, @"добавлено"),
            LabCheck(@"push", @{@"value": a}, @"добавлено"),
            LabCheck(@"push", @{@"value": b}, @"добавлено"),
            LabCheck(@"top", @{}, low),
            LabCheck(@"pop", @{}, low),
            LabCheck(@"pop", @{}, b),
            LabCheck(@"pop", @{}, high),
            LabCheck(@"pop", @{}, @"std::logic_error", @"std::logic_error"),
            LabCheck(@"push", @{@"value": @"-2.5"}, @"добавлено"),
            LabCheck(@"push", @{@"value": @"-2.5"}, @"добавлено"),
            LabCheck(@"pop", @{}, @"-2.5"),
            LabCheck(@"pop", @{}, @"-2.5"),
            LabCheck(@"empty", @{}, @"true")
        ]];
    }
    return checks;
}

static NSString* LabCall(NSString* structure, NSDictionary* operation) {
    NSString* op = operation[@"op"];
    NSString* value = operation[@"value"] ?: @"";
    NSString* key = operation[@"key"] ?: @"";
    if ([op isEqualToString:@"resize"]) return [NSString stringWithFormat:@"Resize(%@)", operation[@"size"]];
    if ([op isEqualToString:@"set"]) return [NSString stringWithFormat:@"Set(%@, %@)", operation[@"index"], value];
    if ([op isEqualToString:@"copy"]) return @"Копия: изменить [0], проверить оригинал";
    if ([op isEqualToString:@"get"] && [structure isEqualToString:@"array"])
        return [NSString stringWithFormat:@"Get(%@)", operation[@"index"]];
    if ([op isEqualToString:@"add"] || [op isEqualToString:@"update"])
        return [NSString stringWithFormat:@"%@(%@, %@)", [op isEqualToString:@"add"] ? @"Add" : @"Update", key, value];
    if ([op isEqualToString:@"push"] && [structure isEqualToString:@"queue"])
        return [NSString stringWithFormat:@"Push(%@, %@)", value, operation[@"priority"]];
    NSDictionary* names = @{@"insert": @"Insert", @"remove": @"Remove", @"get": @"Get",
        @"contains": [structure isEqualToString:@"hash"] ? @"ContainsKey" : @"Contains", @"push": @"Push"};
    if (names[op]) return [NSString stringWithFormat:@"%@(%@)", names[op], [structure isEqualToString:@"hash"] ? key : value];
    NSDictionary* calls = @{@"height": @"GetHeight()", @"inorder": @"LKP()",
        @"threads": @"LKPThreads()", @"reverse": @"PKLThreads()", @"capacity": @"GetCapacity()",
        @"empty": @"IsEmpty()", @"top": [structure isEqualToString:@"tree"] ? @"GetMax()" : @"Top()",
        @"pop": [structure isEqualToString:@"tree"] ? @"ExtractMax()" : @"Pop()"};
    return calls[op] ?: op;
}

template<typename T>
static NSDictionary* RunTypedLab(NSDictionary* request) {
    NSString* structure = request[@"structure"];
    BOOL tests = [request[@"mode"] isEqualToString:@"tests"];
    NSArray* operations = tests ? LabChecks<T>(structure) : request[@"operations"];
    if (![operations isKindOfClass:NSArray.class] || operations.count > 100) {
        throw std::invalid_argument("Можно выполнить до 100 операций. Очистите историю, чтобы начать заново.");
    }
    NSUInteger count = operations.count;
    if (tests && request[@"count"]) {
        int requested = [request[@"count"] intValue];
        count = static_cast<NSUInteger>(std::clamp(requested, 0, static_cast<int>(count)));
    }
    DynamicArray<T> array;
    BinarySearchTree<T> tree;
    HashTable<T, T> table;
    PriorityQueue<T> queue;
    BinaryHeap heap([structure isEqualToString:@"minheap"]);
    std::vector<T> keys;
    NSMutableArray* steps = [NSMutableArray array];
    NSMutableArray* plan = [NSMutableArray array];
    NSMutableArray* expectations = [NSMutableArray array];
    for (NSDictionary* operation in operations) {
        if (![operation isKindOfClass:NSDictionary.class] || ![operation[@"op"] isKindOfClass:NSString.class]) {
            throw std::invalid_argument("Некорректная операция");
        }
        [plan addObject:LabCall(structure, operation)];
        [expectations addObject:operation[@"expect"] ?: NSNull.null];
    }
    int passed = 0, failed = 0;
    for (NSUInteger i = 0; i < count; ++i) {
        NSDictionary* operation = operations[i];
        NSString* op = operation[@"op"];
        NSString* actual = nil;
        NSString* errorType = nil;
        NSString* detail = @"";
        try {
            if ([structure isEqualToString:@"array"]) {
                if ([op isEqualToString:@"resize"]) {
                    int size = LabValue<int>(operation[@"size"]);
                    if (size > 128) throw std::invalid_argument("В этой панели можно показать до 128 элементов");
                    array.Resize(size); actual = LabText(array.GetLength());
                } else if ([op isEqualToString:@"set"]) {
                    int index = LabValue<int>(operation[@"index"]);
                    array.Set(index, LabValue<T>(operation[@"value"])); actual = LabText(array.Get(index));
                } else if ([op isEqualToString:@"get"]) {
                    actual = LabText(array.Get(LabValue<int>(operation[@"index"])));
                } else if ([op isEqualToString:@"copy"]) {
                    DynamicArray<T> copy = array;
                    copy.Set(0, LabValue<T>(operation[@"value"])); actual = LabText(array.Get(0));
                }
            } else if ([structure isEqualToString:@"tree"]) {
                if ([op isEqualToString:@"insert"]) actual = tree.Insert(LabValue<T>(operation[@"value"])) ? @"true" : @"false";
                else if ([op isEqualToString:@"remove"]) actual = tree.Remove(LabValue<T>(operation[@"value"])) ? @"true" : @"false";
                else if ([op isEqualToString:@"contains"]) actual = tree.Contains(LabValue<T>(operation[@"value"])) ? @"true" : @"false";
                else if ([op isEqualToString:@"top"]) actual = LabText(tree.GetMax());
                else if ([op isEqualToString:@"pop"]) actual = LabText(tree.ExtractMax());
                else if ([op isEqualToString:@"height"]) actual = LabText(tree.GetHeight());
                else if ([op isEqualToString:@"inorder"] || [op isEqualToString:@"threads"] || [op isEqualToString:@"reverse"]) {
                    LabOutput output;
                    if ([op isEqualToString:@"inorder"]) tree.LKP();
                    else if ([op isEqualToString:@"threads"]) tree.LKPThreads();
                    else tree.PKLThreads();
                    actual = output.Text();
                }
            } else if ([structure isEqualToString:@"hash"]) {
                if ([op isEqualToString:@"capacity"]) actual = LabText(table.GetCapacity());
                else {
                    T key = LabValue<T>(operation[@"key"]);
                    if ([op isEqualToString:@"add"]) {
                        T value = LabValue<T>(operation[@"value"]);
                        table.Add(key, value);
                        if (std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
                        actual = LabText(table.Get(key));
                    } else if ([op isEqualToString:@"update"]) {
                        table.Update(key, LabValue<T>(operation[@"value"])); actual = LabText(table.Get(key));
                    } else if ([op isEqualToString:@"get"]) actual = LabText(table.Get(key));
                    else if ([op isEqualToString:@"contains"]) actual = table.ContainsKey(key) ? @"true" : @"false";
                    else if ([op isEqualToString:@"remove"]) { table.Remove(key); actual = @"удалено"; }
                }
            } else if ([structure isEqualToString:@"queue"]) {
                if ([op isEqualToString:@"push"]) {
                    queue.Push(LabValue<T>(operation[@"value"]), LabValue<int>(operation[@"priority"])); actual = @"добавлено";
                } else if ([op isEqualToString:@"top"]) actual = LabText(queue.Top());
                else if ([op isEqualToString:@"pop"]) actual = LabText(queue.Pop());
                else if ([op isEqualToString:@"empty"]) actual = queue.IsEmpty() ? @"true" : @"false";
            } else {
                if ([op isEqualToString:@"push"]) { heap.Push(LabValue<double>(operation[@"value"])); actual = @"добавлено"; }
                else if ([op isEqualToString:@"top"]) actual = LabText(heap.Top());
                else if ([op isEqualToString:@"pop"]) actual = LabText(heap.Pop());
                else if ([op isEqualToString:@"empty"]) actual = heap.IsEmpty() ? @"true" : @"false";
            }
            if (!actual) throw std::invalid_argument("Эта операция не поддерживается выбранной структурой");
        } catch (const std::out_of_range& error) {
            errorType = @"std::out_of_range"; detail = LabText(std::string(error.what()));
        } catch (const std::invalid_argument& error) {
            errorType = @"std::invalid_argument"; detail = LabText(std::string(error.what()));
        } catch (const std::logic_error& error) {
            errorType = @"std::logic_error"; detail = LabText(std::string(error.what()));
        }
        if (errorType) actual = errorType;
        BOOL checked = operation[@"expect"] != nil;
        BOOL ok = checked && [actual isEqual:operation[@"expect"]]
            && ((errorType == nil && operation[@"throws"] == nil) || [errorType isEqual:operation[@"throws"]]);
        if (checked) { if (ok) ++passed; else ++failed; }
        [steps addObject:@{@"call": plan[i], @"actual": actual, @"detail": detail,
            @"expected": operation[@"expect"] ?: NSNull.null, @"passed": checked ? @(ok) : NSNull.null,
            @"error": @(errorType != nil)}];
    }
    NSMutableArray* values = [NSMutableArray array];
    NSMutableArray* entries = [NSMutableArray array];
    NSMutableArray* stats = [NSMutableArray array];
    NSString* title = @"Элементы";
    if ([structure isEqualToString:@"array"]) {
        for (int i = 0; i < array.GetLength(); ++i) [values addObject:LabText(array.Get(i))];
        [stats addObject:@{@"label": @"Размер", @"value": @(array.GetLength())}];
    } else if ([structure isEqualToString:@"tree"]) {
        title = @"Значения по возрастанию";
        [stats addObject:@{@"label": @"Высота AVL", @"value": @(tree.GetHeight())}];
        while (!tree.IsEmpty()) [values insertObject:LabText(tree.ExtractMax()) atIndex:0];
    } else if ([structure isEqualToString:@"hash"]) {
        title = @"Пары по корзинам";
        [stats addObjectsFromArray:@[@{@"label": @"Пар", @"value": @(table.GetCount())},
                                    @{@"label": @"Корзин", @"value": @(table.GetCapacity())}]];
        for (const T& key : keys) {
            if (table.ContainsKey(key)) [entries addObject:@{@"key": LabText(key), @"value": LabText(table.Get(key)),
                @"bucket": @(std::hash<T>{}(key) % table.GetCapacity())}];
        }
    } else if ([structure isEqualToString:@"queue"]) {
        title = @"Порядок извлечения · первый слева";
        while (!queue.IsEmpty()) [values addObject:LabText(queue.Pop())];
    } else {
        title = @"Порядок извлечения · первый слева";
        [stats addObject:@{@"label": @"Размер", @"value": @(heap.GetSize())}];
        while (!heap.IsEmpty()) [values addObject:LabText(heap.Pop())];
    }
    if (![structure isEqualToString:@"array"] && ![structure isEqualToString:@"hash"] && ![structure hasSuffix:@"heap"])
        [stats addObject:@{@"label": @"Элементов", @"value": @(values.count)}];
    return @{@"steps": steps, @"plan": plan, @"expectations": expectations,
        @"count": @(count), @"total": @(operations.count),
        @"passed": @(passed), @"failed": @(failed),
        @"snapshot": @{@"values": values, @"entries": entries, @"stats": stats, @"title": title}};
}

static NSDictionary* RunStructureLab(NSDictionary* request) {
    NSString* structure = request[@"structure"];
    NSString* type = request[@"valueType"];
    if (![@[@"array", @"tree", @"hash", @"queue", @"minheap", @"maxheap"] containsObject:structure])
        throw std::invalid_argument("Неизвестная структура данных");
    if ([structure hasSuffix:@"heap"] && ![type isEqualToString:@"double"])
        throw std::invalid_argument("BinaryHeap в этом проекте хранит только double");
    if ([type isEqualToString:@"int"]) return RunTypedLab<int>(request);
    if ([type isEqualToString:@"double"]) return RunTypedLab<double>(request);
    if ([type isEqualToString:@"string"]) return RunTypedLab<std::string>(request);
    throw std::invalid_argument("Неизвестный тип значений");
}
