#pragma once
#include <QWidget>

#include <java/JavaChecker.h>
#include <BaseVersion.h>
#include <QObjectPtr.h>
#include <QIcon>

class QLineEdit;
class VersionSelectWidget;
class QSpinBox;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QGridLayout;
class QLabel;
class QToolButton;

/**
 * This is a widget for all the Java settings dialogs and pages.
 */
class JavaSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit JavaSettingsWidget(QWidget *parent);
    virtual ~JavaSettingsWidget() {};

    enum class JavaStatus
    {
        NotSet,
        Pending,
        Good,
        DoesNotExist,
        DoesNotStart,
        ReturnedInvalidData
    } javaStatus = JavaStatus::NotSet;

    enum class ValidationStatus
    {
        Bad,
        JavaBad,
        AllOK
    };

    void refresh();
    void initialize();
    ValidationStatus validate();
    void retranslate();

    bool permGenEnabled() const;
    int permGenSize() const;
    int minHeapSize() const;
    int maxHeapSize() const;
    QString javaPath() const;

    void updateThresholds();


protected slots:
    void memoryValueChanged(int);
    void javaPathEdited(const QString &path);
    void javaVersionSelected(BaseVersion::Ptr version);
    void on_javaBrowseBtn_clicked();
    void on_javaStatusBtn_clicked();
    void checkFinished(JavaCheckResult result);

protected: /* methods */
    void checkJavaPathOnEdit(const QString &path);
    void checkJavaPath(const QString &path);
    void setJavaStatus(JavaStatus status);
    void setupUi();

private: /* data */
    VersionSelectWidget *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_versionWidget = nullptr;
    QVBoxLayout *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_verticalLayout = nullptr;

    QLineEdit * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaPathTextBox = nullptr;
    QPushButton * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaBrowseBtn = nullptr;
    QToolButton * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javaStatusBtn = nullptr;
    QHBoxLayout *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_horizontalLayout = nullptr;

    QGroupBox *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_memoryGroupBox = nullptr;
    QGridLayout *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gridLayout_2 = nullptr;
    QSpinBox *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_maxMemSpinBox = nullptr;
    QLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMinMem = nullptr;
    QLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMem = nullptr;
    QLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelMaxMemIcon = nullptr;
    QSpinBox *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minMemSpinBox = nullptr;
    QLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_labelPermGen = nullptr;
    QSpinBox *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_permGenSpinBox = nullptr;
    QIcon goodIcon;
    QIcon yellowIcon;
    QIcon badIcon;

    unsigned int observedMinMemory = 0;
    unsigned int observedMaxMemory = 0;
    unsigned int observedPermGenMemory = 0;
    QString queuedCheck;
    uint64_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_availableMemory = 0ull;
    shared_qobject_ptr<JavaChecker> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checker;
    JavaCheckResult hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result;
};
