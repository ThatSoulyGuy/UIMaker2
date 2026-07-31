#ifndef GIZMO_HPP
#define GIZMO_HPP

#include <QObject>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QPainter>
#include <QHash>
#include <QList>
#include <Qt>
#include <functional>

struct GizmoHandle
{
    QString id;
    Qt::CursorShape cursor = Qt::ArrowCursor;
    int priority = 0;

    GizmoHandle() = default;

    GizmoHandle(const QString& handleId, Qt::CursorShape cursorShape, int handlePriority = 0);
};

struct GizmoContext
{
    QPainter* painter = nullptr;
    QRectF itemSceneBounds;
    QRectF itemViewBounds;
    QPointF itemCenter;
    double rotation = 0.0;
    QPointF scale = QPointF(1.0, 1.0);
    double zoom = 1.0;
    bool isActive = false;
    QString activeHandleId;
};

struct GizmoHitResult
{
    QString handleId;
    Qt::CursorShape cursor = Qt::ArrowCursor;
    QString tooltip;

    bool IsHit() const noexcept;
};

class Gizmo : public QObject
{
    Q_OBJECT

public:

    explicit Gizmo(QObject* parent = nullptr);

    virtual ~Gizmo() = default;

    virtual QString GetId() const = 0;
    virtual QString GetDisplayName() const = 0;

    virtual void Draw(const GizmoContext& ctx) = 0;

    virtual GizmoHitResult HitTest(const QPointF& viewPos, const GizmoContext& ctx) = 0;

    virtual QList<GizmoHandle> GetHandles() const = 0;

    virtual Qt::CursorShape GetCursor(const QString& handleId) const;

    using GizmoFactory = std::function<Gizmo*(QObject*)>;

    static QHash<QString, GizmoFactory>& Registry();

    static void Register(const QString& id, GizmoFactory factory);

    static Gizmo* Create(const QString& id, QObject* parent);

    static QStringList GetRegisteredIds();

protected:

    static constexpr int kHandleSize = 10;
    static constexpr int kArrowLength = 80;
    static constexpr int kArrowHeadSize = 12;
    static constexpr int kRotateRadius = 60;
    static constexpr int kHitTolerance = 12;

};

#define REGISTER_GIZMO(ClassName, GizmoId) \
    static const bool ClassName##_gizmo_registered = [](){ \
        Gizmo::Register(GizmoId, [](QObject* p){ return new ClassName(p); }); \
        return true; \
    }();

#endif
