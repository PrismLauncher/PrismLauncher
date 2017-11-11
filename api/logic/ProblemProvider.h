#pragma once

#include "multimc_logic_export.h"

enum class ProblemSeverity
{
	None,
	Warning,
	Error
};

struct PatchProblem
{
	ProblemSeverity m_severity;
	QString m_description;
};

class MULTIMC_LOGIC_EXPORT ProblemProvider
{
public:
	virtual ~ProblemProvider() {};
	virtual const QList<PatchProblem> getProblems() const = 0;
	virtual ProblemSeverity getProblemSeverity() const = 0;
};

class MULTIMC_LOGIC_EXPORT ProblemContainer : public ProblemProvider
{
public:
	const QList<PatchProblem> getProblems() const override
	{
		return m_problems;
	}
	ProblemSeverity getProblemSeverity() const override
	{
		return m_problemSeverity;
	}
	virtual void addProblem(ProblemSeverity severity, const QString &description)
	{
		if(severity > m_problemSeverity)
		{
			m_problemSeverity = severity;
		}
		m_problems.append({severity, description});
	}

private:
	QList<PatchProblem> m_problems;
	ProblemSeverity m_problemSeverity = ProblemSeverity::None;
};
