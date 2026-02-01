#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QProcess>
#include <QRegularExpression>

class CSharpHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit CSharpHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat functionFormat;
};

class ScriptEditor : public QWidget {
    Q_OBJECT
public:
    explicit ScriptEditor(QWidget* parent = nullptr);
    ~ScriptEditor();

    void newScript();
    void openScript(const QString& filepath);
    void saveScript();
    void saveScriptAs();
    bool compileScript();

signals:
    void compilationFinished(bool success, const QString& output);

private slots:
    void onTextChanged();
    void onCompileFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupToolbar();
    void setCurrentFile(const QString& filepath);
    QString getScriptTemplate();

    QPlainTextEdit* m_editor;
    CSharpHighlighter* m_highlighter;
    QTextEdit* m_outputLog;
    QProcess* m_compileProcess;
    
    QString m_currentFile;
    bool m_isModified;
};
