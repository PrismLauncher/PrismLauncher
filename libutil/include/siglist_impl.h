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

#include "siglist.h"

template <typename T>
void SigList<T>::append(const T &value)
{
	QList<T>::append(value);
	onItemAdded(value, QList<T>::length() - 1);
}

template <typename T>
void SigList<T>::prepend(const T &value)
{
	QList<T>::prepend(value);
	onItemAdded(value, 0);
}

template <typename T>
void SigList<T>::append(const QList<T> &other)
{
	int index = QList<T>::length();
	QList<T>::append(other);
	onItemsAdded(other, index);
}

template <typename T>
void SigList<T>::clear()
{
	QList<T>::clear();
	onInvalidated();
}

template <typename T>
void SigList<T>::erase(SigList<T>::iterator pos)
{
	T value = *pos;
	int index = indexOf(*pos);
	QList<T>::erase(pos);
	onItemRemoved(value, index);
}

template <typename T>
void SigList<T>::erase(SigList<T>::iterator first, SigList<T>::iterator last)
{
	QList<T> removedValues;
	int firstIndex = indexOf(*first);
	
	for (SigList<T>::iterator iter = first; iter < last; iter++)
	{
		removedValues << *iter;
		QList<T>::erase(iter);
	}
	
	onItemsRemoved(removedValues, firstIndex);
}

template <typename T>
void SigList<T>::insert(int i, const T &t)
{
	QList<T>::insert(i, t);
	onItemAdded(t, i);
}

template <typename T>
void SigList<T>::insert(SigList<T>::iterator before, const T &t)
{
	QList<T>::insert(before, t);
	onItemAdded(t, indexOf(t));
}

template <typename T>
void SigList<T>::move(int from, int to)
{
	const T &item = QList<T>::at(from);
	QList<T>::move(from, to);
	onItemMoved(item, from, to);
}

template <typename T>
int SigList<T>::removeAll(const T &t)
{
	int retVal = QList<T>::removeAll(t);
	onInvalidated();
	return retVal;
}

template <typename T>
bool SigList<T>::removeOne(const T &t)
{
	int index = indexOf(t);
	if (QList<T>::removeOne(t))
	{
		onItemRemoved(t, index);
		return true;
	}
	return false;
}

template <typename T>
void SigList<T>::swap(QList<T> &other)
{
	QList<T>::swap(other);
	onInvalidated();
}

template <typename T>
void SigList<T>::swap(int i, int j)
{
	const T &item1 = QList<T>::at(i);
	const T &item2 = QList<T>::at(j);
	QList<T>::swap(i, j);
	onItemMoved(item1, i, j);
	onItemMoved(item2, j, i);
}

template <typename T>
T SigList<T>::takeAt(int i)
{
	T val = QList<T>::takeAt(i);
	onItemRemoved(val, i);
	return val;
}

template <typename T>
T SigList<T>::takeFirst()
{
	return takeAt(0);
}

template <typename T>
T SigList<T>::takeLast()
{
	return takeAt(QList<T>::length() - 1);
}

template <typename T>
QList<T> &SigList<T>::operator =(const QList<T> &other)
{
	QList<T>::operator =(other);
	onInvalidated();
	return *this;
}
