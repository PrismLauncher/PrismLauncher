#pragma once

enum class ProblemSeverity
{
    None,
    Warning,
    Error
};

struct PatchProblem
{
    ProblemSeverity hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_severity;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_description;
};

class ProblemProvider
{
public:
    virtual ~ProblemProvider() {};
    virtual const QList<PatchProblem> getProblems() const = 0;
    virtual ProblemSeverity getProblemSeverity() const = 0;
};

class ProblemContainer : public ProblemProvider
{
public:
    const QList<PatchProblem> getProblems() const override
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problems;
    }
    ProblemSeverity getProblemSeverity() const override
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity;
    }
    virtual void addProblem(ProblemSeverity severity, const QString &description)
    {
        if(severity > hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity = severity;
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problems.append({severity, description});
    }

private:
    QList<PatchProblem> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problems;
    ProblemSeverity hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_problemSeverity = ProblemSeverity::None;
};
