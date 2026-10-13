// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "JavaSettingsWidget.h"

#include <array>

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QPainter>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyleOptionSlider>
#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "HardwareInfo.h"
#include "JavaCommon.h"
#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/Setting.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/java/InstallJavaDialog.h"
#include "ui/themes/FreshTheme.h"

#include "ui_JavaSettingsWidget.h"

namespace {
constexpr int MIB_PER_GIB = 1024;

// Fresh memory slider, painted by hand: stylesheets drop native tick marks and can't place them under the groove.
// Its values are whole GiB, so it can only rest on a mark. Geometry comes from the style's groove/handle rects,
// so mouse handling stays QSlider's own.
class MemorySlider : public QSlider {
   public:
    MemorySlider(QWidget* parent) : QSlider(Qt::Horizontal, parent)
    {
        // fresh.qss resets the generic slider min-height for this name, so minimumSizeHint() is what layouts honor
        setObjectName(QStringLiteral("memorySlider"));
        setAttribute(Qt::WA_Hover);
    }

    // via size hints: a stylesheet min-height on QSlider would override setMinimumHeight()
    QSize sizeHint() const override { return { QSlider::sizeHint().width(), totalHeight() }; }
    QSize minimumSizeHint() const override { return { QSlider::minimumSizeHint().width(), totalHeight() }; }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        if (maximum() <= minimum())
            return;

        QStyleOptionSlider option;
        initStyleOption(&option);
        const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
        const int span = groove.width() - handle.width();
        const int origin = groove.left() + handle.width() / 2;
        const int handleX = handle.center().x();
        const double pixelsPerGiB = double(span) / (maximum() - minimum());
        const bool enabled = isEnabled();
        const qreal trackCenter = TRACK_TOP + TRACK_HEIGHT / 2.0;
        const qreal trackRadius = TRACK_HEIGHT / 2.0;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        // Material slider: active and inactive track, split by a gap around the handle
        // the track spans the groove exactly; extending it past the handle's travel got it clipped at the edges,
        // and at the ends the active (or inactive) part is simply absent, as in Material
        const qreal trackLeft = groove.left();
        const qreal trackRight = groove.right() + 1;
        if (handleX - HANDLE_GAP - trackLeft >= MIN_SEGMENT) {
            painter.setBrush(FreshTheme::color(enabled ? "accent" : "text-disabled"));
            painter.drawRoundedRect(QRectF(trackLeft, TRACK_TOP, handleX - HANDLE_GAP - trackLeft, TRACK_HEIGHT), trackRadius, trackRadius);
        }
        if (trackRight - handleX - HANDLE_GAP >= MIN_SEGMENT) {
            painter.setBrush(FreshTheme::color(enabled ? "secondary-container" : "surface-hover"));
            painter.drawRoundedRect(QRectF(handleX + HANDLE_GAP, TRACK_TOP, trackRight - handleX - HANDLE_GAP, TRACK_HEIGHT), trackRadius,
                                    trackRadius);
        }

        // GiB stops as dots on the track, thinned out on machines with lots of RAM, with labels where there is room
        int tickEvery = 1;
        while (tickEvery * pixelsPerGiB < 20)
            tickEvery *= 2;
        int labelEvery = tickEvery;
        while (labelEvery * pixelsPerGiB < 52)
            labelEvery *= 2;

        painter.setFont(labelFont());
        for (int gib = (minimum() + tickEvery - 1) / tickEvery * tickEvery; gib <= maximum(); gib += tickEvery) {
            const int x = origin + QStyle::sliderPositionFromValue(minimum(), maximum(), gib, span);
            const bool reached = enabled && gib <= value();

            if (qAbs(x - handleX) > HANDLE_GAP + 2) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(FreshTheme::color(reached ? "on-accent" : "on-secondary-container"));
                painter.drawEllipse(QPointF(x, trackCenter), STOP_RADIUS, STOP_RADIUS);
            }

