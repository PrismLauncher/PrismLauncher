#pragma once

#include <map>
#include <QString>
#include <QAbstractListModel>

struct GameOptionItem
{
    QString key;
    QString value;
};

class GameOptions : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit GameOptions(const QString& path);
    virtual ~GameOptions() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool isLoaded() const;
    bool reload();
    bool save();

private:
    std::vector<GameOptionItem> contents;
    bool loaded = false;
    QString path;
    int version = 0;
};
