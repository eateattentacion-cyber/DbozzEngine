#pragma once

#include <QSplashScreen>
#include <QTimer>
#include <QPropertyAnimation>

class SplashScreen : public QSplashScreen {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ windowOpacity WRITE setWindowOpacity)

public:
    explicit SplashScreen(const QPixmap& pixmap);
    void showWithDelay(int milliseconds);

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void finished();

private:
    QTimer* m_timer;
    QPropertyAnimation* m_fadeIn;
    QPropertyAnimation* m_fadeOut;
};
