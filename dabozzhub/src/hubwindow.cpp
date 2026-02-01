/**************************************************************************/
/*  hubwindow.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                           DABOZZ ENGINE                                */
/**************************************************************************/
/* Copyright (c) 2026-present DabozzEngine contributors.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "hubwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QFrame>

HubWindow::HubWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_manager = new ProjectManager(this);
    setWindowTitle("DabozzHub");
    resize(900, 550);
    setupUI();
    refreshProjectList();
    refreshEngineVersionList();
}

void HubWindow::setupUI() {
    QWidget* central = new QWidget;
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    QFrame* sidebar = new QFrame;
    sidebar->setFixedWidth(180);
    sidebar->setStyleSheet(
        "QFrame { background-color: #2b2b2b; }"
        "QPushButton { text-align: left; padding: 12px 16px; border: none; color: #ccc; font-size: 14px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:checked { background-color: #444; color: white; }"
    );

    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 20, 0, 0);
    sidebarLayout->setSpacing(2);

    QLabel* logo = new QLabel("DabozzHub");
    logo->setStyleSheet("color: white; font-size: 18px; font-weight: bold; padding: 10px 16px 20px 16px;");
    sidebarLayout->addWidget(logo);

    m_projectsBtn = new QPushButton("Projects");
    m_projectsBtn->setCheckable(true);
    m_projectsBtn->setChecked(true);
    connect(m_projectsBtn, &QPushButton::clicked, this, &HubWindow::showProjectsPage);
    sidebarLayout->addWidget(m_projectsBtn);

    m_enginesBtn = new QPushButton("Engine Versions");
    m_enginesBtn->setCheckable(true);
    connect(m_enginesBtn, &QPushButton::clicked, this, &HubWindow::showEngineVersionsPage);
    sidebarLayout->addWidget(m_enginesBtn);

    sidebarLayout->addStretch();

    // Content area
    m_stack = new QStackedWidget;

    // Projects page
    QWidget* projectsPage = new QWidget;
    QVBoxLayout* projLayout = new QVBoxLayout(projectsPage);
    projLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout* projHeader = new QHBoxLayout;
    QLabel* projTitle = new QLabel("Projects");
    projTitle->setStyleSheet("font-size: 20px; font-weight: bold;");
    projHeader->addWidget(projTitle);
    projHeader->addStretch();

    QPushButton* newBtn = new QPushButton("New Project");
    QPushButton* addBtn = new QPushButton("Add Existing");
    QPushButton* removeBtn = new QPushButton("Remove");
    QPushButton* openBtn = new QPushButton("Open");

    for (auto* btn : {newBtn, addBtn, removeBtn, openBtn}) {
        btn->setStyleSheet(
            "QPushButton { padding: 8px 16px; background-color: #3a3a3a; color: white; border: 1px solid #555; border-radius: 4px; }"
            "QPushButton:hover { background-color: #4a4a4a; }"
        );
    }
    openBtn->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #2563eb; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );

    projHeader->addWidget(newBtn);
    projHeader->addWidget(addBtn);
    projHeader->addWidget(removeBtn);
    projHeader->addWidget(openBtn);
    projLayout->addLayout(projHeader);

    m_projectList = new QListWidget;
    m_projectList->setStyleSheet(
        "QListWidget { background-color: #1e1e1e; border: 1px solid #333; border-radius: 4px; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #333; color: white; }"
        "QListWidget::item:selected { background-color: #2563eb; }"
        "QListWidget::item:hover { background-color: #2a2a2a; }"
    );
    projLayout->addWidget(m_projectList);

    connect(newBtn, &QPushButton::clicked, this, &HubWindow::onNewProject);
    connect(addBtn, &QPushButton::clicked, this, &HubWindow::onAddExisting);
    connect(removeBtn, &QPushButton::clicked, this, &HubWindow::onRemoveProject);
    connect(openBtn, &QPushButton::clicked, this, &HubWindow::onOpenProject);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &HubWindow::onProjectDoubleClicked);

    m_stack->addWidget(projectsPage);

    // Engine versions page
    QWidget* enginesPage = new QWidget;
    QVBoxLayout* engLayout = new QVBoxLayout(enginesPage);
    engLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout* engHeader = new QHBoxLayout;
    QLabel* engTitle = new QLabel("Engine Versions");
    engTitle->setStyleSheet("font-size: 20px; font-weight: bold;");
    engHeader->addWidget(engTitle);
    engHeader->addStretch();

    QPushButton* addEngBtn = new QPushButton("Add Version");
    QPushButton* removeEngBtn = new QPushButton("Remove");
    for (auto* btn : {addEngBtn, removeEngBtn}) {
        btn->setStyleSheet(
            "QPushButton { padding: 8px 16px; background-color: #3a3a3a; color: white; border: 1px solid #555; border-radius: 4px; }"
            "QPushButton:hover { background-color: #4a4a4a; }"
        );
    }
    engHeader->addWidget(addEngBtn);
    engHeader->addWidget(removeEngBtn);
    engLayout->addLayout(engHeader);

    m_engineList = new QListWidget;
    m_engineList->setStyleSheet(
        "QListWidget { background-color: #1e1e1e; border: 1px solid #333; border-radius: 4px; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #333; color: white; }"
        "QListWidget::item:selected { background-color: #2563eb; }"
        "QListWidget::item:hover { background-color: #2a2a2a; }"
    );
    engLayout->addWidget(m_engineList);

    connect(addEngBtn, &QPushButton::clicked, this, &HubWindow::onAddEngineVersion);
    connect(removeEngBtn, &QPushButton::clicked, this, &HubWindow::onRemoveEngineVersion);

    m_stack->addWidget(enginesPage);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(m_stack, 1);

    // Dark theme
    setStyleSheet("QMainWindow { background-color: #1e1e1e; color: white; }");
}

void HubWindow::showProjectsPage() {
    m_stack->setCurrentIndex(0);
    m_projectsBtn->setChecked(true);
    m_enginesBtn->setChecked(false);
}

void HubWindow::showEngineVersionsPage() {
    m_stack->setCurrentIndex(1);
    m_projectsBtn->setChecked(false);
    m_enginesBtn->setChecked(true);
}

void HubWindow::refreshProjectList() {
    m_projectList->clear();
    for (const auto& p : m_manager->projects()) {
        QString text = p.name + "\n" + p.path;
        if (p.lastOpened.isValid()) {
            text += "  |  Last opened: " + p.lastOpened.toString("yyyy-MM-dd hh:mm");
        }
        text += "  |  Engine: " + p.engineVersion;
        m_projectList->addItem(text);
    }
}

void HubWindow::refreshEngineVersionList() {
    m_engineList->clear();
    for (const auto& v : m_manager->engineVersions()) {
        m_engineList->addItem(v.label + "\n" + v.editorPath);
    }
}

void HubWindow::onNewProject() {
    QString name = QInputDialog::getText(this, "New Project", "Project name:");
    if (name.isEmpty()) return;

    QString path = QFileDialog::getExistingDirectory(this, "Select project location");
    if (path.isEmpty()) return;

    QString fullPath = path + "/" + name;

    // Pick engine version
    QStringList versionLabels;
    for (const auto& v : m_manager->engineVersions()) {
        versionLabels << v.label;
    }
    if (versionLabels.isEmpty()) {
        QMessageBox::warning(this, "No Engine", "Add an engine version first.");
        return;
    }

    QString version = versionLabels.first();
    if (versionLabels.size() > 1) {
        bool ok;
        version = QInputDialog::getItem(this, "Engine Version", "Select engine version:", versionLabels, 0, false, &ok);
        if (!ok) return;
    }

    if (!m_manager->createProjectDirectory(fullPath)) {
        QMessageBox::critical(this, "Error", "Failed to create project directory.");
        return;
    }

    m_manager->addProject(name, fullPath, version);
    refreshProjectList();
}

void HubWindow::onAddExisting() {
    QString path = QFileDialog::getExistingDirectory(this, "Select existing project folder");
    if (path.isEmpty()) return;

    m_manager->addExistingProject(path);
    refreshProjectList();
}

void HubWindow::onRemoveProject() {
    int row = m_projectList->currentRow();
    if (row < 0) return;

    auto result = QMessageBox::question(this, "Remove Project",
        "Remove this project from the list?\n(Files will not be deleted)");
    if (result == QMessageBox::Yes) {
        m_manager->removeProject(row);
        refreshProjectList();
    }
}

void HubWindow::onOpenProject() {
    int row = m_projectList->currentRow();
    if (row < 0) return;
    m_manager->openProject(row);
    refreshProjectList();
}

void HubWindow::onProjectDoubleClicked(QListWidgetItem*) {
    onOpenProject();
}

void HubWindow::onAddEngineVersion() {
    QString label = QInputDialog::getText(this, "Add Engine Version", "Version label (e.g. \"1.0\", \"dev\"):");
    if (label.isEmpty()) return;

    QString path = QFileDialog::getOpenFileName(this, "Select DabozzEditor.exe", "", "Executable (*.exe)");
    if (path.isEmpty()) return;

    m_manager->addEngineVersion(label, path);
    refreshEngineVersionList();
}

void HubWindow::onRemoveEngineVersion() {
    int row = m_engineList->currentRow();
    if (row < 0) return;

    m_manager->removeEngineVersion(row);
    refreshEngineVersionList();
}
