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
	CCSimpleQueue(size_t max_size) {
		alloc(max_size);
	}
	~CCSimpleQueue() {
		release();
	}

	bool alloc(size_t max_size) {
		if (m_max_size > 0)
			return false;
		m_max_size = max_size;
		m_stack.alloc(max_size);
		for (size_t i = 0; i < m_max_size; i++)
			m_stack.push(new CCSimpleQueueNode<T>());
		return true;
	}
	bool release() {
		clear();
		for (void* it = m_stack.pop(); it; it = m_stack.pop())
			delete it;
		m_max_size = 0;
		m_size = 0;
		return m_stack.release();
	}

	//����ĩβ
	bool push(T* n) {
		if (m_size < m_max_size) {
			auto node = m_stack.pop();
			Assert(node);
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
	//������λ
	bool push_front(T* n) {
		if (m_size < m_max_size) {
			auto node = m_stack.pop();
			Assert(node);

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
	//����ĩβ
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
	//�����׸�
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

	//�׸�
	CCSimpleQueueNode<T>* begin() {
		return m_first;
	}
	//ĩβ
	CCSimpleQueueNode<T>* end() {
		return m_end;
	}
	//�Ƴ��ڵ�
	CCSimpleQueueNode<T>* erase(CCSimpleQueueNode<T>* node) {
		if (node == m_first) {
			pop_front();
			return m_first;
		}
		if (node == m_end) {
			pop();
			return m_end;
		}

		Assert(node->prev || node->next);

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

		Assert(m_size == 0);

		m_size = 0;
		m_first = nullptr;
		m_end = nullptr;
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
		
		m_stack.increase(size);
		for (size_t i = 0; i < size; i++)
			m_stack.push(new CCSimpleQueueNode<T>());
		m_max_size += size;
	}

private:
	size_t m_size = 0;
	size_t m_max_size = 0;
	CCSimpleStack<CCSimpleQueueNode<T>> m_stack;
	CCSimpleQueueNode<T>* m_first = nullptr;
	CCSimpleQueueNode<T>* m_end = nullptr;

};

