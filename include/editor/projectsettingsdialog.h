/////////////////////////////////////////////////////////////////////////////
// projectsettingsdialog.h                                                 //
/////////////////////////////////////////////////////////////////////////////
//                         This file is part of:                           //
//                           DABOZZ ENGINE                                 //
/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2026-present DabozzEngine contributors.                   //
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>

class ProjectSettingsDialog : public QDialog {
    Q_OBJECT

public:
    ProjectSettingsDialog(QWidget* parent = nullptr);

    QString getProjectName() const;
    QString getProjectPath() const;
    QString getScriptingLanguage() const;
    bool shouldCreateDefaultScene() const;

private slots:
    void onBrowse();
    void onProjectNameChanged(const QString& text);

private:
    void updateProjectPath();

    QLineEdit* m_projectNameEdit;
    QLineEdit* m_projectPathEdit;
    QComboBox* m_languageCombo;
    QCheckBox* m_createSceneCheck;
    QPushButton* m_browseButton;
    QPushButton* m_createButton;
    QPushButton* m_cancelButton;

    QString m_baseProjectPath;
};