            if (gib % labelEvery == 0) {
                // centered under the stop, but kept inside the widget at both ends
                const QString text = QObject::tr("%1 GiB").arg(gib);
                const int textWidth = painter.fontMetrics().horizontalAdvance(text);
                const int left = qBound(0, x - textWidth / 2, width() - textWidth);
                painter.setPen(FreshTheme::color(reached ? "text" : "text-muted"));
                painter.drawText(QRect(left, LABEL_TOP, textWidth, painter.fontMetrics().height()), Qt::AlignLeft | Qt::AlignTop, text);
            }
        }

        // handle: a narrow vertical bar, thinner while dragged
        const qreal handleWidth = isSliderDown() ? HANDLE_WIDTH - 1 : HANDLE_WIDTH;
        painter.setPen(Qt::NoPen);
        painter.setBrush(FreshTheme::color(!enabled ? "text-disabled" : underMouse() ? "accent-hover" : "accent"));
        painter.drawRoundedRect(QRectF(handleX - handleWidth / 2, trackCenter - HANDLE_HEIGHT / 2.0, handleWidth, HANDLE_HEIGHT),
                                handleWidth / 2, handleWidth / 2);
    }

   private:
    static constexpr int HANDLE_HEIGHT = 30;
    static constexpr qreal HANDLE_WIDTH = 4;
    static constexpr int HANDLE_GAP = 6;
    static constexpr int TRACK_HEIGHT = 12;
    static constexpr int TRACK_TOP = (HANDLE_HEIGHT - TRACK_HEIGHT) / 2;
    static constexpr qreal STOP_RADIUS = 2;
    static constexpr qreal MIN_SEGMENT = 4;  // shorter track pieces would show as slivers
    static constexpr int LABEL_TOP = HANDLE_HEIGHT + 6;

    QFont labelFont() const
    {
        QFont labels = font();
        labels.setPointSizeF(labels.pointSizeF() * 0.85);
        return labels;
    }

    // track, handle and labels; follows the real font so labels are never clipped
    int totalHeight() const { return LABEL_TOP + QFontMetrics(labelFont()).height() + 4; }
};

// A slider for a memory spin box, from 1 GiB to the system RAM in whole GiB.
// The spin box keeps MiB precision; typed values show on the nearest mark.
MemorySlider* createMemorySlider(QSpinBox* spinBox, int maxGiB)
{
    auto* slider = new MemorySlider(nullptr);
    slider->setRange(1, maxGiB);
    slider->setSingleStep(1);
    slider->setPageStep(1);
    slider->setToolTip(spinBox->toolTip());
    // start in sync: loading settings only reaches the slider if it changes the spin box value
    slider->setValue(qRound(spinBox->value() / double(MIB_PER_GIB)));

    QObject::connect(slider, &QSlider::valueChanged, spinBox, [spinBox](int gib) { spinBox->setValue(gib * MIB_PER_GIB); });
    // blocked so syncing back (rounded, or clamped to the system RAM) never rewrites the spin box
    QObject::connect(spinBox, &QSpinBox::valueChanged, slider, [slider](int mib) {
        const QSignalBlocker blocker(slider);
        slider->setValue(qRound(mib / double(MIB_PER_GIB)));
    });
    return slider;
}

// Fresh: the memory form becomes a grid with a second column holding the min/max sliders on a slightly
// different panel, so both sliders start and end at the same place (the unit labels have different widths).
void addMemorySliders(Ui::JavaSettingsWidget* ui)
{
    const int maxGiB = static_cast<int>(std::min<uint64_t>(HardwareInfo::totalRamMiB(), ui->maxMemSpinBox->maximum()) / MIB_PER_GIB);
    if (maxGiB < 2)
        return;

    constexpr int panelPadding = 12;
    QFormLayout* form = ui->formLayout;
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(form->horizontalSpacing());
    grid->setVerticalSpacing(form->verticalSpacing());

    // columns: 0 label, 1 spin box + unit, 2..4 slider panel (padding, slider, padding)
    for (int row = 0, rows = form->rowCount(); row < rows; ++row) {
        const QFormLayout::TakeRowResult taken = form->takeRow(0);
        if (taken.labelItem != nullptr)
            grid->addItem(taken.labelItem, row, 0);
        if (taken.fieldItem != nullptr)
            grid->addItem(taken.fieldItem, row, 1);
    }

    auto* panel = new QFrame;
    panel->setObjectName(QStringLiteral("memorySliderPanel"));
    grid->addWidget(panel, 0, 2, 2, 3);
    grid->setColumnMinimumWidth(2, panelPadding);
    grid->setColumnMinimumWidth(4, panelPadding);
    grid->setColumnStretch(3, 1);

    const std::array<QSpinBox*, 2> spinBoxes{ ui->minMemSpinBox, ui->maxMemSpinBox };
    for (int row = 0; row < 2; ++row) {
        auto* slider = createMemorySlider(spinBoxes[row], maxGiB);
        grid->addWidget(slider, row, 3, Qt::AlignVCenter);
        grid->setRowMinimumHeight(row, slider->sizeHint().height() + 2 * panelPadding);
    }

    QVBoxLayout* groupLayout = ui->verticalLayout_2;
    const int index = groupLayout->indexOf(form);
    groupLayout->removeItem(form);
    delete form;
    ui->formLayout = nullptr;
    groupLayout->insertLayout(index, grid);
    panel->lower();  // behind the sliders it spans
}
}  // namespace

