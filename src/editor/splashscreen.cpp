#include "editor/splashscreen.h"
#include <QPainter>
#include <QFont>
#include <QMouseEvent>

SplashScreen::SplashScreen(const QPixmap& pixmap)
    : QSplashScreen(pixmap)
    , m_timer(new QTimer(this))
    , m_fadeIn(new QPropertyAnimation(this, "opacity"))
    , m_fadeOut(new QPropertyAnimation(this, "opacity"))
{
    setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowOpacity(0.0);
    
    m_fadeIn->setDuration(500);
    m_fadeIn->setStartValue(0.0);
    m_fadeIn->setEndValue(1.0);
    
    m_fadeOut->setDuration(500);
    m_fadeOut->setStartValue(1.0);
    m_fadeOut->setEndValue(0.0);
    
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_fadeOut->start();
    });
    
    connect(m_fadeOut, &QPropertyAnimation::finished, this, [this]() {
        close();
        emit finished();
    });
}

void SplashScreen::showWithDelay(int milliseconds)
{
    show();
    m_fadeIn->start();
    m_timer->start(milliseconds);
}

void SplashScreen::mousePressEvent(QMouseEvent* event)
{
    if (m_timer->isActive()) {
        m_timer->stop();
        m_fadeOut->start();
    }
    QSplashScreen::mousePressEvent(event);
}
