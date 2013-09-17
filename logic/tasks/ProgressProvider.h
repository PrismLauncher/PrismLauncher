#pragma once
#include <QObject>
class ProgressProvider : public QObject
{
	Q_OBJECT
protected:
    explicit ProgressProvider(QObject* parent = 0): QObject(parent){}
signals:
	void started();
	void progress(qint64 current, qint64 total);
	void succeeded();
	void failed(QString reason);
	void status(QString status);
public:
	virtual QString getStatus() const = 0;
	virtual void getProgress(qint64 &current, qint64 &total) = 0;
	virtual bool isRunning() const = 0;
public slots:
	virtual void start() = 0;
};
