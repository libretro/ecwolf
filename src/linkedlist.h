/*
** linkedlist.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

/**
 * Provides an interface for having a linked list embedded into a class.
 *
 * The class should inherit from EmbeddedList<T>::Node. T can be any type
 * although typically it will be the class. Then, a EmbeddedList<T>::List must
 * exist in order to define the starting point of the chain. Only the head of
 * the list is stored, so new items are pushed to the front of the list.
 *
 * Being that the nodes in the list is embedded into the class, each object can
 * only exist in one list, although there may be many lists the object can be
 * apart of.
 */
template<class T> class EmbeddedList
{
public:
	class Node
	{
	public:
		Node() : elNext(NULL), elPrev(NULL) {}

	protected:
		friend class EmbeddedList<T>::Iterator;
		friend class EmbeddedList<T>::ConstIterator;
		friend class EmbeddedList<T>::List;

		Node *elNext, *elPrev;
	};

	class Iterator
	{
	public:
		Iterator() : node(NULL) {}
		Iterator(typename EmbeddedList<T>::Node *node) : node(node)
		{
		}

		bool HasNext() { return node->elNext; }
		bool HasPrev() { return node->elPrev; }

		Iterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		Iterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		Iterator &operator++() { return Next(); }
		Iterator operator++(int) { Iterator copy(*this); Next(); return copy; }
		Iterator &operator--() { return Prev(); }
		Iterator operator--(int) { Iterator copy(*this); Prev(); return copy; }

		T &operator*() const
		{
			return *static_cast<T*>(node);
		}
		T *operator->() const
		{
			return static_cast<T*>(node);
		}

		operator T*() const { return static_cast<T*>(node); }
		operator bool() const { return node != NULL; }
	private:
		typename EmbeddedList<T>::Node *node;
	};

	class ConstIterator
	{
	public:
		ConstIterator() : node(NULL) {}
		ConstIterator(const typename EmbeddedList<T>::Node *node) : node(node)
		{
		}

		bool HasNext() { return node->elNext; }
		bool HasPrev() { return node->elPrev; }

		ConstIterator &Next()
		{
			node = node->elNext;
			return *this;
		}
		ConstIterator &Prev()
		{
			node = node->elPrev;
			return *this;
		}
		ConstIterator &operator++() { return Next(); }
		ConstIterator operator++(int) { ConstIterator copy(*this); Next(); return copy; }
		ConstIterator &operator--() { return Prev(); }
		ConstIterator operator--(int) { ConstIterator copy(*this); Prev(); return copy; }

		const T &operator*() const
		{
			return *static_cast<const T*>(node);
		}
		const T *operator->() const
		{
			return static_cast<const T*>(node);
		}

		operator const T*() const { return static_cast<const T*>(node); }
		operator bool() const { return node != NULL; }
	private:
		const typename EmbeddedList<T>::Node *node;
	};

	class List
	{
	public:
		typedef typename EmbeddedList<T>::Node N;

		List() : head(NULL), size(0)
		{
		}

		// Creates an iterator for the list. Note that the Iterator will point
		// to just before the list so it must be incremented before use.
		typename EmbeddedList<T>::Iterator Iterator()
		{
			assert(reinterpret_cast<N*>(this)->elNext == head);
			return EmbeddedList<T>::Iterator(reinterpret_cast<N*>(this));
		}
		typename EmbeddedList<T>::ConstIterator Iterator() const
		{
			return EmbeddedList<T>::ConstIterator(reinterpret_cast<N*>(this));
		}

		// Head() is the same as Iterator() only the iterator is initialized
		// to the head instead of one space before the head.
		typename EmbeddedList<T>::Iterator Head()
		{
			return EmbeddedList<T>::Iterator(head);
		}
		typename EmbeddedList<T>::ConstIterator Head() const
		{
			return EmbeddedList<T>::ConstIterator(head);
		}

		void Push(N *node)
		{
			++size;

			node->elNext = head;
			if(head)
				head->elPrev = node;
			head = node;
		}
		void Remove(N *node)
		{
			if(node->elNext)
				node->elNext->elPrev = node->elPrev;
			else if(!node->elPrev && head != node)
				return; // Does not appear to be on the list, so we're done

			if(node->elPrev)
				node->elPrev->elNext = node->elNext;
			else
				head = node->elNext;

			node->elNext = node->elPrev = NULL;
			--size;
		}

		unsigned int Size() const
		{
			return size;
		}
	private:
		List(const List &other) {}

		// This should be the first member so we can type pun as Node::elNext.
		typename EmbeddedList<T>::Node *head;
		unsigned int size;
	};
};

template<class T> class LinkedList
{
	public:
		class Node : public EmbeddedList<Node>::Node
		{
			public:
				Node(const T &item, typename EmbeddedList<Node>::List &head) : item(item)
				{
					head.Push(this);
				}

				T		&Item()
				{
					return item;
				}
				const T	&Item() const
				{
					return item;
				}

				Node	*Next() const
				{
					return static_cast<Node*>(EmbeddedList<Node>::Node::elNext);
				}

				Node	*Prev() const
				{
					return static_cast<Node*>(EmbeddedList<Node>::Node::elPrev);
				}

			private:
				friend class LinkedList;

				T		item;
		};

		LinkedList()
		{
		}
		LinkedList(const LinkedList &other)
		{
			typename EmbeddedList<Node>::ConstIterator iter = other.list.Head();
			if(iter)
			{
				while(iter.HasNext())
					++iter;

				do
				{
					new Node(iter->Item(), list);
				}
				while(--iter);
			}
			assert(Size() == other.Size());
		}
		~LinkedList()
		{
			Clear();
		}

		void Clear()
		{
			typename EmbeddedList<Node>::Iterator iter = list.Head();
			if(iter)
			{
				typename EmbeddedList<Node>::Iterator current;
				do
				{
					current = iter++;
					delete current;
				}
				while(iter);
				
			}
		}

		Node *Head()
		{
			return list.Head();
		}

		Node *Push(const T &item)
		{
			return new Node(item, list);
		}

		void Remove(Node *node)
		{
			list.Remove(node);
		}

		unsigned int Size() const
		{
			return list.Size();
		}

	private:
		typename EmbeddedList<Node>::List list;
};

#endif
