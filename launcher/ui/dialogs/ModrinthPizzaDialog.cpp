#include "ModrinthPizzaDialog.h"
#include "ui_ModrinthPizzaDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include "FileSystem.h"
#include "minecraft/PackProfile.h"

ModrinthPizzaDialog::ModrinthPizzaDialog(MinecraftInstancePtr instance, QWidget* parent)
    : QDialog(parent), m_ui(new Ui::ModrinthPizzaDialog), m_instance(instance), m_seed(QDateTime::currentMSecsSinceEpoch())
{
    m_ui->setupUi(this);

    for (QString key : Component::KNOWN_MODLOADERS.keys()) {
        const ComponentPtr component = instance->getPackProfile()->getComponent(key);
        if (component != nullptr && component->isEnabled())
            m_ui->modLoader->setText(tr("Mod Loader: %1").arg(component->getName()));
    }

    const ComponentPtr minecraft = instance->getPackProfile()->getComponent("net.minecraft");
    if (minecraft != nullptr)
        m_ui->gameVersion->setText(tr("Game Version: %1").arg(minecraft->getVersion()));

    updateFlavour();
    connect(m_ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModrinthPizzaDialog::updateFlavour);
    connect(m_ui->checkBox, &QCheckBox::toggled, this, &ModrinthPizzaDialog::updateFlavour);
}

ModrinthPizzaDialog::~ModrinthPizzaDialog()
{
    delete m_ui;
}

void ModrinthPizzaDialog::done(int result)
{
    if (result != Accepted) {
        QDialog::done(result);
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Pizza"), FS::PathCombine(QDir::homePath(), "pizza.mrpizza"),
                                                    tr("Modrinth Pizza") + " (*.mrpizza)");

    if (fileName.isEmpty())
        return;

    // so it's readable :)
    fileName += ".txt";

    QFile file(fileName);

    if (!file.open(QFile::WriteOnly))
        return;

    file.write("An error occurred. Please contact us at https://www.prismsocial.net/.");

    QDialog::done(result);
}

void ModrinthPizzaDialog::updateFlavour()
{
    QStringList craftyWords = { "Ghast", "Wither", "Creeper", "Skeleton", "Zombie", "Spider", "Shulker", "Ender", "Nether", "Dolphin" };

    QStringList foodyWords = { tr("Margherita"), tr("Marinara"),   tr("Pepperoni"), tr("Greek"),
                               tr("New York"),   tr("Neapolitan"), tr("Hawaiian") };

    long seed = m_seed & 0xFF;
    seed |= m_ui->comboBox->currentIndex() << 8;
    if (m_ui->checkBox->isChecked())
        seed |= 1L << 16;

    QRandomGenerator random(seed);

    const QString craftyWord = craftyWords[static_cast<int>(random.generateDouble() * craftyWords.length())];
    const QString foodyWord = foodyWords[static_cast<int>(random.generateDouble() * foodyWords.length())];
    m_ui->flavourResult->setText(tr("Your flavour is: %1 %2... delicious!").arg(craftyWord, foodyWord));
}
