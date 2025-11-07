#include "Component.hpp"

namespace game::ecs {

ComponentTypeID ComponentRegistry::nextTypeID = 0;
std::unordered_map<std::type_index, ComponentTypeID> ComponentRegistry::typeMap;

} // namespace game::ecs

