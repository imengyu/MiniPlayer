#pragma once
#include <assert.h>
#include "CCSimpleStack.h"

template <typename T>
class CCSimpleQueueNode {
public:
	CCSimpleQueueNode* next = nullptr;
	CCSimpleQueueNode* prev = nullptr;
	T* value = nullptr;
};

template <typename T>
class CCSimpleQueue
{
public:
	CCSimpleQueue() {
	}
	CCSimpleQueue(int max_size) {
		alloc(max_size);
	}
	~CCSimpleQueue() {
		clear();
		for (int i = 0; i < m_max_size; i++)
			delete m_stack.pop();
		m_stack.clear();
		m_max_size = 0;
		m_size = 0;
	}

	bool alloc(int max_size) {
		if (m_max_size > 0)
			return false;
		m_max_size = max_size;
		m_stack.alloc(max_size);
		for (int i = 0; i < m_max_size; i++)
			m_stack.push(new CCSimpleQueueNode<T>());
		m_size = 0;
		return true;
	}

	//推入末尾
	bool push(T* n) {
		if (m_size < m_max_size) {
			auto node = m_stack.pop();
			assert(node);
			m_size++;
			if (!m_first)
				m_first = node;
			if (m_end)
				m_end->next = node;
			node->value = n;
			node->prev = m_end;
			node->next = nullptr;
			m_end = node;
			return true;
		}
		return false;
	}
	//推入首位
	bool push_front(T* n) {
		if (m_size < m_max_size) {
			auto node = m_stack.pop();
			assert(node);

			m_size++;
			node->next = m_first;
			node->prev = nullptr;
			node->value = n;
			if (m_first)
				m_first->prev = node;
			m_first = node;
			if (!m_end)
				m_end = node;
			return true;
		}
		return false;
	}
	//弹出末尾
	T* pop() {
		if (m_size > 0) {
			m_size--;
			auto node = m_end;
			m_end = node->prev;
			if (node == m_first)
				m_first = nullptr;
			if (m_end)
				m_end->next = nullptr;
			node->next = nullptr;
			node->prev = nullptr;
			m_stack.push(node);
			return node->value;
		}
		return nullptr;
	}
	//弹出首个
	T* pop_front() {
		if (m_size > 0) {
			m_size--;
			
			auto node = m_first;
			m_first = node->next;
			if (m_first)
				m_first->prev = nullptr;
			if (m_end == node)
				m_end = nullptr;
			node->next = nullptr;
			node->prev = nullptr;
			m_stack.push(node);
			return node->value;
		}
		return nullptr;
	}

	//首个
	CCSimpleQueueNode<T>* begin() {
		return m_first;
	}
	//末尾
	CCSimpleQueueNode<T>* end() {
		return m_end;
	}
	//移除节点
	CCSimpleQueueNode<T>* erase(CCSimpleQueueNode<T>* node) {
		if (node == m_first) {
			pop_front();
			return m_first;
		}
		if (node == m_end) {
			pop();
			return m_end;
		}

		assert(node->prev || node->next);

		auto next = node->next;

		if (node->prev)
			node->prev->next = next;
		if (next)
			next->prev = node->prev;

		node->next = nullptr;
		node->prev = nullptr;
		node->value = nullptr;

		m_size--;
		m_stack.push(node);

		return next;
	}


	void clear() {
		for (auto it = begin(); it; )
			it = erase(it);

		assert(m_size == 0);

		m_size = 0;
		m_first = nullptr;
		m_end = nullptr;
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
		
		m_stack.increase(size);
		for (int i = 0; i < size; i++)
			m_stack.push(new CCSimpleQueueNode<T>());
		m_max_size += size;
	}

private:
	int m_size = 0;
	int m_max_size = 0;
	CCSimpleStack<CCSimpleQueueNode<T>> m_stack;
	CCSimpleQueueNode<T>* m_first = nullptr;
	CCSimpleQueueNode<T>* m_end = nullptr;

};

