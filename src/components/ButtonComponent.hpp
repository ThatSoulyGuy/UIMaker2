#ifndef COMPONENTS_BUTTONCOMPONENT_HPP
#define COMPONENTS_BUTTONCOMPONENT_HPP

#include <QString>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QLinearGradient>
#include <algorithm>

#include "core/Component.hpp"

class ButtonComponent : public Component
{
    Q_OBJECT
    Q_PROPERTY(QString text READ GetText WRITE SetText NOTIFY ComponentChanged)
    Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor NOTIFY ComponentChanged)
    Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontFamily READ GetFontFamily WRITE SetFontFamily NOTIFY ComponentChanged)
    Q_PROPERTY(int pixelSize READ GetPixelSize WRITE SetPixelSize NOTIFY ComponentChanged)
    Q_PROPERTY(QString fontPath READ GetFontPath WRITE SetFontPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString assetPath READ GetAssetPath WRITE SetAssetPath NOTIFY ComponentChanged)
    Q_PROPERTY(QString imagePath READ GetImagePath WRITE SetImagePath NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceLeft READ GetSliceLeft WRITE SetSliceLeft NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceTop READ GetSliceTop WRITE SetSliceTop NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceRight READ GetSliceRight WRITE SetSliceRight NOTIFY ComponentChanged)
    Q_PROPERTY(int sliceBottom READ GetSliceBottom WRITE SetSliceBottom NOTIFY ComponentChanged)

