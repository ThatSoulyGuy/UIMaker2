#ifndef UIMAKER2_CHECKS_HPP
#define UIMAKER2_CHECKS_HPP

// Shared by the check translation units.
void check(bool ok, const char* what);

// Layout settles over two queued hops: Component::NotifyChanged posts
// ComponentChanged, and SceneElementItem::OnComponentChanged then queues the
// refresh. Sampling after one turn reads a half-settled scene.
void Settle();

void CheckRowIndexSpace();
void CheckGeometryChecksum();
void CheckAnchorRoundTrip();
void CheckRegressions();
void CheckPixelSpans();
void CheckPixelRaster();
void CheckDocumentBake();
void CheckUiBinConformance();
void CheckTextOffset();
void CheckPropertyGroups();
void CheckTreeModelReset();
void CheckLayouts();
void CheckBlockBounds();
void CheckRecentDirs();
void CheckLayoutStretch();

#endif
