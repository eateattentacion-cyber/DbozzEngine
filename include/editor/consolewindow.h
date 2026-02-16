#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <vector>
#include <QString>

class ConsoleWindow : public QWidget {
    Q_OBJECT

public:
    enum class LogLevel {
        Info,
        Warning,
        Error,
        Script
    };

    explicit ConsoleWindow(QWidget* parent = nullptr);

    void log(const QString& message, LogLevel level = LogLevel::Info);
    void clear();

signals:
    void commandEntered(const QString& command);

private slots:
    void onCommandSubmit();
    void onClearClicked();
    void onFilterChanged(int index);

private:
    void setupUI();
    void applyDarkTheme();
    QString getColorForLevel(LogLevel level) const;
    QString getLevelPrefix(LogLevel level) const;

    QTextEdit* m_output;
    QLineEdit* m_input;
    QPushButton* m_clearButton;
    QComboBox* m_filterCombo;
    
    std::vector<std::pair<QString, LogLevel>> m_logHistory;
    LogLevel m_currentFilter;
};
