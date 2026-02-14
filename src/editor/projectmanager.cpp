/////////////////////////////////////////////////////////////////////////////
// projectmanager.cpp                                                      //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#include "editor/projectmanager.h"
#include "editor/projectsettingsdialog.h"
#include "editor/mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPalette>

ProjectManager::ProjectManager(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("DabozzEngine - Project Manager");
    resize(900, 600);

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

    m_settings = new QSettings("DabozzStudios", "DabozzEngine", this);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    m_titleLabel = new QLabel("DabozzEngine Project Manager", this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search projects...");
    connect(m_searchBox, &QLineEdit::textChanged, this, &ProjectManager::onSearchChanged);
    mainLayout->addWidget(m_searchBox);

    m_projectList = new QListWidget(this);
    m_projectList->setAlternatingRowColors(true);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectManager::onProjectDoubleClicked);
    mainLayout->addWidget(m_projectList);

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_newButton = new QPushButton("New Project", this);
    connect(m_newButton, &QPushButton::clicked, this, &ProjectManager::onNewProject);
    buttonLayout->addWidget(m_newButton);

    m_importButton = new QPushButton("Import Project", this);
    connect(m_importButton, &QPushButton::clicked, this, &ProjectManager::onImportProject);
    buttonLayout->addWidget(m_importButton);

    m_openButton = new QPushButton("Open Selected", this);
    connect(m_openButton, &QPushButton::clicked, this, &ProjectManager::onOpenProject);
    buttonLayout->addWidget(m_openButton);

    m_removeButton = new QPushButton("Remove from List", this);
    connect(m_removeButton, &QPushButton::clicked, this, &ProjectManager::onRemoveProject);
    buttonLayout->addWidget(m_removeButton);

    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    loadProjects();
    refreshProjectList();
}

ProjectManager::~ProjectManager()
{
    saveProjects();
}

void ProjectManager::loadProjects()
{
    m_projects.clear();

    int size = m_settings->beginReadArray("projects");
    for (int i = 0; i < size; ++i) {
        m_settings->setArrayIndex(i);
        ProjectInfo info;
        info.name = m_settings->value("name").toString();
        info.path = m_settings->value("path").toString();
        info.lastOpened = m_settings->value("lastOpened").toDateTime();
        info.favorite = m_settings->value("favorite", false).toBool();
        info.tags = m_settings->value("tags").toStringList();

        if (QDir(info.path).exists()) {
            m_projects.append(info);
        }
    }
    m_settings->endArray();

    std::sort(m_projects.begin(), m_projects.end(), [](const ProjectInfo& a, const ProjectInfo& b) {
        if (a.favorite != b.favorite) return a.favorite;
        return a.lastOpened > b.lastOpened;
    });
}

void ProjectManager::saveProjects()
{
    m_settings->beginWriteArray("projects");
    for (int i = 0; i < m_projects.size(); ++i) {
        m_settings->setArrayIndex(i);
        m_settings->setValue("name", m_projects[i].name);
        m_settings->setValue("path", m_projects[i].path);
        m_settings->setValue("lastOpened", m_projects[i].lastOpened);
        m_settings->setValue("favorite", m_projects[i].favorite);
        m_settings->setValue("tags", m_projects[i].tags);
    }
    m_settings->endArray();
    m_settings->sync();
}

void ProjectManager::refreshProjectList()
{
    m_projectList->clear();

    QString searchTerm = m_searchBox->text().toLower();

    for (const ProjectInfo& info : m_projects) {
        if (!searchTerm.isEmpty() && !info.name.toLower().contains(searchTerm)) {
            continue;
        }

        addProjectToList(info);
    }
}

void ProjectManager::addProjectToList(const ProjectInfo& info)
{
    QString displayText = info.name;
    if (info.favorite) {
        displayText = "★ " + displayText;
    }
    displayText += "\n" + info.path;
    displayText += "\nLast opened: " + info.lastOpened.toString("yyyy-MM-dd hh:mm");

    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, info.path);
    m_projectList->addItem(item);
}

