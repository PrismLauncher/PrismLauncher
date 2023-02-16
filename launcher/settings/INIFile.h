/* Copyright 2013-2021 MultiMC Contributors
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

#pragma once

#include <QString>
#include <QVariant>
#include <QIODevice>

#include <QJsonDocument>
#include <QJsonArray>

// Sectionless INI parser (for instance config files)
class INIFile : public QMap<QString, QVariant>
{
public:
    explicit INIFile();

    bool loadFile(QByteArray file);
    bool loadFile(QString fileName);
    bool saveFile(QString fileName);

    QVariant get(QString key, QVariant def) const;
    void set(QString key, QVariant val);
    static QString unescape(QString orig);
    static QString escape(QString orig);

    void setList(QString key, QVariantList val);
    template <typename T> void setList(QString key, QList<T> val)
    {
        QVariantList variantList;
        variantList.reserve(val.size());
        for (const T& v : val)
        {
            variantList.append(v);
        }

        this->setList(key, variantList);
    }

    QVariantList getList(QString key, QVariantList def) const;
    template <typename T> QList<T>  getList(QString key, QList<T> def) const
    {   
        if (this->contains(key)) {
            QVariant src = this->operator[](key);
            QVariantList variantList = QJsonDocument::fromJson(src.toByteArray()).toVariant().toList();

            QList<T>TList;
            TList.reserve(variantList.size());
            for (const QVariant& v : variantList)
            {
                TList.append(v.value<T>());
            }
            return TList;
        }

        return def;   
    }
};
