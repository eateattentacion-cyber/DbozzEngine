/**************************************************************************/
/*  projectmanager.h                                                      */
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

#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QObject>

struct ProjectInfo {
    QString name;
    QString path;
    QString engineVersion;
    QDateTime lastOpened;
};

struct EngineVersion {
    QString label;
    QString editorPath;
};

class ProjectManager : public QObject {
    Q_OBJECT
public:
    explicit ProjectManager(QObject* parent = nullptr);

    void load();
    void save();

    const QVector<ProjectInfo>& projects() const { return m_projects; }
    const QVector<EngineVersion>& engineVersions() const { return m_engineVersions; }

    void addProject(const QString& name, const QString& path, const QString& engineVersion);
    void addExistingProject(const QString& path);
    void removeProject(int index);
    void openProject(int index);

    void addEngineVersion(const QString& label, const QString& editorPath);
    void removeEngineVersion(int index);

    bool createProjectDirectory(const QString& path);

signals:
    void projectLaunched(const QString& projectPath);

private:
    QString configPath() const;
    QVector<ProjectInfo> m_projects;
    QVector<EngineVersion> m_engineVersions;
};
