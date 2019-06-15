#ifndef QUEUE_H
#define QUEUE_H


template <typename TYPE> class Queue
{
protected:
    int count = 0;
    int capacity = 0;
    TYPE *array = 0x00;

    int startPoint = 0;

    void DeleteQueue() {
        if (array != 0x00) {
            delete[] array;
            array = 0x00;
        }
    }

public:
    int Capacity() { return capacity; }
    int Count() { return this->count; }

    bool IsEmpty() { return count <= 0; }
    bool IsFull() { return count >= capacity; }

    Queue(int capacity) {
        this->capacity = capacity;
        array = new TYPE[capacity];
    }

    ~Queue() {
        DeleteQueue();
    }

    bool Pop() {

        if (count <= 0)
        {
            return false;
        }

        count--;
        startPoint = (startPoint + 1) % capacity;
        return true;
    }

    bool front(TYPE *item) {
        if(count <= 0)
        {
            return false;
        }

        *item = array[startPoint];
        return true;
    }

    bool Push(const TYPE &item) {

        if (count >= capacity)
        {
            return false;
        }

        array[(startPoint + count) % capacity] = item;

        count++;

        return true;
    }

    void Clear() {
        startPoint = 0;
        count = 0;
    }
};

#endif // !_INCLUDE_QUEUE_H_
