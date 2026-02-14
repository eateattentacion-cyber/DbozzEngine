/////////////////////////////////////////////////////////////////////////////
// projectmanager.h                                                        //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QDateTime>

class ProjectManager : public QMainWindow {
    Q_OBJECT

public:
    ProjectManager(QWidget* parent = nullptr);
    ~ProjectManager();

    struct ProjectInfo {
        QString name;
        QString path;
        QDateTime lastOpened;
        bool favorite;
        QStringList tags;
    };

private slots:
    void onNewProject();
    void onOpenProject();
    void onImportProject();
    void onProjectDoubleClicked(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onRemoveProject();
    void onToggleFavorite();

private:
    void loadProjects();
    void saveProjects();
    void refreshProjectList();
    void addProjectToList(const ProjectInfo& info);
    QString getProjectKey(const QString& path) const;

    QListWidget* m_projectList;
    QLineEdit* m_searchBox;
    QPushButton* m_newButton;
    QPushButton* m_openButton;
    QPushButton* m_importButton;
    QPushButton* m_removeButton;
    QLabel* m_titleLabel;

    QSettings* m_settings;
    QList<ProjectInfo> m_projects;
};
