cmake_minimum_required(VERSION 3.16)

project(UIMaker2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

qt_add_executable(UIMaker2
    MANUAL_FINALIZATION

    # App
    src/main.cpp
    src/app/MainWindow.hpp
    src/app/MainWindow.cpp
    src/app/mainwindow.ui

    # Core
    src/core/Anchor.hpp
    src/core/Component.hpp
    src/core/UiElement.hpp

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

    # UI
    src/ui/EntityTreeModel.hpp
    src/ui/EntityTreeModel.cpp
    src/ui/PropertyEditorPanel.hpp
    src/ui/PropertyEditorPanel.cpp
    src/ui/ViewportWidget.hpp

    # Tools
    src/tools/Tool.hpp
    src/tools/ToolManager.hpp
    src/tools/TranslateTool.hpp
    src/tools/TranslateTool.cpp
    src/tools/RotateTool.hpp
    src/tools/RotateTool.cpp
    src/tools/ScaleTool.hpp
    src/tools/ScaleTool.cpp

    # Gizmos
    src/gizmos/Gizmo.hpp
    src/gizmos/GizmoManager.hpp
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
    src/input/PanZoomHandler.hpp
    src/input/TransformInputHandler.hpp
    src/input/TransformInputHandler.cpp

    # Render
    src/render/RenderPass.hpp
    src/render/RenderPipeline.hpp
    src/render/GizmoRenderPass.hpp
    src/render/SelectionRenderPass.hpp
)

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
