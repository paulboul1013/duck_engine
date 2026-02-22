#pragma once
#include "ecs/Entity.h"
#include <vector>
#include <unordered_map>
#include <cassert>

namespace duck {

// ============================================================
// IComponentPool — 型別擦除基底類別
// ============================================================
// 為什麼需要這個？
// Registry 需要用一個 map 管理所有不同類型的 ComponentPool。
// 但 ComponentPool<Transform> 和 ComponentPool<Sprite> 是不同的類型，
// 無法直接放在同一個容器裡。
// IComponentPool 提供共同的虛擬介面，讓 Registry 可以統一操作它們。
// 這是 C++ 中「型別擦除（Type Erasure）」的經典手法。
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID entity) = 0;
    virtual bool has(EntityID entity) const = 0;
};

// ============================================================
// ComponentPool<T> — 快取友好的元件儲存池
// ============================================================
// 核心思想：Sparse Set（稀疏集合）
//
// 傳統做法（慢）：
//   std::map<EntityID, Component>
//   → 每次查詢是 O(log n)，資料散布在堆積的各處，cache miss 很高
//
// 我們的做法（快）：
//   Dense Array + 雙向映射
//   → 同類型元件在記憶體中 **連續排列**，遍歷時幾乎零 cache miss
//
// 記憶體佈局示意：
//   m_components:    [Transform_A] [Transform_B] [Transform_C]  ← 連續！
//   m_indexToEntity:  [entity_5]    [entity_2]    [entity_8]
//   m_entityToIndex:  {5→0, 2→1, 8→2}
//
template <typename T>
class ComponentPool : public IComponentPool {
public:
    // 新增元件到指定 entity
    T& add(EntityID entity, T component) {
        assert(!has(entity) && "Entity already has this component");
        size_t index = m_components.size();
        m_components.push_back(std::move(component));
        m_indexToEntity.push_back(entity);
        m_entityToIndex[entity] = index;
        return m_components.back();
    }

    // 取得 entity 的元件參照（可修改）
    T& get(EntityID entity) {
        assert(has(entity) && "Entity does not have this component");
        return m_components[m_entityToIndex[entity]];
    }

    // 取得 entity 的元件參照（唯讀）
    const T& get(EntityID entity) const {
        assert(has(entity) && "Entity does not have this component");
        return m_components[m_entityToIndex.at(entity)];
    }

    // 移除 entity 的元件
    // 關鍵技巧：Swap-and-Pop（交換並彈出）
    // 直接刪除中間元素會讓 vector 移動後面所有元素（O(n)）
    // Swap-and-Pop 則是把最後一個元素搬到被刪除的位置，然後 pop_back（O(1)）
    // 代價是不保證順序，但 ECS 不需要保證順序
    void remove(EntityID entity) override {
        if (!has(entity)) return;

        size_t indexToRemove = m_entityToIndex[entity];
        size_t lastIndex = m_components.size() - 1;

        if (indexToRemove != lastIndex) {
            // 把最後一個元素搬到被刪除的位置
            m_components[indexToRemove] = std::move(m_components[lastIndex]);
            EntityID lastEntity = m_indexToEntity[lastIndex];
            m_indexToEntity[indexToRemove] = lastEntity;
            m_entityToIndex[lastEntity] = indexToRemove;
        }

        m_components.pop_back();
        m_indexToEntity.pop_back();
        m_entityToIndex.erase(entity);
    }

    // 檢查 entity 是否擁有此類型元件
    bool has(EntityID entity) const override {
        return m_entityToIndex.find(entity) != m_entityToIndex.end();
    }

    // 元件數量
    size_t size() const { return m_components.size(); }

    // 供 View 遍歷用 — 回傳所有擁有此元件的 entity 列表
    const std::vector<EntityID>& entities() const { return m_indexToEntity; }

    // 直接存取底層元件陣列（進階用途）
    std::vector<T>& components() { return m_components; }

private:
    // Dense Array：所有同類型元件連續存放
    // 這是效能的關鍵！CPU 讀取記憶體時會預取相鄰的資料（cache line 通常 64 bytes）
    // 連續存放意味著遍歷時幾乎每次都是 cache hit
    std::vector<T> m_components;

    // Dense → Entity 映射：index i 對應哪個 entity
    std::vector<EntityID> m_indexToEntity;

    // Entity → Dense 映射：查詢特定 entity 的元件在哪個 index
    // 用 unordered_map 實現 O(1) 查詢
    std::unordered_map<EntityID, size_t> m_entityToIndex;
};

} // namespace duck
