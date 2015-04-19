#pragma once
#include <QString>
#include <QMap>
#include <QStringList>

template <char Tseparator>
class SeparatorPrefixTree
{
public:
	SeparatorPrefixTree(QStringList paths)
	{
		insert(paths);
	}

	SeparatorPrefixTree(bool contained = false)
	{
		m_contained = contained;
	}

	void insert(QStringList paths)
	{
		for(auto &path: paths)
		{
			insert(path);
		}
	}

	/// insert an exact path into the tree
	SeparatorPrefixTree & insert(QString path)
	{
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			children[path] = SeparatorPrefixTree(true);
			return children[path];
		}
		else
		{
			auto prefix = path.left(sepIndex);
			if(!children.contains(prefix))
			{
				children[prefix] = SeparatorPrefixTree(false);
			}
			return children[prefix].insert(path.mid(sepIndex + 1));
		}
	}

	/// is the path fully contained in the tree?
	bool contains(QString path) const
	{
		auto node = find(path);
		return node != nullptr;
	}

	/// does the tree cover a path? That means the prefix of the path is contained in the tree
	bool covers(QString path) const
	{
		// if we found some valid node, it's good enough. the tree covers the path
		if(m_contained)
		{
			return true;
		}
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			auto found = children.find(path);
			if(found == children.end())
			{
				return false;
			}
			return (*found).covers(QString());
		}
		else
		{
			auto prefix = path.left(sepIndex);
			auto found = children.find(prefix);
			if(found == children.end())
			{
				return false;
			}
			return (*found).covers(path.mid(sepIndex + 1));
		}
	}

	/// return the contained path that covers the path specified
	QString cover(QString path) const
	{
		// if we found some valid node, it's good enough. the tree covers the path
		if(m_contained)
		{
			return QString("");
		}
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			auto found = children.find(path);
			if(found == children.end())
			{
				return QString();
			}
			auto nested = (*found).cover(QString());
			if(nested.isNull())
			{
				return nested;
			}
			if(nested.isEmpty())
				return path;
			return path + Tseparator + nested;
		}
		else
		{
			auto prefix = path.left(sepIndex);
			auto found = children.find(prefix);
			if(found == children.end())
			{
				return QString();
			}
			auto nested = (*found).cover(path.mid(sepIndex + 1));
			if(nested.isNull())
			{
				return nested;
			}
			if(nested.isEmpty())
				return prefix;
			return prefix + Tseparator + nested;
		}
	}

	/// Does the path-specified node exist in the tree? It does not have to be contained.
	bool exists(QString path) const
	{
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			auto found = children.find(path);
			if(found == children.end())
			{
				return false;
			}
			return true;
		}
		else
		{
			auto prefix = path.left(sepIndex);
			auto found = children.find(prefix);
			if(found == children.end())
			{
				return false;
			}
			return (*found).exists(path.mid(sepIndex + 1));
		}
	}

	/// find a node in the tree by name
	const SeparatorPrefixTree * find(QString path) const
	{
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			auto found = children.find(path);
			if(found == children.end())
			{
				return nullptr;
			}
			return &(*found);
		}
		else
		{
			auto prefix = path.left(sepIndex);
			auto found = children.find(prefix);
			if(found == children.end())
			{
				return nullptr;
			}
			return (*found).find(path.mid(sepIndex + 1));
		}
	}

	/// is this a leaf node?
	bool leaf() const
	{
		return children.isEmpty();
	}

	/// is this node actually contained in the tree, or is it purely structural?
	bool contained() const
	{
		return m_contained;
	}

	/// Remove a path from the tree
	bool remove(QString path)
	{
		return removeInternal(path) != Failed;
	}

	/// Clear all children of this node tree node
	void clear()
	{
		children.clear();
	}

	QStringList toStringList() const
	{
		QStringList collected;
		// collecting these is more expensive.
		auto iter = children.begin();
		while(iter != children.end())
		{
			QStringList list = iter.value().toStringList();
			for(int i = 0; i < list.size(); i++)
			{
				list[i] = iter.key() + Tseparator + list[i];
			}
			collected.append(list);
			if((*iter).m_contained)
			{
				collected.append(iter.key());
			}
			iter++;
		}
		return collected;
	}
private:
	enum Removal
	{
		Failed,
		Succeeded,
		HasChildren
	};
	Removal removeInternal(QString path = QString())
	{
		if(path.isEmpty())
		{
			if(!m_contained)
			{
				// remove all children - we are removing a prefix
				clear();
				return Succeeded;
			}
			m_contained = false;
			if(children.size())
			{
				return HasChildren;
			}
			return Succeeded;
		}
		Removal remStatus = Failed;
		QString childToRemove;
		auto sepIndex = path.indexOf(Tseparator);
		if(sepIndex == -1)
		{
			childToRemove = path;
			auto found = children.find(childToRemove);
			if(found == children.end())
			{
				return Failed;
			}
			remStatus = (*found).removeInternal();
		}
		else
		{
			childToRemove = path.left(sepIndex);
			auto found = children.find(childToRemove);
			if(found == children.end())
			{
				return Failed;
			}
			remStatus = (*found).removeInternal(path.mid(sepIndex + 1));
		}
		switch (remStatus)
		{
			case Failed:
			case HasChildren:
			{
				return remStatus;
			}
			case Succeeded:
			{
				children.remove(childToRemove);
				if(m_contained)
				{
					return HasChildren;
				}
				if(children.size())
				{
					return HasChildren;
				}
				return Succeeded;
			}
		}
		return Failed;
	}

private:
	QMap<QString,SeparatorPrefixTree<Tseparator>> children;
	bool m_contained = false;
};
