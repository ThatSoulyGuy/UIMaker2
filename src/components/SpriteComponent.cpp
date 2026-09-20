#include "components/SpriteComponent.hpp"
#include "core/PixelDraw.hpp"
#include "core/SpriteSidecar.hpp"
#include "core/PixelModel.hpp"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <algorithm>

#include "core/AssetContext.hpp"

REGISTER_COMPONENT(SpriteComponent, "Sprite")

SpriteComponent::SpriteComponent(QObject* parent)
    : Component(parent)
    , m_frameWidth(64)
    , m_frameHeight(64)
    , m_frameCount(1)
    , m_currentFrame(0)
    , m_columns(1)
{ }

QString SpriteComponent::GetTypeName() const { return QStringLiteral("Sprite"); }

void SpriteComponent::Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect)
{
    Q_UNUSED(item);
    Q_UNUSED(parentRect);

    rect = QRectF(0, 0, m_frameWidth, m_frameHeight);
}

bool SpriteComponent::Paint(QPainter* painter, const QRectF& rect, bool selected)
{
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !PixelModel::PixelSnap());

    if (!m_pixmap.isNull() && m_columns > 0)
    {
        int frame = std::clamp(m_currentFrame, 0, std::max(0, m_frameCount - 1));
        int col = frame % m_columns;
        int row = frame / m_columns;

        QRectF srcRect(col * m_frameWidth, row * m_frameHeight, m_frameWidth, m_frameHeight);
        // A sprite sheet already selects a sub-frame, so the frame IS the
        // texture as far as wrapping is concerned.
        PixelDraw::DrawTexture(painter, rect, m_pixmap.copy(srcRect.toRect()),
                               SpriteSidecar::MetaFor(m_imagePath).slice,
                               tex.anchor, tex.cropOffsetX, tex.cropOffsetY,
                               static_cast<PixelDraw::Fill>(tex.fill));
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

QString SpriteComponent::GetImagePath() const noexcept { return m_imagePath; }
void SpriteComponent::SetImagePath(const QString& v)
{
    if (m_imagePath == v) return;
    m_imagePath = v;
    m_pixmap = QPixmap();
    if (!m_imagePath.isEmpty())
    {
        QPixmap loaded(AssetContext::Resolve(m_imagePath));
        if (!loaded.isNull())
            m_pixmap = loaded;
    }
    NotifyChanged();
}

int SpriteComponent::GetFrameWidth() const noexcept { return m_frameWidth; }
void SpriteComponent::SetFrameWidth(int v) { v = std::max(1, v); if (m_frameWidth == v) return; m_frameWidth = v; NotifyChanged(); }

int SpriteComponent::GetFrameHeight() const noexcept { return m_frameHeight; }
void SpriteComponent::SetFrameHeight(int v) { v = std::max(1, v); if (m_frameHeight == v) return; m_frameHeight = v; NotifyChanged(); }

int SpriteComponent::GetFrameCount() const noexcept { return m_frameCount; }
void SpriteComponent::SetFrameCount(int v) { v = std::max(1, v); if (m_frameCount == v) return; m_frameCount = v; NotifyChanged(); }

int SpriteComponent::GetCurrentFrame() const noexcept { return m_currentFrame; }
void SpriteComponent::SetCurrentFrame(int v) { v = std::max(0, v); if (m_currentFrame == v) return; m_currentFrame = v; NotifyChanged(); }

int SpriteComponent::GetColumns() const noexcept { return m_columns; }
void SpriteComponent::SetColumns(int v) { v = std::max(1, v); if (m_columns == v) return; m_columns = v; NotifyChanged(); }

QString SpriteComponent::GetAssetDomain() const noexcept { return m_assetDomain; }
void SpriteComponent::SetAssetDomain(const QString& v) { if (m_assetDomain == v) return; m_assetDomain = v; NotifyChanged(); }

QString SpriteComponent::GetAssetRegistryValue() const noexcept { return m_assetRegistryValue; }
void SpriteComponent::SetAssetRegistryValue(const QString& v) { if (m_assetRegistryValue == v) return; m_assetRegistryValue = v; NotifyChanged(); }

void SpriteComponent::ToJson(QJsonObject& out) const
{
    out["kind"] = "Sprite";
    out["imagePath"] = m_imagePath;
    out["frameWidth"] = m_frameWidth;
    out["frameHeight"] = m_frameHeight;
    out["frameCount"] = m_frameCount;
    out["currentFrame"] = m_currentFrame;
    out["columns"] = m_columns;
    out["assetDomain"] = m_assetDomain;
    out["assetRegistryValue"] = m_assetRegistryValue;
    out["textureFill"] = tex.fill;
    out["cropAnchor"] = tex.anchor;
    out["cropOffsetX"] = tex.cropOffsetX;
    out["cropOffsetY"] = tex.cropOffsetY;
}

void SpriteComponent::FromJson(const QJsonObject& in)
{
    SetImagePath(in["imagePath"].toString());
    SetFrameWidth(in["frameWidth"].toInt(64));
    SetFrameHeight(in["frameHeight"].toInt(64));
    SetFrameCount(in["frameCount"].toInt(1));
    SetCurrentFrame(in["currentFrame"].toInt(0));
    SetColumns(in["columns"].toInt(1));
    SetAssetDomain(in["assetDomain"].toString());
    SetAssetRegistryValue(in["assetRegistryValue"].toString());
    SetTextureFill(in["textureFill"].toInt(PixelDraw::FillStretch));
    SetCropAnchor(in["cropAnchor"].toInt(PixelDraw::Center));
    SetCropOffsetX(in["cropOffsetX"].toInt(0));
    SetCropOffsetY(in["cropOffsetY"].toInt(0));
}

int SpriteComponent::GetTextureFill() const noexcept { return tex.fill; }

void SpriteComponent::SetTextureFill(int v)
{
    const int c = PixelDraw::ClampFill(v);
    if (tex.fill == c) return;
    tex.fill = c;
    NotifyChanged();
}

int SpriteComponent::GetCropAnchor() const noexcept { return tex.anchor; }

void SpriteComponent::SetCropAnchor(int v)
{
    const int c = PixelDraw::ClampAnchor(v);
    if (tex.anchor == c) return;
    tex.anchor = c;
    NotifyChanged();
}

int SpriteComponent::GetCropOffsetX() const noexcept { return tex.cropOffsetX; }

void SpriteComponent::SetCropOffsetX(int v)
{
    if (tex.cropOffsetX == v) return;
    tex.cropOffsetX = v;
    NotifyChanged();
}

int SpriteComponent::GetCropOffsetY() const noexcept { return tex.cropOffsetY; }

void SpriteComponent::SetCropOffsetY(int v)
{
    if (tex.cropOffsetY == v) return;
    tex.cropOffsetY = v;
    NotifyChanged();
}
