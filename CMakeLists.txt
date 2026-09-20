cmake_minimum_required(VERSION 3.16)

project(UIMaker2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Qt6::Test is used ONLY by the optional UIMaker2Checks target (QSignalSpy).
find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Test)

# Shared source list. Compiled into BOTH the app and the checks target.
#
# Deliberately NOT a static library: REGISTER_COMPONENT (Component.hpp) registers
# each component through a file-scope static initialiser, and the linker discards
# object files from a static library when nothing references their symbols. The
# registry would come back silently EMPTY and Component::Create would return
# nullptr for every kind, with no error.
set(UIMAKER_SOURCES

    # App
    src/app/MainWindow.hpp
    src/app/MainWindow.cpp
    src/app/mainwindow.ui

    # Core
    src/core/Anchor.hpp
    src/core/Component.hpp
    src/core/Component.cpp
    src/core/UiElement.hpp
    src/core/UiElement.cpp
    src/core/AssetContext.hpp
    src/core/AssetContext.cpp
    src/core/GridSnap.hpp
    src/core/GridSnap.cpp
    src/core/PixelModel.hpp
    src/core/PixelModel.cpp

    # Components
    src/components/TransformComponent.hpp
    src/components/TransformComponent.cpp
    src/components/ImageComponent.hpp
    src/components/ImageComponent.cpp
    src/components/TextComponent.hpp
    src/components/TextComponent.cpp
    src/components/ButtonComponent.hpp
    src/components/ButtonComponent.cpp
    src/components/StackLayoutComponent.hpp
    src/components/StackLayoutComponent.cpp
    src/components/GridLayoutComponent.hpp
    src/components/GridLayoutComponent.cpp
    src/components/ScrollBoxComponent.hpp
    src/components/ScrollBoxComponent.cpp
    src/components/PanelComponent.hpp
    src/components/PanelComponent.cpp
    src/components/ProgressBarComponent.hpp
    src/components/ProgressBarComponent.cpp
    src/components/ToggleComponent.hpp
    src/components/ToggleComponent.cpp
    src/components/DropdownComponent.hpp
    src/components/DropdownComponent.cpp
    src/components/TextInputComponent.hpp
    src/components/TextInputComponent.cpp
    src/components/IconComponent.hpp
    src/components/IconComponent.cpp
    src/components/SpriteComponent.hpp
    src/components/SpriteComponent.cpp
    src/components/TooltipComponent.hpp
    src/components/TooltipComponent.cpp
    src/components/ModalComponent.hpp
    src/components/ModalComponent.cpp
    src/components/TabContainerComponent.hpp
    src/components/TabContainerComponent.cpp
    src/components/RadialMenuComponent.hpp
    src/components/RadialMenuComponent.cpp
    src/components/MinimapComponent.hpp
    src/components/MinimapComponent.cpp
    src/components/DragSlotComponent.hpp
    src/components/DragSlotComponent.cpp
    src/components/ListRepeaterComponent.hpp
    src/components/ListRepeaterComponent.cpp
    src/components/SlotComponent.hpp
    src/components/SlotComponent.cpp

    # Scene
    src/scene/SceneDocument.hpp
    src/scene/SceneDocument.cpp
    src/scene/SceneElementItem.hpp
    src/scene/SceneElementItem.cpp
    src/scene/SceneExporter.hpp
    src/scene/SceneExporter.cpp
    src/scene/UiBinCommon.hpp
    src/scene/UiBinCommon.cpp
    src/scene/UiBinWriter.hpp
    src/scene/UiBinWriter.cpp
    src/scene/UiBinReader.hpp
    src/scene/UiBinReader.cpp

    # UI
    src/ui/EntityTreeModel.hpp
    src/ui/EntityTreeModel.cpp
    src/ui/PropertyEditorPanel.hpp
    src/ui/PropertyEditorPanel.cpp
    src/ui/ViewportWidget.hpp
    src/ui/ViewportWidget.cpp

    # Tools
    src/tools/Tool.hpp
    src/tools/Tool.cpp
    src/tools/ToolManager.hpp
    src/tools/ToolManager.cpp
    src/tools/TranslateTool.hpp
    src/tools/TranslateTool.cpp
    src/tools/RotateTool.hpp
    src/tools/RotateTool.cpp
    src/tools/ScaleTool.hpp
    src/tools/ScaleTool.cpp

    # Gizmos
    src/gizmos/Gizmo.hpp
    src/gizmos/Gizmo.cpp
    src/gizmos/GizmoManager.hpp
    src/gizmos/GizmoManager.cpp
    src/gizmos/TranslateGizmo.hpp
    src/gizmos/TranslateGizmo.cpp
    src/gizmos/RotateGizmo.hpp
    src/gizmos/RotateGizmo.cpp
    src/gizmos/ScaleGizmo.hpp
    src/gizmos/ScaleGizmo.cpp

    # Input
    src/input/InputEvents.hpp
    src/input/EditorContext.hpp
    src/input/EditorContext.cpp
    src/input/InputHandler.hpp
    src/input/InputHandler.cpp
    src/input/PanZoomHandler.hpp
    src/input/PanZoomHandler.cpp
    src/input/TransformInputHandler.hpp
    src/input/TransformInputHandler.cpp

    # Render
    src/render/RenderPass.hpp
    src/render/RenderPass.cpp
    src/render/RenderPipeline.hpp
    src/render/RenderPipeline.cpp
    src/render/GizmoRenderPass.hpp
    src/render/GizmoRenderPass.cpp
)

qt_add_executable(UIMaker2
    MANUAL_FINALIZATION

    src/main.cpp
    ${UIMAKER_SOURCES}
)

# Assertions over the real types. Built on demand:
#   cmake --build <dir> --target UIMaker2Checks
#   QT_QPA_PLATFORM=offscreen ./UIMaker2Checks
qt_add_executable(UIMaker2Checks
    EXCLUDE_FROM_ALL
    MANUAL_FINALIZATION

    tools/checks/checks.cpp
    tools/checks/checks.hpp
    tools/checks/regressions.cpp
    ${UIMAKER_SOURCES}
)

target_include_directories(UIMaker2Checks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(UIMaker2Checks PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)

target_include_directories(UIMaker2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(UIMaker2 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(UIMaker2 PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS UIMaker2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(UIMaker2)
qt_finalize_executable(UIMaker2Checks)
