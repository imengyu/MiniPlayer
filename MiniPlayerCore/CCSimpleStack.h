#pragma once
#include "DbgHelper.h"

template <typename T>
class CCSimpleStack
{
public:
	CCSimpleStack() {
	}
	CCSimpleStack(size_t max_size) {
		alloc(max_size);
	}
	~CCSimpleStack() {
		release();
	}

	bool alloc(size_t max_size) {
		if (m_stack)
			return false;
		if (max_size > 1048576 * 64)
			return false;
		m_max_size = max_size;
		m_stack = (T**)malloc(sizeof(T*) * max_size);
		if (!m_stack)
			return false;
		m_size = 0;
		memset(m_stack, 0, sizeof(T*) * max_size);
		return true;
	}
	bool release() {
		m_max_size = 0;
		m_size = 0;
		if (m_stack) {
			free(m_stack);
			m_stack = nullptr;
			return true;
		}
		return false;
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
	size_t size() {
		return m_size; 
	}
	size_t capacity() {
		return m_max_size;
	}
	bool empty() {
		return m_size == 0;
	}
	void increase(size_t size) {
		Assert(size > 0);

		auto old_max_size = m_max_size;
		m_stack = (T**)realloc(m_stack, (old_max_size + size) * sizeof(T*));

		Assert(m_stack);

		m_max_size = old_max_size + size;
		memset(m_stack + old_max_size, 0, size * sizeof(T*));
	}

private:
	size_t m_size = 0;
	size_t m_max_size = 0;
	T** m_stack = nullptr;
};
