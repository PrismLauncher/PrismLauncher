/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SIGLIST_H
#define SIGLIST_H

#include <QObject>
#include <QList>

// A QList that allows emitting signals when the list changes.
// Since QObject doesn't support templates, to use this class with a 
// certain type, you should create a class deriving from SigList<T> and then
// call the DEFINE_SIGLIST_SIGNALS(T) and SETUP_SIGLIST_SIGNALS(T) macros.
template <typename T>
class SigList : public QList<T>
{
public:
	explicit SigList() : QList<T>() {}
	
	virtual void append(const T &value);
	virtual void append(const QList<T> &other);
	
	virtual void clear();
	
	virtual void erase(typename QList<T>::iterator pos);
	virtual void erase(typename QList<T>::iterator first, typename QList<T>::iterator last);
	
	virtual void insert(int i, const T &t);
	virtual void insert(typename QList<T>::iterator before, const T &t);
	
	virtual void move(int from, int to);
	
	virtual void pop_back() { takeLast(); }
	virtual void pop_front() { takeFirst(); }
	
	virtual void push_back(const T &t) { append(t); }
	virtual void push_front(const T &t) { prepend(t); }
	
	virtual void prepend(const T &t);
	
	virtual int removeAll(const T &t);
	virtual bool removeOne(const T &t);
	
	virtual void removeAt(int i) { takeAt(i); }
	virtual void removeFirst() { takeFirst(); }
	virtual void removeLast() { takeLast(); }
	
	virtual void swap(QList<T> &other);
	virtual void swap(int i, int j);
	
	virtual T takeAt(int i);
	virtual T takeFirst();
	virtual T takeLast();
	
	virtual QList<T> &operator +=(const QList<T> &other) { append(other); return *this; }
	virtual QList<T> &operator +=(const T &value) { append(value); return *this; }
	virtual QList<T> &operator <<(const QList<T> &other) { append(other); return *this; }
	virtual QList<T> &operator <<(const T &value) { append(value); return *this; }
	
	virtual QList<T> &operator =(const QList<T> &other);
	
protected:
	// Signal emitted after an item is added to the list. 
	// Contains a reference to item and the item's new index.
	virtual void onItemAdded(const T &item, int index) = 0;
	
	// Signal emitted after multiple items are added to the list at once.
	// The items parameter is a const reference to a QList of the items that 
	// were added.
	// The firstIndex parameter is the new index of the first item added.
	virtual void onItemsAdded(const QList<T> &items, int firstIndex) = 0;
	
	// Signal emitted after an item is removed to the list.
	// Contains a reference to the item and the item's old index.
	virtual void onItemRemoved(const T &item, int index) = 0;
	
	// Signal emitted after multiple items are removed from the list at once.
	// The items parameter is a const reference to a QList of the items that 
	// were added.
	// The firstIndex parameter is the new index of the first item added.
	virtual void onItemsRemoved(const QList<T> &items, int firstIndex) = 0;
	
	// Signal emitted after an item is moved to another index.
	// Contains the item, the old index, and the new index.
	virtual void onItemMoved(const T &item, int oldIndex, int newIndex) = 0;
	
	// Signal emitted after an operation that changes the whole list occurs.
	// This signal should be treated as if all data in the entire list was cleared 
	// and new data added in its place.
	virtual void onInvalidated() = 0;
};

// Defines the signals for a SigList
#define DEFINE_SIGLIST_SIGNALS(TYPE) \
	Q_SIGNAL void itemAdded(TYPE const &item, int index);\
	Q_SIGNAL void itemsAdded(const QList<TYPE> &items, int firstIndex);\
	Q_SIGNAL void itemRemoved(TYPE const &item, int index);\
	Q_SIGNAL void itemsRemoved(const QList<TYPE> &items, int firstIndex);\
	Q_SIGNAL void itemMoved(TYPE const &item, int oldIndex, int newIndex);\
	Q_SIGNAL void invalidated();

// Overrides the onItem* functions and causes them to emit their corresponding 
// signals.
#define SETUP_SIGLIST_SIGNALS(TYPE) \
	virtual void onItemAdded(TYPE const &item, int index)\
	{ emit itemAdded(item, index); }\
	virtual void onItemsAdded(const QList<TYPE> &items, int firstIndex)\
	{ emit itemsAdded(items, firstIndex); }\
	virtual void onItemRemoved(TYPE const &item, int index)\
	{ emit itemRemoved(item, index); }\
	virtual void onItemsRemoved(const QList<TYPE> &items, int firstIndex)\
	{ emit itemsRemoved(items, firstIndex); }\
	virtual void onItemMoved(TYPE const &item, int oldIndex, int newIndex)\
	{ emit itemMoved(item, oldIndex, newIndex); }\
	virtual void onInvalidated() { emit invalidated(); }

#endif // SIGLIST_H
