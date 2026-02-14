/////////////////////////////////////////////////////////////////////////////
// projectsettingsdialog.cpp                                               //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "editor/projectsettingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QPalette>

ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("New Project Settings");
    setModal(true);
    resize(600, 400);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    setPalette(darkPalette);

    m_baseProjectPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DabozzProjects";

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* titleLabel = new QLabel("Create New Project", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(20);

    QFormLayout* formLayout = new QFormLayout();

    m_projectNameEdit = new QLineEdit(this);
    m_projectNameEdit->setPlaceholderText("MyGame");
    connect(m_projectNameEdit, &QLineEdit::textChanged, this, &ProjectSettingsDialog::onProjectNameChanged);
    formLayout->addRow("Project Name:", m_projectNameEdit);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_projectPathEdit = new QLineEdit(this);
    m_projectPathEdit->setReadOnly(true);
    pathLayout->addWidget(m_projectPathEdit);

    m_browseButton = new QPushButton("Browse...", this);
    connect(m_browseButton, &QPushButton::clicked, this, &ProjectSettingsDialog::onBrowse);
    pathLayout->addWidget(m_browseButton);

    formLayout->addRow("Project Path:", pathLayout);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("Lua", "lua");
    m_languageCombo->addItem("AngelScript", "angelscript");
    m_languageCombo->addItem("Both (Lua + AngelScript)", "both");
    formLayout->addRow("Scripting Language:", m_languageCombo);

    mainLayout->addLayout(formLayout);

    mainLayout->addSpacing(20);

    m_createSceneCheck = new QCheckBox("Create default scene", this);
    m_createSceneCheck->setChecked(true);
    mainLayout->addWidget(m_createSceneCheck);

    mainLayout->addStretch();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    m_createButton = new QPushButton("Create Project", this);
    m_createButton->setDefault(true);
    m_createButton->setEnabled(false);
    connect(m_createButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_createButton);

    mainLayout->addLayout(buttonLayout);

    updateProjectPath();
}

QString ProjectSettingsDialog::getProjectName() const
{
    return m_projectNameEdit->text();
}

QString ProjectSettingsDialog::getProjectPath() const
{
    return m_projectPathEdit->text();
}

QString ProjectSettingsDialog::getScriptingLanguage() const
{
    return m_languageCombo->currentData().toString();
}

bool ProjectSettingsDialog::shouldCreateDefaultScene() const
{
    return m_createSceneCheck->isChecked();
}

void ProjectSettingsDialog::onBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Location", m_baseProjectPath);
    if (!dir.isEmpty()) {
        m_baseProjectPath = dir;
        updateProjectPath();
    }
}

void ProjectSettingsDialog::onProjectNameChanged(const QString& text)
{
    updateProjectPath();
    m_createButton->setEnabled(!text.trimmed().isEmpty());
}

void ProjectSettingsDialog::updateProjectPath()
{
    QString projectName = m_projectNameEdit->text().trimmed();
    if (projectName.isEmpty()) {
        m_projectPathEdit->clear();
    } else {
        m_projectPathEdit->setText(m_baseProjectPath + "/" + projectName);
    }
}
