/**************************************************************************/
/*  hubwindow.h                                                           */
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

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include "projectmanager.h"

class HubWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit HubWindow(QWidget* parent = nullptr);

private slots:
    void onNewProject();
    void onAddExisting();
    void onRemoveProject();
    void onOpenProject();
    void onProjectDoubleClicked(QListWidgetItem* item);

    void onAddEngineVersion();
    void onRemoveEngineVersion();

    void showProjectsPage();
    void showEngineVersionsPage();

private:
    void refreshProjectList();
    void refreshEngineVersionList();
    void setupUI();

    ProjectManager* m_manager;
    QListWidget* m_projectList;
    QListWidget* m_engineList;
    QStackedWidget* m_stack;
    QPushButton* m_projectsBtn;
    QPushButton* m_enginesBtn;
};
