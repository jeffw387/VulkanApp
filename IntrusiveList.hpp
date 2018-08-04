#pragma once

template <typename T>
class IntrusiveList {
 public:
  void push_front(T* element) {
    if (root != nullptr) {
      auto prevRoot = root;
      prevRoot->previous = element;
      element->next = prevRoot;
    }
    root = element;
    ++count;
  }

  void pop_front() {
    root = root->next;
    if (count > 0) --count;
  }

  T* front() { return root; }

  void erase(T* element) {
    if (element->previous != nullptr) element->previous->next = element->next;
    if (element->next != nullptr) element->next->previous = element->previous;
    if (root == element) root = nullptr;
    if (count > 0) --count;
  }

  const size_t& size() { return count; }

 private:
  T* root = nullptr;
  size_t count = 0U;
};