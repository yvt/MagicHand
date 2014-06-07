#ifndef MHPLATFORM_H
#define MHPLATFORM_H

#include <QSizeF>
#include <QPointF>

namespace MHPlatform
{
    enum class MouseButton
    {
        Left,
        Right,
        Center
    };

    QSizeF screenSize();
    void mouseMotion(QPointF);
    void mouseButtonDown(MouseButton);
    void mouseButtonUp(MouseButton);
}

#endif // MHPLATFORM_H
