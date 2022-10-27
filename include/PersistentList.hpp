#pragma once

#include "framework.hpp"

namespace NativeJS
{
	template<typename T>
	class PersistentList
	{
		struct Node
		{
			template<class... Args>
				requires std::constructible_from<T, Args...>
			Node(const size_t index, Args&&... args) :
				index(index),
				data(new T(std::forward<Args>(args)...)),
				isFree(false)
			{ }

			size_t index;
			T* data;
			bool isFree;
		};
	public:
		PersistentList() :
			size_(0),
			free_(),
			list_()
		{}

		template<class... Args>
			requires std::constructible_from<T, Args...>
		size_t alloc(Args&&... args)
		{
			Node* node;
			if (free_.size() > 0)
			{
				int i = free_.size() - 1;
				node = free_[i];
				free_.erase(free_.begin() + i);
				node->isFree = false;
				std::construct_at<T>(node->data, std::forward<Args>(args)...);
			}
			else
			{
				const size_t l = list_.size();
				node = new Node(l, std::forward<Args>(args)...);
				list_.emplace_back(node);
			}

			size_++;

			return node->index;
		}

		bool free(const size_t index)
		{
			if(index >= list_.size())
				return false;

			Node* node = list_.at(index);
			node->isFree = true;
			node->data->~T();
			free_.emplace_back(node);
			size_--;

			return true;
		}

		bool free(const T* data)
		{
			for (Node* node : list_)
				if (!node->isFree && data == node->data)
				{
					node->isFree = true;
					node->data->~T();
					free_.emplace_back(node);
					size_--;
					return true;
				}

			return false;
		}

		template<typename Callback>
		void forEach(Callback callback) const
		{
			for (Node* node : list_)
				if (!node->isFree)
					callback(node->data);
		}

		void clear()
		{
			for (Node* node : list_)
			{
				if (!node->isFree)
					node->data->~T();
				delete node;
			}
			free_.clear();
			list_.clear();
			size_ = 0;
		}

		size_t size() const { return size_; }

		T* at(const size_t i) const { return list_.at(i)->data; }

	private:
		std::vector<Node*> list_;
		std::vector<Node*> free_;
		size_t size_ = 0;
	};
}