JavaSettingsWidget::JavaSettingsWidget(MinecraftInstance* instance, QWidget* parent)
    : QWidget(parent), m_instance(instance), m_ui(new Ui::JavaSettingsWidget)
{
    m_ui->setupUi(this);

    if (m_instance == nullptr) {
        m_ui->javaDownloadBtn->hide();
        if (BuildConfig.JAVA_DOWNLOADER_ENABLED) {
            connect(m_ui->autodetectJavaCheckBox, &QCheckBox::checkStateChanged, this, [this](bool state) {
                m_ui->autodownloadJavaCheckBox->setEnabled(state);
                if (!state) {
                    m_ui->autodownloadJavaCheckBox->setChecked(false);
                }
            });
        } else {
            m_ui->autodownloadJavaCheckBox->hide();
        }
    } else {
        m_ui->javaDownloadBtn->setVisible(BuildConfig.JAVA_DOWNLOADER_ENABLED);
        m_ui->skipWizardCheckBox->hide();
        m_ui->autodetectJavaCheckBox->hide();
        m_ui->autodownloadJavaCheckBox->hide();

        m_ui->javaInstallationGroupBox->setCheckable(true);
        m_ui->memoryGroupBox->setCheckable(true);
        m_ui->javaArgumentsGroupBox->setCheckable(true);

        SettingsObject* settings = m_instance->settings();

        connect(settings->getSetting("OverrideJavaLocation").get(), &Setting::SettingChanged, m_ui->javaInstallationGroupBox,
                [this, settings] { m_ui->javaInstallationGroupBox->setChecked(settings->get("OverrideJavaLocation").toBool()); });
        connect(settings->getSetting("JavaPath").get(), &Setting::SettingChanged, m_ui->javaInstallationGroupBox,
                [this, settings] { m_ui->javaPathTextBox->setText(settings->get("JavaPath").toString()); });

        connect(m_ui->javaDownloadBtn, &QPushButton::clicked, this, [this] {
            auto* javaDialog = new Java::InstallDialog({}, m_instance, this);
            javaDialog->exec();
        });
        connect(m_ui->javaPathTextBox, &QLineEdit::textChanged, this, [this](const QString& newValue) {
            if (m_instance->settings()->get("JavaPath").toString() != newValue) {
                m_instance->settings()->set("AutomaticJava", false);
            }
        });
    }

    connect(m_ui->javaTestBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaTest);
    connect(m_ui->javaDetectBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaAutodetect);
    connect(m_ui->javaBrowseBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaBrowse);

    connect(m_ui->maxMemSpinBox, &QSpinBox::valueChanged, this, &JavaSettingsWidget::updateThresholds);
    connect(m_ui->minMemSpinBox, &QSpinBox::valueChanged, this, &JavaSettingsWidget::updateThresholds);

    if (FreshTheme::isActive()) {
        addMemorySliders(m_ui);
    }

    loadSettings();
    updateThresholds();
}

JavaSettingsWidget::~JavaSettingsWidget()
{
    delete m_ui;
}

