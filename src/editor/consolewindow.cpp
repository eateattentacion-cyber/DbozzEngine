#include "editor/consolewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

ConsoleWindow::ConsoleWindow(QWidget* parent)
    : QWidget(parent)
    , m_currentFilter(LogLevel::Info)
{
    setupUI();
    applyDarkTheme();
    
    log("Console initialized", LogLevel::Info);
}

void ConsoleWindow::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setContentsMargins(8, 8, 8, 8);
    topBar->setSpacing(8);
    
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("All");
    m_filterCombo->addItem("Info");
    m_filterCombo->addItem("Warning");
    m_filterCombo->addItem("Error");
    m_filterCombo->addItem("Script");
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConsoleWindow::onFilterChanged);
    
    m_clearButton = new QPushButton("Clear", this);
    connect(m_clearButton, &QPushButton::clicked, this, &ConsoleWindow::onClearClicked);
    
    topBar->addWidget(m_filterCombo);
    topBar->addStretch();
    topBar->addWidget(m_clearButton);
    
    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QTextEdit::WidgetWidth);
    
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);
    
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Enter command...");
    connect(m_input, &QLineEdit::returnPressed, this, &ConsoleWindow::onCommandSubmit);
    
    QPushButton* submitBtn = new QPushButton("Run", this);
    connect(submitBtn, &QPushButton::clicked, this, &ConsoleWindow::onCommandSubmit);
    
    inputLayout->addWidget(m_input);
    inputLayout->addWidget(submitBtn);
    
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(m_output);
    mainLayout->addLayout(inputLayout);
}

void ConsoleWindow::applyDarkTheme()
{
    setStyleSheet(R"(
        QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QTextEdit {
            background-color: #0a0a0f;
            border: 1px solid #333;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
        }
        QLineEdit {
            background-color: #2a2a2a;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 6px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #2563eb;
        }
        QPushButton {
            background-color: #333;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 6px 14px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
        }
        QPushButton:pressed {
            background-color: #2563eb;
        }
        QComboBox {
            background-color: #2a2a2a;
            border: 1px solid #3a3a3a;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 12px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #252525;
            border: 1px solid #333;
            selection-background-color: #2563eb;
        }
    )");
}

void ConsoleWindow::log(const QString& message, LogLevel level)
{
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    QString prefix = getLevelPrefix(level);
    QString color = getColorForLevel(level);
    
    QString formattedMsg = QString("<span style='color: #888;'>%1</span> "
                                   "<span style='color: %2;'>%3</span> %4")
                                   .arg(timestamp)
                                   .arg(color)
                                   .arg(prefix)
                                   .arg(message);
    
    m_logHistory.push_back({formattedMsg, level});
    
    if (m_currentFilter == LogLevel::Info || 
        static_cast<int>(m_currentFilter) - 1 == static_cast<int>(level)) {
        m_output->append(formattedMsg);
    }
}

void ConsoleWindow::clear()
{
    m_output->clear();
    m_logHistory.clear();
}

void ConsoleWindow::onCommandSubmit()
{
    QString command = m_input->text().trimmed();
    if (command.isEmpty()) return;
    
    log("> " + command, LogLevel::Script);
    emit commandEntered(command);
    
    m_input->clear();
}

void ConsoleWindow::onClearClicked()
{
    clear();
}

void ConsoleWindow::onFilterChanged(int index)
{
    m_output->clear();
    
    if (index == 0) {
        m_currentFilter = LogLevel::Info;
        for (const auto& [msg, level] : m_logHistory) {
            m_output->append(msg);
        }
    } else {
        m_currentFilter = static_cast<LogLevel>(index - 1);
        for (const auto& [msg, level] : m_logHistory) {
            if (level == m_currentFilter) {
                m_output->append(msg);
            }
        }
    }
}

QString ConsoleWindow::getColorForLevel(LogLevel level) const
{
    switch (level) {
        case LogLevel::Info: return "#00d4ff";
        case LogLevel::Warning: return "#ffaa00";
        case LogLevel::Error: return "#ff4444";
        case LogLevel::Script: return "#88ff88";
        default: return "#d4d4d4";
    }
}

QString ConsoleWindow::getLevelPrefix(LogLevel level) const
{
    switch (level) {
        case LogLevel::Info: return "[INFO]";
        case LogLevel::Warning: return "[WARN]";
        case LogLevel::Error: return "[ERROR]";
        case LogLevel::Script: return "[SCRIPT]";
        default: return "[LOG]";
    }
}
