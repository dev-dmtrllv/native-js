#pragma once

#include "framework.hpp"

namespace NativeJS
{
	namespace LockFree
	{
		template <typename T>
		class Stack
		{
		public:
			explicit Stack(size_t capacity)
				: _capacity(capacity)
			{
				_indexMask = capacity;

				for (size_t i = 1; i <= sizeof(void*) * 4; i <<= 1)
					_indexMask |= _indexMask >> i;
				
				_abaOffset = _indexMask + 1;

				_queue = new Node[capacity + 1];
				
				for (size_t i = 1; i < capacity;)
				{
					Node& node = _queue[i];
					node.abaNextFree = ++i;
				}
				
				_queue[capacity].abaNextFree = 0;

				_abaFree = 1;
				_abaPushed = 0;
			}

			~Stack()
			{
				for (size_t abaPushed = _abaPushed;;)
				{
					size_t nodeIndex = abaPushed & _indexMask;
					if (!nodeIndex)
						break;
					Node& node = _queue[nodeIndex];
					abaPushed = node.abaNextPushed;
					(&node.data)->~T();
				}

				delete _queue;
			}

			size_t capacity() const { return _capacity; }

			size_t size() const { return 0; }

			bool push(const T& data)
			{
				Node* node;
				size_t abaFree;
				for (;;)
				{
					abaFree = _abaFree;
					size_t nodeIndex = abaFree & _indexMask;
					if (!nodeIndex)
						return false;
					node = &_queue[nodeIndex];
					if (_abaFree.compare_exchange_weak(abaFree, node->abaNextFree + _abaOffset, std::memory_order::acq_rel))
						break;
				}

				new (&node->data)T(data);
				for (;;)
				{
					size_t abaPushed = _abaPushed;
					node->abaNextPushed = abaPushed;
					if (_abaPushed.compare_exchange_weak(abaPushed, abaFree, std::memory_order::acq_rel))
						return true;
				}
			}

			bool pop(T& result)
			{
				Node* node;
				size_t abaPushed;
				for (;;)
				{
					abaPushed = _abaPushed;
					size_t nodeIndex = abaPushed & _indexMask;
					if (!nodeIndex)
						return false;
					node = &_queue[nodeIndex];
					if (_abaPushed.compare_exchange_weak(abaPushed, node->abaNextPushed + _abaOffset, std::memory_order::acq_rel))
						break;
				}

				result = node->data;
				(&node->data)->~T();
				abaPushed += _abaOffset;
				for (;;)
				{
					size_t abaFree = _abaFree;
					node->abaNextFree = abaFree;
					if (_abaFree.compare_exchange_weak(abaFree, abaPushed, std::memory_order::acq_rel))
						return true;
				}
			}

		private:
			struct Node
			{
				T data;
				std::atomic<size_t> abaNextFree;
				std::atomic<size_t> abaNextPushed;
			};

		private:
			size_t _indexMask;
			Node* _queue;
			size_t _abaOffset;
			size_t _capacity;
			char cacheLinePad1[64];
			std::atomic<size_t> _abaFree;
			char cacheLinePad2[64];
			std::atomic<size_t> _abaPushed;
			char cacheLinePad3[64];
		};
	}
}