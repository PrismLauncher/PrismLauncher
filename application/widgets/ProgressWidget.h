// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QWidget>
#include <memory>

class Task;
class QProgressBar;
class QLabel;

class ProgressWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ProgressWidget(QWidget *parent = nullptr);

public slots:
	void start(std::shared_ptr<Task> task);
	bool exec(std::shared_ptr<Task> task);

private slots:
	void handleTaskFinish();
	void handleTaskStatus(const QString &status);
	void handleTaskProgress(qint64 current, qint64 total);
	void taskDestroyed();

private:
	QLabel *m_label;
	QProgressBar *m_bar;
	std::shared_ptr<Task> m_task;
};