void JavaSettingsWidget::loadSettings()
{
    SettingsObject* settings = nullptr;

    if (m_instance != nullptr) {
        settings = m_instance->settings();
    } else {
        settings = APPLICATION->settings();
    }

    // Java Settings
    m_ui->javaInstallationGroupBox->setChecked(settings->get("OverrideJavaLocation").toBool());
    m_ui->javaPathTextBox->setText(settings->get("JavaPath").toString());

    m_ui->skipCompatibilityCheckBox->setChecked(settings->get("IgnoreJavaCompatibility").toBool());

    m_ui->javaArgumentsGroupBox->setChecked(m_instance == nullptr || settings->get("OverrideJavaArgs").toBool());
    m_ui->jvmArgsTextBox->setPlainText(settings->get("JvmArgs").toString());

    if (m_instance == nullptr) {
        m_ui->skipWizardCheckBox->setChecked(settings->get("IgnoreJavaWizard").toBool());
        m_ui->autodetectJavaCheckBox->setChecked(settings->get("AutomaticJavaSwitch").toBool());
        m_ui->autodownloadJavaCheckBox->setEnabled(m_ui->autodetectJavaCheckBox->isChecked());
        m_ui->autodownloadJavaCheckBox->setChecked(settings->get("AutomaticJavaDownload").toBool());
    }

    // Memory
    m_ui->memoryGroupBox->setChecked(m_instance == nullptr || settings->get("OverrideMemory").toBool());
    int min = settings->get("MinMemAlloc").toInt();
    int max = settings->get("MaxMemAlloc").toInt();
    if (min < max) {
        m_ui->minMemSpinBox->setValue(min);
        m_ui->maxMemSpinBox->setValue(max);
    } else {
        m_ui->minMemSpinBox->setValue(max);
        m_ui->maxMemSpinBox->setValue(min);
    }
    m_ui->permGenSpinBox->setValue(settings->get("PermGen").toInt());
    m_ui->lowMemWarningCheckBox->setChecked(settings->get("LowMemWarning").toBool());

    // Java arguments
    m_ui->javaArgumentsGroupBox->setChecked(m_instance == nullptr || settings->get("OverrideJavaArgs").toBool());
    m_ui->jvmArgsTextBox->setPlainText(settings->get("JvmArgs").toString());
}

void JavaSettingsWidget::saveSettings()
{
    SettingsObject* settings = nullptr;

    if (m_instance != nullptr) {
        settings = m_instance->settings();
    } else {
        settings = APPLICATION->settings();
    }

    // Java Install Settings
    bool javaInstall = m_instance == nullptr || m_ui->javaInstallationGroupBox->isChecked();

    if (m_instance != nullptr) {
        settings->set("OverrideJavaLocation", javaInstall);
    }

    if (javaInstall) {
        settings->set("JavaPath", m_ui->javaPathTextBox->text());
        settings->set("IgnoreJavaCompatibility", m_ui->skipCompatibilityCheckBox->isChecked());
    } else {
        settings->reset("JavaPath");
        settings->reset("IgnoreJavaCompatibility");
    }

    if (m_instance == nullptr) {
        settings->set("IgnoreJavaWizard", m_ui->skipWizardCheckBox->isChecked());
        settings->set("AutomaticJavaSwitch", m_ui->autodetectJavaCheckBox->isChecked());
        settings->set("AutomaticJavaDownload", m_ui->autodownloadJavaCheckBox->isChecked());
    }

    // Memory
    bool memory = m_instance == nullptr || m_ui->memoryGroupBox->isChecked();

    if (m_instance != nullptr) {
        settings->set("OverrideMemory", memory);
    }

    if (memory) {
        int min = m_ui->minMemSpinBox->value();
        int max = m_ui->maxMemSpinBox->value();
        if (min < max) {
            settings->set("MinMemAlloc", min);
            settings->set("MaxMemAlloc", max);
        } else {
            settings->set("MinMemAlloc", max);
            settings->set("MaxMemAlloc", min);
        }
        settings->set("PermGen", m_ui->permGenSpinBox->value());
        settings->set("LowMemWarning", m_ui->lowMemWarningCheckBox->isChecked());
    } else {
        settings->reset("MinMemAlloc");
        settings->reset("MaxMemAlloc");
        settings->reset("PermGen");
        settings->reset("LowMemWarning");
    }

    // Java arguments
    bool javaArgs = m_instance == nullptr || m_ui->javaArgumentsGroupBox->isChecked();

    if (m_instance != nullptr) {
        settings->set("OverrideJavaArgs", javaArgs);
    }

    if (javaArgs) {
        settings->set("JvmArgs", m_ui->jvmArgsTextBox->toPlainText().replace("\n", " "));
    } else {
        settings->reset("JvmArgs");
    }
}