public:

    explicit ButtonComponent(QObject* parent = nullptr) : Component(parent), backgroundColor(QColor(40, 40, 40)), textColor(Qt::white), fontFamily("Inter"), pixelSize(24) { }

    QString GetTypeName() const override
    {
        return QStringLiteral("Button");
    }

    void Update(SceneElementItem& item, QRectF& rect, const QRectF& parentRect) override
    {
        Q_UNUSED(item);
        Q_UNUSED(parentRect);

        QFont font(fontFamily, pixelSize);
        QFontMetrics fm(font);

        const QSize size(fm.horizontalAdvance(text) + 40, fm.height() + 20);

        rect = QRectF(QPointF(0.0, 0.0), size);
    }

    bool Paint(QPainter* painter, const QRectF& rect, bool selected) override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QPixmap& skin = !customSkin.isNull() ? customSkin : EnsureDefaultSkin();

        DrawNineSlice(painter, rect, skin, sliceLeft, sliceTop, sliceRight, sliceBottom);

        painter->setPen(textColor);

        QFont font(fontFamily, pixelSize);

        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, text);

        if (selected)
        {
            painter->setPen(QPen(QColor(0, 180, 255), 2, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(rect, 6.0, 6.0);
        }

        painter->restore();

        return true;
    }

    QString GetText() const noexcept { return text; }
    void SetText(const QString& v) { if (text == v) return; text = v; NotifyChanged(); }

    QColor GetBackgroundColor() const noexcept { return backgroundColor; }
    void SetBackgroundColor(const QColor& v) { if (backgroundColor == v) return; backgroundColor = v; NotifyChanged(); }

    QColor GetTextColor() const noexcept { return textColor; }
    void SetTextColor(const QColor& v) { if (textColor == v) return; textColor = v; NotifyChanged(); }

    QString GetFontFamily() const noexcept { return fontFamily; }
    void SetFontFamily(const QString& v) { if (fontFamily == v) return; fontFamily = v; NotifyChanged(); }

    int GetPixelSize() const noexcept { return pixelSize; }
    void SetPixelSize(int v) { if (pixelSize == v) return; pixelSize = v; NotifyChanged(); }

    QString GetFontPath() const noexcept { return fontPath; }
    void SetFontPath(const QString& v)
    {
        if (fontPath == v) return;
        fontPath = v;
        if (!fontPath.isEmpty())
        {
            int id = QFontDatabase::addApplicationFont(fontPath);
            if (id != -1)
            {
                const QStringList fams = QFontDatabase::applicationFontFamilies(id);
                if (!fams.isEmpty())
                    fontFamily = fams.first();
            }
        }
        NotifyChanged();
    }

    QString GetAssetPath() const noexcept { return assetPath; }
    void SetAssetPath(const QString& v) { if (assetPath == v) return; assetPath = v; NotifyChanged(); }

    QString GetImagePath() const noexcept { return imagePath; }
    void SetImagePath(const QString& v)
    {
        if (imagePath == v) return;
        imagePath = v;
        customSkin = QPixmap();
        if (!imagePath.isEmpty())
        {
            QPixmap loaded(imagePath);
            if (!loaded.isNull())
                customSkin = loaded;
        }
        NotifyChanged();
    }

    int GetSliceLeft() const noexcept { return sliceLeft; }
    int GetSliceTop() const noexcept { return sliceTop; }
    int GetSliceRight() const noexcept { return sliceRight; }
    int GetSliceBottom() const noexcept { return sliceBottom; }

    void SetSliceLeft(int v) { v = std::max(0, v); if (sliceLeft == v) return; sliceLeft = v; InvalidateDefaultSkin(); NotifyChanged(); }
    void SetSliceTop(int v) { v = std::max(0, v); if (sliceTop == v) return; sliceTop = v; InvalidateDefaultSkin(); NotifyChanged(); }
    void SetSliceRight(int v) { v = std::max(0, v); if (sliceRight == v) return; sliceRight = v; InvalidateDefaultSkin(); NotifyChanged(); }
    void SetSliceBottom(int v) { v = std::max(0, v); if (sliceBottom == v) return; sliceBottom = v; InvalidateDefaultSkin(); NotifyChanged(); }

    void ToJson(QJsonObject& out) const override
    {
        out["kind"] = "Button";
        out["text"] = text;
        out["backgroundColor"] = backgroundColor.name(QColor::HexArgb);
        out["textColor"] = textColor.name(QColor::HexArgb);
        out["fontFamily"] = fontFamily;
        out["pixelSize"] = pixelSize;
        out["fontPath"] = fontPath;
        out["assetPath"] = assetPath;
        out["imagePath"] = imagePath;
        out["sliceLeft"] = sliceLeft;
        out["sliceTop"] = sliceTop;
        out["sliceRight"] = sliceRight;
        out["sliceBottom"] = sliceBottom;
    }

    void FromJson(const QJsonObject& in) override
    {
        SetText(in["text"].toString("Button"));
        SetBackgroundColor(QColor(in["backgroundColor"].toString("#FF282828")));
        SetTextColor(QColor(in["textColor"].toString("#FFFFFFFF")));
        SetFontFamily(in["fontFamily"].toString(fontFamily));
        SetPixelSize(in["pixelSize"].toInt(pixelSize));
        SetFontPath(in["fontPath"].toString());
        SetAssetPath(in["assetPath"].toString());
        SetImagePath(in["imagePath"].toString());
        SetSliceLeft(in["sliceLeft"].toInt(6));
        SetSliceTop(in["sliceTop"].toInt(6));
        SetSliceRight(in["sliceRight"].toInt(6));
        SetSliceBottom(in["sliceBottom"].toInt(6));
    }

private:

    mutable QPixmap defaultSkin;
    mutable QColor defaultSkinColor;

    void InvalidateDefaultSkin() { defaultSkin = QPixmap(); }

    const QPixmap& EnsureDefaultSkin() const
    {
        if (!defaultSkin.isNull() && defaultSkinColor == backgroundColor)
            return defaultSkin;

        const int baseW = std::max(48, sliceLeft + sliceRight + 24);
        const int baseH = std::max(32, sliceTop  + sliceBottom + 16);

        QPixmap pm(baseW, baseH);
        pm.fill(Qt::transparent);

        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        QLinearGradient g(0, 0, 0, baseH);
        QColor top = backgroundColor.lighter(115);
        QColor bot = backgroundColor.darker(105);
        g.setColorAt(0.0, top);
        g.setColorAt(1.0, bot);

        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(QRectF(0.5, 0.5, baseW - 1.0, baseH - 1.0), 6.0, 6.0);

        p.setPen(QPen(QColor(0, 0, 0, 110), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(0.5, 0.5, baseW - 1.0, baseH - 1.0), 6.0, 6.0);

        p.setPen(QPen(QColor(255, 255, 255, 35), 1.0));
        p.drawLine(QPointF(6.0, 1.0), QPointF(baseW - 6.0, 1.0));

        p.end();

        defaultSkin = pm;
        defaultSkinColor = backgroundColor;

        return defaultSkin;
    }

    static void DrawNineSlice(QPainter* painter, const QRectF& dest, const QPixmap& img, int l, int t, int r, int b)
    {
        if (img.isNull() || dest.isEmpty())
            return;

        const int iw = img.width();
        const int ih = img.height();
        const int midW = iw - l - r;
        const int midH = ih - t - b;

        if (midW <= 0 || midH <= 0)
        {
            painter->drawPixmap(dest, img, QRectF(0, 0, iw, ih));
            return;
        }

        const qreal dl = std::min<qreal>(l, dest.width()  * 0.5);
        const qreal dt = std::min<qreal>(t, dest.height() * 0.5);
        const qreal dr = std::min<qreal>(r, dest.width()  - dl);
        const qreal db = std::min<qreal>(b, dest.height() - dt);
        const qreal dw = std::max<qreal>(0.0, dest.width()  - dl - dr);
        const qreal dh = std::max<qreal>(0.0, dest.height() - dt - db);

        const qreal x0 = dest.x(), y0 = dest.y();
        const qreal x1 = x0 + dl, x2 = x1 + dw;
        const qreal y1 = y0 + dt, y2 = y1 + dh;

        auto SR = [](int x, int y, int w, int h){ return QRectF(x, y, w, h); };
        auto DR = [](qreal x, qreal y, qreal w, qreal h){ return QRectF(x, y, w, h); };

        painter->drawPixmap(DR(x0,y0,dl,dt), img, SR(0,0,l,t));
        painter->drawPixmap(DR(x1,y0,dw,dt), img, SR(l,0,midW,t));
        painter->drawPixmap(DR(x2,y0,dr,dt), img, SR(l+midW,0,r,t));
        painter->drawPixmap(DR(x0,y1,dl,dh), img, SR(0,t,l,midH));
        painter->drawPixmap(DR(x1,y1,dw,dh), img, SR(l,t,midW,midH));
        painter->drawPixmap(DR(x2,y1,dr,dh), img, SR(l+midW,t,r,midH));
        painter->drawPixmap(DR(x0,y2,dl,db), img, SR(0,t+midH,l,b));
        painter->drawPixmap(DR(x1,y2,dw,db), img, SR(l,t+midH,midW,b));
        painter->drawPixmap(DR(x2,y2,dr,db), img, SR(l+midW,t+midH,r,b));
    }

    QString text;
    QColor backgroundColor;
    QColor textColor;
    QString fontFamily;
    int pixelSize;
    QString fontPath;
    QString assetPath;
    QString imagePath;
    QPixmap customSkin;
    int sliceLeft   = 6;
    int sliceTop    = 6;
    int sliceRight  = 6;
    int sliceBottom = 6;

};

#endif
