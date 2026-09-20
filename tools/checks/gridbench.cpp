// Offscreen harness: renders the real ViewportWidget's background at several
// zoom levels and times it, then re-renders with the old nested-drawEllipse
// grid for a like-for-like comparison on this machine.
#include "ui/ViewportWidget.hpp"
#include "scene/SceneDocument.hpp"
#include "core/GridSnap.hpp"

#include <QApplication>
#include <QImage>
#include <QPaintDevice>
#include <QPainter>
#include <QElapsedTimer>
#include <QTransform>
#include <cmath>
#include <cstdio>

// Verbatim copy of the grid loop this change replaced, for timing comparison.
static void OldGrid(QPainter* painter, const QRectF& rect)
{
    const double step = 16.0;
    const double radius = 1.2;
    const double left = std::floor(rect.left() / step) * step;
    const double top  = std::floor(rect.top()  / step) * step;
    const double cols = (rect.right() - left) / step + 1.0;
    const double rows = (rect.bottom() - top) / step + 1.0;

    if (cols * rows > 50000.0)
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(110, 110, 115));
    for (double x = left; x <= rect.right(); x += step)
        for (double y = top; y <= rect.bottom(); y += step)
            painter->drawEllipse(QPointF(x, y), radius, radius);
    painter->restore();
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    SceneDocument doc;
    ViewportWidget view;
    view.SetDocument(&doc);
    view.resize(1280, 800);

    const double zooms[] = { 1.0, 0.625, 0.26, 0.15, 3.0 };

    for (double z : zooms)
    {
        view.setTransform(QTransform::fromScale(z, z));
        view.centerOn(doc.GetCanvasRect().center());

        QImage img(1280, 800, QImage::Format_ARGB32_Premultiplied);

        // --- new (tiled) path, through the real widget ---
        img.fill(Qt::transparent);
        QElapsedTimer t; t.start();
        int n = 0;
        while (t.elapsed() < 400) { QPainter rp(&img); view.viewport()->render(&rp, QPoint(), QRegion(), QWidget::DrawWindowBackground); ++n; }
        const double newMs = double(t.nsecsElapsed()) / 1e6 / n;

        if (std::abs(z - 0.625) < 1e-9 || std::abs(z - 3.0) < 1e-9)
            { QPainter sp(&img); view.viewport()->render(&sp, QPoint(), QRegion(), QWidget::DrawWindowBackground); sp.end(); img.save(QStringLiteral("grid_v_%1.png").arg(z)); }

        // --- old (per-ellipse) path, same exposed area ---
        const QRectF exposed = view.mapToScene(view.viewport()->rect()).boundingRect();
        QImage img2(1280, 800, QImage::Format_ARGB32_Premultiplied);
        img2.fill(QColor(35, 35, 38));
        QElapsedTimer t2; t2.start();
        int n2 = 0;
        while (t2.elapsed() < 400)
        {
            QPainter p(&img2);
            p.setTransform(QTransform::fromScale(z, z));
            OldGrid(&p, exposed);
            ++n2;
        }
        const double oldMs = double(t2.nsecsElapsed()) / 1e6 / n2;

        const double cols = exposed.width() / 16.0 + 1.0;
        const double rows = exposed.height() / 16.0 + 1.0;

        std::printf("zoom %5.3f | dots %8.0f | OLD grid %8.4f ms | NEW full widget render %8.4f ms\n",
                    z, cols * rows, oldMs, newMs);
    }

    return 0;
}
