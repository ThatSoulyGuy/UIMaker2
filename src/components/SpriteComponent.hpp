#ifndef COMPONENTS_SPRITECOMPONENT_HPP
#define COMPONENTS_SPRITECOMPONENT_HPP

#include <QColor>
#include <QString>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <algorithm>

#include "core/Component.hpp"

class SpriteComponent : public Component
{
    Q_OBJECT

    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int frameWidth READ GetFrameWidth WRITE SetFrameWidth NOTIFY ComponentChanged)
    Q_PROPERTY(int frameHeight READ GetFrameHeight WRITE SetFrameHeight NOTIFY ComponentChanged)
    Q_PROPERTY(int frameCount READ GetFrameCount WRITE SetFrameCount NOTIFY ComponentChanged)
    Q_PROPERTY(int currentFrame READ GetCurrentFrame WRITE SetCurrentFrame NOTIFY ComponentChanged)
    Q_PROPERTY(int columns READ GetColumns WRITE SetColumns NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)

public:

    explicit SpriteComponent(QObject* parent = nullptr)
        : Component(parent)
        , m_frameWidth(64)
        , m_frameHeight(64)
        , m_frameCount(1)
        , m_currentFrame(0)
        , m_columns(1)
    { }

    QString GetTypeName() const override { return QStringLiteral("Sprite"); }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        rect = QRectF(0, 0, m_frameWidth, m_frameHeight);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (!m_pixmap.isNull() && m_columns > 0)
        {
            int frame = std::clamp(m_currentFrame, 0, std::max(0, m_frameCount - 1));
            int col = frame % m_columns;
            int row = frame / m_columns;

            QRectF srcRect(col * m_frameWidth, row * m_frameHeight, m_frameWidth, m_frameHeight);
            painter->drawPixmap(rect, m_pixmap, srcRect);
        }
        else
        {
            // Draw placeholder sprite frame
            QPen pen(QColor(200, 180, 100), 1, Qt::DashLine);
            pen.setCosmetic(true);
            painter->setPen(pen);
            painter->setBrush(QColor(60, 55, 40, 100));
            painter->drawRect(rect);

            painter->setPen(QColor(200, 180, 100));
            QFont font;
            font.setPixelSize(12);
            painter->setFont(font);
            QString label = QString("F%1").arg(m_currentFrame);
            painter->drawText(rect, Qt::AlignCenter, label);
        }

        if (selected)
        {
            QPen selPen(QColor(0, 180, 255), 2, Qt::DashLine);
            selPen.setCosmetic(true);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(rect);
        }

        painter->restore();
        return true;
    }

    QString GetImagePath() const noexcept { return m_imagePath; }
    void SetImagePath(const QString& v)
    {
        if (m_imagePath == v) return;
        m_imagePath = v;
        m_pixmap = QPixmap();
        if (!m_imagePath.isEmpty())
        {
            QPixmap loaded(m_imagePath);
            if (!loaded.isNull())
                m_pixmap = loaded;
        }
        NotifyChanged();
    }

    int GetFrameWidth() const noexcept { return m_frameWidth; }
    void SetFrameWidth(int v) { v = std::max(1, v); if (m_frameWidth == v) return; m_frameWidth = v; NotifyChanged(); }

    int GetFrameHeight() const noexcept { return m_frameHeight; }
    void SetFrameHeight(int v) { v = std::max(1, v); if (m_frameHeight == v) return; m_frameHeight = v; NotifyChanged(); }

    int GetFrameCount() const noexcept { return m_frameCount; }
    void SetFrameCount(int v) { v = std::max(1, v); if (m_frameCount == v) return; m_frameCount = v; NotifyChanged(); }

    int GetCurrentFrame() const noexcept { return m_currentFrame; }
    void SetCurrentFrame(int v) { v = std::max(0, v); if (m_currentFrame == v) return; m_currentFrame = v; NotifyChanged(); }

    int GetColumns() const noexcept { return m_columns; }
    void SetColumns(int v) { v = std::max(1, v); if (m_columns == v) return; m_columns = v; NotifyChanged(); }

    QString GetAssetPath() const noexcept { return m_assetPath; }
    void SetAssetPath(const QString& v) { if (m_assetPath == v) return; m_assetPath = v; NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Sprite";
        out["imagePath"] = m_imagePath;
        out["frameWidth"] = m_frameWidth;
        out["frameHeight"] = m_frameHeight;
        out["frameCount"] = m_frameCount;
        out["currentFrame"] = m_currentFrame;
        out["columns"] = m_columns;
        out["assetPath"] = m_assetPath;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetImagePath(in["imagePath"].toString());
        SetFrameWidth(in["frameWidth"].toInt(64));
        SetFrameHeight(in["frameHeight"].toInt(64));
        SetFrameCount(in["frameCount"].toInt(1));
        SetCurrentFrame(in["currentFrame"].toInt(0));
        SetColumns(in["columns"].toInt(1));
        SetAssetPath(in["assetPath"].toString());
    }

private:

    QString m_imagePath;
    int m_frameWidth;
    int m_frameHeight;
    int m_frameCount;
    int m_currentFrame;
    int m_columns;
    QString m_assetPath;
    QPixmap m_pixmap;
};

#endif
