#include "DynamicArray.h"
#include "BinarySearchTree.h"
#include "BinaryHeap.h"
#include "PriorityQueue.h"
#include "HashTable.h"
#include <cassert>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

void TestDynamicArray() {
    DynamicArray<int> empty;
    assert(empty.GetLength() == 0);

    DynamicArray<int> array(3);
    assert(array.GetSize() == 3);
    assert(array[0] == 0);
    array.Set(0, 10);
    array[1] = 20;
    array[2] = 30;
    assert(array.Get(0) == 10);

    array.Resize(5);
    assert(array.GetLength() == 5);
    assert(array[0] == 10);
    assert(array[1] == 20);
    assert(array[2] == 30);
    assert(array[3] == 0);
    assert(array[4] == 0);
    array.Resize(2);
    assert(array.GetLength() == 2);
    assert(array[0] == 10);
    assert(array[1] == 20);

    DynamicArray<int> copy(array);
    copy[0] = 99;
    assert(array[0] == 10);
    assert(copy[0] == 99);
    DynamicArray<int> assigned(1);
    assigned = array;
    assigned[1] = 88;
    assert(array[1] == 20);
    assert(assigned[1] == 88);

    int values[] = {7, 8};
    DynamicArray<int> other(values, 2);
    array.Swap(other);
    assert(array[0] == 7);
    assert(other[0] == 10);
    other.Resize(0);
    assert(other.GetLength() == 0);

    bool caught = false;
    try {
        array.Get(-1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        array.Set(array.GetLength(), 1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        array.Resize(-1);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    assert(array.GetLength() == 2);
    assert(array[0] == 7);
    std::cout << "[OK] DynamicArray\n";
}

void TestBinarySearchTree() {
    BinarySearchTree<int> tree;
    assert(tree.IsEmpty());
    assert(tree.Insert(10));
    assert(tree.Insert(5));
    assert(tree.Insert(15));
    assert(tree.Insert(3));
    assert(tree.Insert(7));
    assert(tree.Insert(12));
    assert(tree.Insert(20));
    assert(!tree.Insert(10));
    assert(tree.Contains(7));
    assert(!tree.Contains(99));
    assert(tree.GetMax() == 20);
    assert(!tree.Remove(99));

    assert(tree.Remove(3));
    assert(!tree.Contains(3));
    assert(tree.Remove(5));
    assert(!tree.Contains(5));
    assert(tree.Contains(7));
    assert(tree.Remove(10));
    assert(!tree.Contains(10));
    assert(tree.Contains(7));
    assert(tree.Contains(12));
    assert(tree.Contains(15));
    assert(tree.Contains(20));

    assert(tree.ExtractMax() == 20);
    assert(tree.ExtractMax() == 15);
    assert(tree.ExtractMax() == 12);
    assert(tree.ExtractMax() == 7);
    assert(tree.IsEmpty());

    bool caught = false;
    try {
        tree.GetMax();
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        tree.ExtractMax();
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
    std::cout << "[OK] BinarySearchTree\n";
}

void TestTreeTraversal() {
    BinarySearchTree<int> tree;
    std::ostringstream output;
    std::streambuf* original = std::cout.rdbuf(output.rdbuf());

    tree.LKP();
    tree.BuildThreads();
    tree.LKPThreads();
    tree.PKLThreads();
    assert(output.str().empty());

    tree.Insert(4);
    tree.LKP();
    assert(output.str() == "4 ");
    output.str("");
    tree.LKPThreads();
    assert(output.str() == "4 ");
    output.str("");
    tree.PKLThreads();
    assert(output.str() == "4 ");
    output.str("");

    for (int value : {2, 6, 1, 3, 5, 7}) {
        tree.Insert(value);
    }
    tree.LKP();
    assert(output.str() == "1 2 3 4 5 6 7 ");
    output.str("");
    tree.LKPThreads();
    assert(output.str() == "1 2 3 4 5 6 7 ");
    output.str("");
    tree.PKLThreads();
    assert(output.str() == "7 6 5 4 3 2 1 ");
    output.str("");

    tree.BuildThreads();
    tree.BuildThreads();
    tree.LKPThreads();
    assert(output.str() == "1 2 3 4 5 6 7 ");
    output.str("");

    tree.Remove(1);
    tree.Remove(4);
    tree.Remove(6);
    assert(tree.ExtractMax() == 7);
    for (int value : {8, 9, 10}) {
        tree.Insert(value);
    }
    tree.LKP();
    assert(output.str() == "2 3 5 8 9 10 ");
    output.str("");
    tree.LKPThreads();
    assert(output.str() == "2 3 5 8 9 10 ");
    output.str("");
    tree.PKLThreads();
    assert(output.str() == "10 9 8 5 3 2 ");
    output.str("");

    while (!tree.IsEmpty()) {
        tree.ExtractMax();
    }
    tree.LKPThreads();
    tree.PKLThreads();
    assert(output.str().empty());
    tree.Insert(42);
    tree.LKPThreads();
    assert(output.str() == "42 ");

    std::cout.rdbuf(original);
    std::cout << "[OK] Tree traversal and threads\n";
}

void TestBinaryHeap() {
    for (bool minHeap : {true, false}) {
        BinaryHeap heap(minHeap);
        assert(heap.IsEmpty());
        assert(heap.GetSize() == 0);

        bool caught = false;
        try {
            heap.Top();
        } catch (const std::logic_error&) {
            caught = true;
        }
        assert(caught);
        caught = false;
        try {
            heap.Pop();
        } catch (const std::logic_error&) {
            caught = true;
        }
        assert(caught);
        assert(heap.GetSize() == 0);

        for (double value : {3.5, -2.0, 7.0, 7.0, 0.0}) {
            heap.Push(value);
        }
        assert(!heap.IsEmpty());
        assert(heap.GetSize() == 5);
        double expected[] = {-2, 0, 3.5, 7, 7};
        for (int i = 0; i < 5; ++i) {
            double value = minHeap ? expected[i] : expected[4 - i];
            assert(heap.Top() == value);
            assert(heap.Top() == value);
            assert(heap.GetSize() == 5 - i);
            assert(heap.Pop() == value);
            assert(heap.GetSize() == 4 - i);
        }
        assert(heap.IsEmpty());

        heap.Push(42);
        assert(heap.Top() == 42);
        assert(heap.Pop() == 42);
        assert(heap.IsEmpty());

        for (int i = 31; i >= 0; --i) {
            heap.Push(i);
        }
        assert(heap.GetSize() == 32);
        for (int i = 0; i < 32; ++i) {
            double value = minHeap ? i : 31 - i;
            assert(heap.Pop() == value);
        }
        assert(heap.IsEmpty());
        assert(heap.GetSize() == 0);
    }
    std::cout << "[OK] BinaryHeap\n";
}

void TestPriorityQueue() {
    PriorityQueue<int> queue;
    assert(queue.IsEmpty());
    queue.Push(100, 1);
    queue.Push(200, 3);
    queue.Push(300, 2);
    assert(!queue.IsEmpty());
    assert(queue.Top() == 200);
    assert(queue.Top() == 200);
    assert(queue.Pop() == 200);
    assert(queue.Pop() == 300);
    assert(queue.Pop() == 100);
    assert(queue.IsEmpty());

    queue.Push(1, 5);
    queue.Push(2, 5);
    queue.Push(3, 5);
    int first = queue.Pop();
    int second = queue.Pop();
    int third = queue.Pop();
    assert(first >= 1 && first <= 3);
    assert(second >= 1 && second <= 3);
    assert(third >= 1 && third <= 3);
    assert(first != second);
    assert(first != third);
    assert(second != third);
    assert(queue.IsEmpty());

    queue.Push(40, -2);
    queue.Push(50, -1);
    assert(queue.Pop() == 50);
    assert(queue.Pop() == 40);

    bool caught = false;
    try {
        queue.Pop();
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        queue.Top();
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
    std::cout << "[OK] PriorityQueue\n";
}

void TestAVLBalance() {
    int orders[][3] = {
        {30, 20, 10},
        {10, 20, 30},
        {30, 10, 20},
        {10, 30, 20}
    };
    for (const auto& order : orders) {
        BinarySearchTree<int> tree;
        for (int value : order) {
            assert(tree.Insert(value));
        }
        assert(tree.GetHeight() == 2);
        assert(tree.Contains(10));
        assert(tree.Contains(20));
        assert(tree.Contains(30));
        assert(!tree.Insert(20));
        assert(tree.GetHeight() == 2);
        assert(tree.Remove(20));
        assert(tree.ExtractMax() == 30);
        assert(tree.ExtractMax() == 10);
        assert(tree.GetHeight() == 0);
    }

    for (int direction : {1, -1}) {
        BinarySearchTree<int> tree;
        for (int i = 0; i < 1000; ++i) {
            int value = direction == 1 ? i : 999 - i;
            assert(tree.Insert(value));
            assert(tree.GetHeight() <= 15);
        }
        for (int i = 0; i < 1000; i += 2) {
            assert(tree.Remove(i));
            assert(!tree.Contains(i));
            assert(tree.GetHeight() <= 15);
        }
        for (int i = 999; i >= 1; i -= 2) {
            assert(tree.ExtractMax() == i);
            assert(tree.GetHeight() <= 15);
        }
        assert(tree.IsEmpty());
        assert(tree.GetHeight() == 0);
    }

    BinarySearchTree<int> tree;
    std::set<int> expected;
    std::mt19937 random(42);
    for (int i = 0; i < 5000; ++i) {
        int value = static_cast<int>(random() % 1000);
        int operation = static_cast<int>(random() % 4);
        if (operation == 0) {
            bool inserted = expected.insert(value).second;
            assert(tree.Insert(value) == inserted);
        } else if (operation == 1) {
            bool removed = expected.erase(value) != 0;
            assert(tree.Remove(value) == removed);
        } else if (operation == 2 && !expected.empty()) {
            int maximum = *expected.rbegin();
            assert(tree.ExtractMax() == maximum);
            expected.erase(maximum);
        } else {
            assert(tree.Contains(value) == expected.contains(value));
        }
        assert(tree.IsEmpty() == expected.empty());
        assert(tree.GetHeight() <= 15);
        if (!expected.empty()) {
            assert(tree.GetMax() == *expected.rbegin());
        }
    }
    while (!expected.empty()) {
        int maximum = *expected.rbegin();
        assert(tree.ExtractMax() == maximum);
        expected.erase(maximum);
    }
    assert(tree.IsEmpty());

    PriorityQueue<int> queue;
    for (int i = 0; i < 1000; ++i) {
        queue.Push(i, i);
    }
    for (int i = 999; i >= 0; --i) {
        assert(queue.Pop() == i);
    }
    assert(queue.IsEmpty());
    std::cout << "[OK] AVL balance\n";
}

void TestHashTable() {
    HashTable<int, int> table;
    assert(table.GetCount() == 0);
    table.Add(1, 10);
    table.Add(9, 90);
    table.Add(17, 170);
    assert(table.GetCount() == 3);
    assert(table.ContainsKey(9));
    assert(!table.ContainsKey(99));
    assert(table.Get(9) == 90);
    table.Update(9, 95);
    assert(table.Get(9) == 95);
    assert(table.GetCount() == 3);
    table.Remove(9);
    assert(!table.ContainsKey(9));
    assert(table.GetCount() == 2);
    assert(table.Get(1) == 10);
    assert(table.Get(17) == 170);

    bool caught = false;
    try {
        table.Add(1, 999);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
    assert(table.Get(1) == 10);
    assert(table.GetCount() == 2);

    caught = false;
    try {
        table.Get(99);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        table.Update(99, 1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        table.Remove(99);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
    assert(table.GetCount() == 2);

    HashTable<int, int> growing;
    std::size_t initialCapacity = growing.GetCapacity();
    for (int i = 0; i < 32; ++i) {
        growing.Add(i, i * 10);
    }
    assert(growing.GetCount() == 32);
    assert(growing.GetCapacity() > initialCapacity);
    for (int i = 0; i < 32; ++i) {
        assert(growing.Get(i) == i * 10);
    }
    std::size_t largeCapacity = growing.GetCapacity();
    for (int i = 0; i < 28; ++i) {
        growing.Remove(i);
    }
    assert(growing.GetCount() == 4);
    assert(growing.GetCapacity() < largeCapacity);
    for (int i = 28; i < 32; ++i) {
        assert(growing.Get(i) == i * 10);
        growing.Remove(i);
    }
    assert(growing.GetCount() == 0);
    growing.Add(100, 1000);
    assert(growing.Get(100) == 1000);
    std::cout << "[OK] HashTable\n";
}

int main() {
    TestDynamicArray();
    TestBinarySearchTree();
    TestAVLBalance();
    TestTreeTraversal();
    TestBinaryHeap();
    TestPriorityQueue();
    TestHashTable();
    std::cout << "Все тесты структур данных пройдены.\n";
}
