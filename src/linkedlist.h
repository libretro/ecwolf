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

template<class T> class LinkedList
{
	public:
		class Node
		{
			public:
				Node(const T &item, Node *&head) : item(item), next(head), prev(NULL)
				{
					if(head != NULL)
						head->prev = this;
					head = this;
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
					return next;
				}

				Node	*Prev() const
				{
					return prev;
				}

			private:
				friend class LinkedList;

				T		item;
				Node	*next;
				Node	*prev;
		};

		LinkedList() : head(NULL), size(0)
		{
		}
		LinkedList(const LinkedList &other) : head(NULL), size(0)
		{
			Node *iter = other.Head();
			if(iter != NULL)
			{
				while(iter->Next())
					iter = iter->Next();
			}

			for(;iter;iter = iter->Prev())
				Push(iter->Item());
		}
		~LinkedList()
		{
			Clear();
		}

		void Clear()
		{
			if(!head)
				return;

			Node *node = head;
			Node *del = NULL;
			do
			{
				delete del;
				del = node;
			}
			while((node = node->next) != NULL);
			delete del;
			head = NULL;
		}

		Node *Head() const
		{
			return head;
		}

		Node *Push(const T &item)
		{
			++size;
			return new Node(item, head);
		}

		void Remove(Node *node)
		{
			if(node->next)
				node->next->prev = node->prev;

			if(node->prev)
				node->prev->next = node->next;
			else
				head = head->next;

			delete node;
			--size;
		}

		unsigned int Size() const
		{
			return size;
		}

	private:
		Node			*head;
		unsigned int	size;
};

#endif
