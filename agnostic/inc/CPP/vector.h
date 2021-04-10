#ifndef AGNOSTIC_VECTOR_H
#define AGNOSTIC_VECTOR_H

#include <stddef.h>

template <class T>
class Vector {
public:
    T at(size_t a) {
        // Check if at is in bounds
        if(a >= _size) { return (T)NULL; }
        return array[a];
    }

    // Add to end.
    void push_back(T t) {
        // Check if there is space left
        if(_size == _capacity) { reserve(_size + 5); } // Reserve incase of the array being too small
        // Add to back
        array[_size++] = t;
    }
    
    // Remove a element.
    void remove(size_t at) {
        // Check if at is in bounds
        if(at >= _size) { return; }

        // Move all entries past at left
        for(int i = at; i < (_size - 1); i++) {
            array[i] = array[i + 1];
        }
        _size--;
    }

    // Resize the vector. Does not change size
    void reserve(size_t n) {
        if(_capacity > n) { return; }
        if(_capacity == 0) {
            // Allocate and return
            array = (T*)malloc(sizeof(T) * n);
            _capacity = n;
            return;
        }

        T* new_array = (T*)malloc(sizeof(T) * n);
        for(size_t i = 0; i < _size; i++) {
            new_array[i] = array[i];
        }
        free(array);
        // Set new capacity
        _capacity = n;
        array = new_array;
    }

    // Insert a element into the vector at a specific position.
    void insert(size_t at, T t) {
        if(at > _size) { return; }
        // Check capacity
        if(_size == _capacity) { reserve(_size + 5); }
        // Copy everything from at to at + 1
        for(size_t i = at; i < _size; i++) {
            array[i + 1] = array[i];
        }
        array[at] = t;
        _size++;
    }

    // Get the size.
    size_t size() { return _size; }

    Vector() {
        _size = 0;
        _capacity = 0;
        array = NULL;
    }
    ~Vector() {
        // Delete all entries
        free(array);
    }
private:
    T* array;
    size_t _size;
    size_t _capacity;
};

#endif