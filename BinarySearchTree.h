#ifndef LAB2_BINARYSEARCHTREE_H
#define LAB2_BINARYSEARCHTREE_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

template <typename T>
class BinarySearchTree {
private:
    struct Node {
        T value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        int height = 1;

        Node* previous = nullptr;
        Node* next = nullptr;

        explicit Node(const T& value) : value(value) {}
    };

    std::unique_ptr<Node> root;

    int GetHeight(const std::unique_ptr<Node>& node) const {
        return node == nullptr ? 0 : node->height;
    }

    void UpdateHeight(std::unique_ptr<Node>& node) {
        node->height = 1 + std::max(GetHeight(node->left), GetHeight(node->right));
    }

    int GetBalance(const std::unique_ptr<Node>& node) const {
        return (GetHeight(node->left) - GetHeight(node->right));
    }

    void RotateRight(std::unique_ptr<Node>& node) {
        std::unique_ptr<Node> newRoot = std::move(node->left);
        node->left = std::move(newRoot->right);
        UpdateHeight(node);
        newRoot->right = std::move(node);
        UpdateHeight(newRoot);
        node = std::move(newRoot);
    }

    void RotateLeft(std::unique_ptr<Node>& node) {
        std::unique_ptr<Node> newRoot = std::move(node->right);
        node->right = std::move(newRoot->left);
        UpdateHeight(node);
        newRoot->left = std::move(node);
        UpdateHeight(newRoot);
        node = std::move(newRoot);
    }

    void Balance(std::unique_ptr<Node>& node) {
        if (node == nullptr) {
            return;
        }
        UpdateHeight(node);
        int balance = GetBalance(node);
        if (balance > 1) {
            if (GetBalance(node->left) < 0) {
                RotateLeft(node->left);
            }
            RotateRight(node);
        } else if (balance < -1) {
            if (GetBalance(node->right) > 0) {
                RotateRight(node->right);
            }
            RotateLeft(node);
        }
    }

    bool InsertNode(std::unique_ptr<Node>& node, const T& value){
        if (node == nullptr) {
            node = std::make_unique<Node>(value);
            return true;
        }
        bool inserted;
        if (value < node->value){
            inserted = InsertNode(node->left, value);
        } else if (value > node->value){
            inserted = InsertNode(node->right, value);
        } else {
            return false;
        }
        Balance(node);
        return inserted;
    }

    bool ContainsNode(const std::unique_ptr<Node>& node, const T& value) const {
        if (node == nullptr) {
            return false;
        } else if (node->value < value) {
            return ContainsNode(node->right, value);
        } else if (value < node->value) {
            return ContainsNode(node->left, value);
        } else {
            return true;
        }
    }

    T ExtractMaxNode(std::unique_ptr<Node>& node){
        if (node == nullptr) {
            throw std::out_of_range("null");
        } else if (node->right) {
            T value = ExtractMaxNode(node->right);
            Balance(node);
            return value;
        } else {
            std::unique_ptr<Node> temp_node = std::move(node);
            node = std::move(temp_node->left);
            return temp_node->value;
        }
    }

    bool RemoveNode(std::unique_ptr<Node>& node, const T& value) {
        if (node == nullptr) {
            return false;
        }
        bool removed;
        if (node->value < value) {
            removed = RemoveNode(node->right, value);
        } else if (value < node->value) {
            removed = RemoveNode(node->left, value);
        } else {
            if (!(node->right) && !(node->left)){
                node.reset();
            } else if (node->left && node->right) {
                node->value = ExtractMaxNode(node->left);
            } else {
                if (node->right) {
                    std::unique_ptr<Node> temp_node = std::move(node);
                    node = std::move(temp_node->right);
                } else {
                    std::unique_ptr<Node> temp_node = std::move(node);
                    node = std::move(temp_node->left);
                }
            }
            removed = true;
        }
        Balance(node);
        return removed;
    }

    void PrintInOrderNode(const Node* node) const {
        if (node == nullptr) {
            return;
        }

        PrintInOrderNode(node->left.get());
        std::cout << node->value << ' ';
        PrintInOrderNode(node->right.get());
    }

    Node* prev = nullptr;

    void Threads(Node* node) {
        if (node == nullptr) {
            return;
        }

        Threads(node->left.get());

        node->previous = nullptr;
        node->next = nullptr;

        if (node->left == nullptr) {
            node->previous = prev;
        }

        if (prev != nullptr && prev->right == nullptr) {
            prev->next = node;
        }

        prev = node;

        Threads(node->right.get());
    }

public:
    int GetHeight() const {
        return GetHeight(root);
    }

    bool Insert(const T& value) {
        return InsertNode(root, value);
    }

    bool Contains(const T& value) {
        return ContainsNode(root, value);
    }

    const T& GetMax() const {
        const Node* maxi = root.get();
        if (root == nullptr){
            throw std::out_of_range("tree is empty");
        }
        while (maxi->right){
            maxi = maxi->right.get();
        }
        return maxi->value;
    }

    T ExtractMax() {
        return ExtractMaxNode(root);
    }

    bool Remove(const T& value) {
        return RemoveNode(root, value);
    }

    bool IsEmpty() const {
        if (root == nullptr){
            return true;
        } else {
            return false;
        }
    }

    void LKP() const {
        PrintInOrderNode(root.get());
    }

    void BuildThreads() {
        prev = nullptr;
        Threads(root.get());
    }

    void LKPThreads() {
        BuildThreads();
        const Node* node = root.get();
        while (node != nullptr && node->left != nullptr) {
            node = node->left.get();
        }
        while (node != nullptr) {
            std::cout << node->value << ' ';
            if (node->right == nullptr) {
                node = node->next;
            } else {
                node = node->right.get();
                while (node->left != nullptr) {
                    node = node->left.get();
                }
            }
        }
    }

    void PKLThreads() {
        BuildThreads();
        const Node* node = root.get();
        while (node != nullptr && node->right != nullptr) {
            node = node->right.get();
        }
        while (node != nullptr) {
            std::cout << node->value << ' ';
            if (node->left == nullptr) {
                node = node->previous;
            } else {
                node = node->left.get();
                while (node->right != nullptr) {
                    node = node->right.get();
                }
            }
        }
    }

};

#endif