void ProjectManager::onNewProject()
{
    ProjectSettingsDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString projectName = dialog.getProjectName();
    QString projectPath = dialog.getProjectPath();
    QString scriptLang = dialog.getScriptingLanguage();
    bool createScene = dialog.shouldCreateDefaultScene();

    QDir dir(projectPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    dir.mkpath("Assets");
    dir.mkpath("Scenes");
    dir.mkpath("Scripts");

    QString projectFile = projectPath + "/project.dbz";
    QFile file(projectFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject projectData;
        projectData["name"] = projectName;
        projectData["version"] = "1.0";
        projectData["engine_version"] = "1.0.0";
        projectData["scripting_language"] = scriptLang;

        QJsonDocument doc(projectData);
        file.write(doc.toJson());
        file.close();
    }

    if (createScene) {
        QString sceneFile = projectPath + "/Scenes/main.scene";
        QFile scene(sceneFile);
        if (scene.open(QIODevice::WriteOnly)) {
            QJsonObject sceneData;
            sceneData["name"] = "Main Scene";
            sceneData["entities"] = QJsonArray();
            QJsonDocument doc(sceneData);
            scene.write(doc.toJson());
            scene.close();
        }
    }

    ProjectInfo info;
    info.name = projectName;
    info.path = projectPath;
    info.lastOpened = QDateTime::currentDateTime();
    info.favorite = false;

    m_projects.prepend(info);
    saveProjects();
    refreshProjectList();

    MainWindow* editor = new MainWindow(projectPath);
    editor->showMaximized();
    close();
}

void ProjectManager::onOpenProject()
{
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    onProjectDoubleClicked(item);
}

void ProjectManager::onImportProject()
{
    QString projectPath = QFileDialog::getExistingDirectory(this, "Select Project Folder");
    if (projectPath.isEmpty()) return;

    QString projectFile = projectPath + "/project.dbz";
    if (!QFile::exists(projectFile)) {
        QMessageBox::warning(this, "Invalid Project", "No project.dbz file found in selected folder.");
        return;
    }

    QString projectName = QFileInfo(projectPath).fileName();

    ProjectInfo info;
    info.name = projectName;
    info.path = projectPath;
    info.lastOpened = QDateTime::currentDateTime();
    info.favorite = false;

    m_projects.prepend(info);
    saveProjects();
    refreshProjectList();
}

void ProjectManager::onProjectDoubleClicked(QListWidgetItem* item)
{
    QString projectPath = item->data(Qt::UserRole).toString();

    for (ProjectInfo& info : m_projects) {
        if (info.path == projectPath) {
            info.lastOpened = QDateTime::currentDateTime();
            break;
        }
    }
    saveProjects();

    MainWindow* editor = new MainWindow(projectPath);
    editor->showMaximized();
    close();
}

void ProjectManager::onSearchChanged(const QString& text)
{
    refreshProjectList();
}

void ProjectManager::onRemoveProject()
{
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    QString projectPath = item->data(Qt::UserRole).toString();

    m_projects.erase(std::remove_if(m_projects.begin(), m_projects.end(),
        [&projectPath](const ProjectInfo& info) { return info.path == projectPath; }),
        m_projects.end());

    saveProjects();
    refreshProjectList();
}

void ProjectManager::onToggleFavorite()
{
    QListWidgetItem* item = m_projectList->currentItem();
    if (!item) return;

    QString projectPath = item->data(Qt::UserRole).toString();

    for (ProjectInfo& info : m_projects) {
        if (info.path == projectPath) {
            info.favorite = !info.favorite;
            break;
        }
    }

    saveProjects();
    refreshProjectList();
}

QString ProjectManager::getProjectKey(const QString& path) const
{
    return path;
}
