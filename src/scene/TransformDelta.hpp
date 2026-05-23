#ifndef SCENE_TRANSFORMDELTA_HPP
#define SCENE_TRANSFORMDELTA_HPP

#include <QList>
#include <QPointF>
#include <QUuid>

// One element's worth of "what the gizmo just did". Captured per affected
// element at HandlePress / HandleRelease and shipped to MainWindow so the
// undo system can store the delta (not a full-scene JSON snapshot) and apply
// it by id in O(touched elements) instead of rebuilding the whole tree.
struct TransformDelta
{
    QUuid   id;
    QPointF beforePos;
    QPointF afterPos;
    double  beforeRotation = 0.0;
    double  afterRotation  = 0.0;
    QPointF beforeScale;
    QPointF afterScale;
};

#endif
