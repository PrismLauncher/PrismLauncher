#include "UntrustedModsDialog.h"
#include "ui_UntrustedModsDialog.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>

UntrustedModsDialog::UntrustedModsDialog(const QStringList& paths, QWidget* parent) : QDialog{ parent }, m_ui{ new Ui::UntrustedModsDialog }
{
    m_ui->setupUi(this);
    m_ui->modList->addItems(paths);

    auto* ok = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    ok->setEnabled(false);

    connect(m_ui->confirmCheckbox, &QAbstractButton::clicked, ok, &QWidget::setEnabled);

    m_ui->confirmCheckbox->setEnabled(false);
    QTimer::singleShot(3000, this, [this] { m_ui->confirmCheckbox->setEnabled(true); });
}

UntrustedModsDialog::~UntrustedModsDialog()
{
    delete m_ui;
}
