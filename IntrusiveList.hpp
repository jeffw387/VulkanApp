#pragma once

struct TestNode
{
    TestNode* next = nullptr;
    TestNode* previous = nullptr;
    int x;

    TestNode() noexcept = default;
    TestNode(int x) : x(x) {}
};

template <typename T>
class IntrusiveForwardList
{
public:
    void push_front(T* element)
    {
        element->next = root;
        root->previous = element;
        root = element;
        ++count;
    }

    void pop_front()
    {
        root = root->next;
        if (count > 0)
            --count;
    }

    T* front()
    {
        return root;
    }

    void erase(T* element)
    {
        if (element->previous != nullptr)
            element->previous->next = element->next;
        if (element->next != nullptr)
            element->next->previous = element->previous;
        if (count > 0)
            --count;
    }

    const size_t& size() { return count; }
private:
    T* root = nullptr;
    size_t count;
};