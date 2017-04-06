#pragma once

enum class ProblemSeverity
{
	None,
	Warning,
	Error
};

class PatchProblem
{
public:
	PatchProblem(ProblemSeverity severity, const QString & description)
	{
		m_severity = severity;
		m_description = description;
	}
	const QString & getDescription() const
	{
		return m_description;
	}
	const ProblemSeverity getSeverity() const
	{
		return m_severity;
	}
private:
	ProblemSeverity m_severity;
	QString m_description;
};

class ProblemProvider
{
public:
	virtual const QList<PatchProblem> getProblems() = 0;
	virtual ProblemSeverity getProblemSeverity() = 0;
};

class ProblemContainer : public ProblemProvider
{
public:
	const QList<PatchProblem> getProblems() override
	{
		return m_problems;
	}
	ProblemSeverity getProblemSeverity() override
	{
		return m_problemSeverity;
	}
	virtual void addProblem(ProblemSeverity severity, const QString &description)
	{
		if(severity > m_problemSeverity)
		{
			m_problemSeverity = severity;
		}
		m_problems.append(PatchProblem(severity, description));
	}

private:
	QList<PatchProblem> m_problems;
	ProblemSeverity m_problemSeverity = ProblemSeverity::None;
};
