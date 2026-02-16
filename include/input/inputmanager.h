#pragma once

#include <QSet>
#include <QPoint>
#include <Qt>

namespace DabozzEngine {
namespace Input {

class InputManager {
public:
    static InputManager& getInstance();

    void update();
    void reset();

    void keyPressed(int key);
    void keyReleased(int key);
    
    void mousePressed(Qt::MouseButton button);
    void mouseReleased(Qt::MouseButton button);
    void mouseMoved(const QPoint& pos);
    void mouseScrolled(int delta);

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isKeyReleased(int key) const;

    bool isMouseButtonDown(Qt::MouseButton button) const;
    bool isMouseButtonPressed(Qt::MouseButton button) const;
    bool isMouseButtonReleased(Qt::MouseButton button) const;

    QPoint getMousePosition() const { return m_mousePosition; }
    QPoint getMouseDelta() const { return m_mouseDelta; }
    int getMouseScrollDelta() const { return m_mouseScrollDelta; }

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    QSet<int> m_keysDown;
    QSet<int> m_keysPressed;
    QSet<int> m_keysReleased;

    QSet<Qt::MouseButton> m_mouseButtonsDown;
    QSet<Qt::MouseButton> m_mouseButtonsPressed;
    QSet<Qt::MouseButton> m_mouseButtonsReleased;

    QPoint m_mousePosition;
    QPoint m_lastMousePosition;
    QPoint m_mouseDelta;
    int m_mouseScrollDelta = 0;
};

}
}