void JavaSettingsWidget::onJavaBrowse()
{
    QString rawPath = QFileDialog::getOpenFileName(this, tr("Find Java executable"));

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (rawPath.isEmpty()) {
        return;
    }

    QString cookedPath = FS::NormalizePath(rawPath);
    QFileInfo javaInfo(cookedPath);
    if (!javaInfo.exists() || !javaInfo.isExecutable()) {
        return;
    }
    m_ui->javaPathTextBox->setText(cookedPath);
}

void JavaSettingsWidget::onJavaTest()
{
    if (m_checker != nullptr) {
        return;
    }

    QString jvmArgs;

    if (m_instance == nullptr || m_ui->javaArgumentsGroupBox->isChecked()) {
        jvmArgs = m_ui->jvmArgsTextBox->toPlainText().replace("\n", " ");
    } else {
        jvmArgs = APPLICATION->settings()->get("JvmArgs").toString();
    }

    m_checker.reset(new JavaCommon::TestCheck(this, m_ui->javaPathTextBox->text(), jvmArgs, m_ui->minMemSpinBox->value(),
                                              m_ui->maxMemSpinBox->value(), m_ui->permGenSpinBox->value()));
    connect(m_checker.get(), &JavaCommon::TestCheck::finished, this, [this] { m_checker.reset(); });
    m_checker->run();
}

void JavaSettingsWidget::onJavaAutodetect()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }

    VersionSelectDialog versionDialog(APPLICATION->javalist(), tr("Select a Java version"), this, true);
    versionDialog.setResizeOn(2);
    versionDialog.exec();

    if (versionDialog.result() == QDialog::Accepted && versionDialog.selectedVersion()) {
        JavaInstallPtr java = std::dynamic_pointer_cast<JavaInstall>(versionDialog.selectedVersion());
        m_ui->javaPathTextBox->setText(java->path);

        if (!java->is_64bit && m_ui->maxMemSpinBox->value() > 2048) {
            CustomMessageBox::selectable(this, tr("Confirm Selection"),
                                         tr("You selected a 32-bit version of Java.\n"
                                            "This installation does not support more than 2048MiB of RAM.\n"
                                            "Please make sure that the maximum memory value is lower."),
                                         QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Ok)
                ->exec();
        }
    }
}
void JavaSettingsWidget::updateThresholds()
{
    auto sysMiB = HardwareInfo::totalRamMiB();
    unsigned int maxMem = m_ui->maxMemSpinBox->value();
    unsigned int minMem = m_ui->minMemSpinBox->value();

    const QString warningColour(QStringLiteral("<span style='color:#f5c211'>%1</span>"));

    if (maxMem >= sysMiB) {
        m_ui->labelMaxMemNotice->setText(
            QString("<span style='color:red'>%1</span>").arg(tr("Your maximum memory allocation exceeds your system memory capacity.")));
        m_ui->labelMaxMemNotice->show();
    } else if (static_cast<uint64_t>(maxMem) * 10 > sysMiB * 9) {
        m_ui->labelMaxMemNotice->setText(warningColour.arg(tr("Your maximum memory allocation is close to your system memory capacity.")));
        m_ui->labelMaxMemNotice->show();
    } else if (maxMem < minMem) {
        m_ui->labelMaxMemNotice->setText(warningColour.arg(tr("Your maximum memory allocation is below the minimum memory allocation.")));
        m_ui->labelMaxMemNotice->show();
    } else {
        m_ui->labelMaxMemNotice->hide();
    }
}
