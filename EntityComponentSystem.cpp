#include "EntityComponentSystem.hpp"

const bool TransformComponent::registered = [](){ Component::Register("Transform", [](QObject* parent){ return new TransformComponent(parent); }); return true; }();
const bool ImageComponent::registered = [](){ Component::Register("Image", [](QObject* parent){ return new ImageComponent(parent); }); return true; }();
const bool TextComponent::registered = [](){ Component::Register("Text", [](QObject* parent){ return new TextComponent(parent); }); return true; }();
const bool ButtonComponent::registered = [](){ Component::Register("Button", [](QObject* parent){ return new ButtonComponent(parent); }); return true; }();
