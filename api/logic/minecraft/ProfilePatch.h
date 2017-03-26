#pragma once

#include <memory>
#include <QList>
#include <QJsonDocument>
#include <QDateTime>
#include "JarMod.h"

class MinecraftProfile;

enum ProblemSeverity
{
	PROBLEM_NONE,
	PROBLEM_WARNING,
	PROBLEM_ERROR
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

class ProfilePatch : public std::enable_shared_from_this<ProfilePatch>
{
public:
	virtual ~ProfilePatch(){};
	virtual void applyTo(MinecraftProfile *profile) = 0;

	virtual bool isMinecraftVersion() = 0;

	virtual bool isMoveable() = 0;
	virtual bool isCustomizable() = 0;
	virtual bool isRevertible() = 0;
	virtual bool isRemovable() = 0;
	virtual bool isCustom() = 0;
	virtual bool isEditable() = 0;
	virtual bool isVersionChangeable() = 0;

	virtual void setOrder(int order) = 0;
	virtual int getOrder() = 0;

	virtual QString getID() = 0;
	virtual QString getName() = 0;
	virtual QString getVersion() = 0;
	virtual QDateTime getReleaseDateTime() = 0;

	virtual QString getFilename() = 0;

	virtual std::shared_ptr<class VersionFile> getVersionFile() = 0;

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

protected:
	QList<PatchProblem> m_problems;
	ProblemSeverity m_problemSeverity = PROBLEM_NONE;
};

typedef std::shared_ptr<ProfilePatch> ProfilePatchPtr;
