#include "ecs/Registry.h"

namespace duck {

EntityID Registry::create() {
    EntityID id = m_nextID++;
    m_alive.insert(id);
    return id;
}

void Registry::destroy(EntityID entity) {
    // 遍歷所有 pool，移除該 entity 的所有元件
    // 這就是 IComponentPool 型別擦除的價值：
    // 不需要知道具體的元件類型，就能呼叫 remove()
    for (auto& [type, pool] : m_pools) {
        pool->remove(entity);
    }
    m_alive.erase(entity);
}

bool Registry::alive(EntityID entity) const {
    return m_alive.count(entity) > 0;
}

} // namespace duck
