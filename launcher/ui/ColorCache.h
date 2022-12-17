#pragma once
#include <QtGui/QColor>
#include <rainbow.h>
#include <MessageLevel.h>
#include <QMap>

class ColorCache
{
public:
    ColorCache(QColor front, QColor back, qreal bias)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front = front;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back = back;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bias = bias;
    };

    void addColor(int key, QColor color)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors[key] = {color, blend(color), blendBackground(color)};
    }

    void setForeground(QColor front)
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front != front)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front = front;
            recolorAll();
        }
    }

    void setBackground(QColor back)
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back != back)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back = back;
            recolorAll();
        }
    }

    QColor getFront(int key)
    {
        auto iter = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.find(key);
        if(iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.end())
        {
            return QColor();
        }
        return (*iter).front;
    }

    QColor getBack(int key)
    {
        auto iter = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.find(key);
        if(iter == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.end())
        {
            return QColor();
        }
        return (*iter).back;
    }

    /**
     * Blend the color with the front color, adapting to the back color
     */
    QColor blend(QColor color);

    /**
     * Blend the color with the back color
     */
    QColor blendBackground(QColor color);

protected:
    void recolorAll();

protected:
    struct ColorEntry
    {
        QColor original;
        QColor front;
        QColor back;
    };

protected:
    qreal hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bias;
    QColor hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front;
    QColor hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back;
    QMap<int, ColorEntry> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors;
};

class LogColorCache : public ColorCache
{
public:
    LogColorCache(QColor front, QColor back)
        : ColorCache(front, back, 1.0)
    {
        addColor((int)MessageLevel::Launcher, QColor("purple"));
        addColor((int)MessageLevel::Debug, QColor("green"));
        addColor((int)MessageLevel::Warning, QColor("orange"));
        addColor((int)MessageLevel::Error, QColor("red"));
        addColor((int)MessageLevel::Fatal, QColor("red"));
        addColor((int)MessageLevel::Message, front);
    }

    QColor getFront(MessageLevel::Enum level)
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.contains((int) level))
        {
            return ColorCache::getFront((int)MessageLevel::Message);
        }
        return ColorCache::getFront((int)level);
    }

    QColor getBack(MessageLevel::Enum level)
    {
        if(level == MessageLevel::Fatal)
        {
            return QColor(Qt::black);
        }
        return QColor(Qt::transparent);
    }
};
