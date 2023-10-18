#pragma once
#include <assert.h>

template <typename T>
class CCSimpleStack
{
public:
	CCSimpleStack() {
	}
	CCSimpleStack(int max_size) {
		alloc(max_size);
	}
	~CCSimpleStack() {
		free(m_stack);
		m_max_size = 0;
		m_size = 0;
	}

	bool alloc(int max_size) {
		if (m_stack)
			return false;
		m_max_size = max_size;
		m_stack = (T**)malloc(sizeof(T*) * max_size);
		if (!m_stack)
			return false;
		m_size = 0;
		memset(m_stack, 0, sizeof(T*) * max_size);
		return true;
	}

	bool push(T* n) {
		if (m_size < m_max_size) {
			m_stack[m_size] = n;
			m_size++;
			return true;
		}
		return false;
	}
	T* pop() {
		if (m_size > 0) {
			m_size--;
			auto v = m_stack[m_size];
			m_stack[m_size] = nullptr;
			return v;
		}
		return nullptr;
	}
	void clear() {
		memset(m_stack, 0, sizeof(T*) * m_max_size);
		m_size = 0;
	}
	int size() { 
		return m_size; 
	}
	int capacity() {
		return m_max_size;
	}
	bool empty() {
		return m_size == 0;
	}
	void increase(int size) {
		assert(size > 0);

		auto old_max_size = m_max_size;
		m_stack = (T**)realloc(m_stack, (old_max_size + size) * sizeof(T*));

		assert(m_stack);

		m_max_size = old_max_size + size;
		memset(m_stack + old_max_size, 0, size * sizeof(T*));
	}

private:
	int m_size;
	int m_max_size;
	T** m_stack;
};
