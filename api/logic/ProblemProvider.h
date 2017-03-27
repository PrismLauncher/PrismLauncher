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
	virtual const QList<PatchProblem>& getProblems()
	{
		return m_problems;
	}
	virtual void addProblem(ProblemSeverity severity, const QString &description)
	{
		if(severity > m_problemSeverity)
		{
			m_problemSeverity = severity;
		}
		m_problems.append(PatchProblem(severity, description));
	}
	virtual ProblemSeverity getProblemSeverity()
	{
		return m_problemSeverity;
	}

private:
	QList<PatchProblem> m_problems;
	ProblemSeverity m_problemSeverity = ProblemSeverity::None;
};
