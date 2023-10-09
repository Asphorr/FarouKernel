#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>
#include <memory>

using namespace std;

template <typename T>
struct Node {
    T data;
    unique_ptr<Node<T>> left;
    unique_ptr<Node<T>> right;
};

template <typename T>
void printTree(unique_ptr<Node<T>> root) {
    queue<unique_ptr<Node<T>>> q;
    q.emplace(root);
    while (!q.empty()) {
        auto node = q.front();
        cout << node->data << " ";
        if (node->left != nullptr) {
            q.emplace(node->left);
        }
        if (node->right != nullptr) {
            q.emplace(node->right);
        }
        q.pop();
    }
    cout << endl;
}

template <typename T>
bool containsDuplicates(unique_ptr<Node<T>> root) {
    stack<unique_ptr<Node<T>>> s;
    s.emplace(root);
    unordered_map<T, bool> seen;
    while (!s.empty()) {
        auto node = s.top();
        if (seen.find(node->data) == seen.end()) {
            seen[node->data] = true;
            if (node->left != nullptr && !containsDuplicates(node->left)) {
                return false;
            }
            if (node->right != nullptr && !containsDuplicates(node->right)) {
                return false;
            }
        } else {
            return true;
        }
        s.pop();
    }
    return false;
}

template <typename T>
void addToQueue(unique_ptr<Node<T>> root, queue<unique_ptr<Node<T>>>& q) {
    if (root != nullptr) {
        q.emplace(root);
        addToQueue(root->left, q);
        addToQueue(root->right, q);
    }
}

template <typename T>
void levelOrderTraversal(unique_ptr<Node<T>> root) {
    queue<unique_ptr<Node<T>>> q;
    addToQueue(root, q);
    while (!q.empty()) {
        auto node = q.front();
        cout << node->data << " ";
        q.pop();
    }
    cout << endl;
}

int main() {
    unique_ptr<Node<int>> root = make_unique<Node<int>>(1);
    root->left = make_unique<Node<int>>(2);
    root->right = make_unique<Node<int>>(3);
    root->left->left = make_unique<Node<int>>(4);
    root->left->right = make_unique<Node<int>>(5);
    root->right->left = make_unique<Node<int>>(6);
    root->right->right = make_unique<Node<int>>(7);

    printTree(root);
    cout << "Contains duplicates? " << containsDuplicates(root) << endl;
    levelOrderTraversal(root);

    return 0;
}
