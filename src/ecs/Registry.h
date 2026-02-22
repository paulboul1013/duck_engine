#pragma once
#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace duck {

// ============================================================
// Registry — ECS 的核心管理器
// ============================================================
// 職責：
// 1. Entity 的創建與銷毀（生命週期管理）
// 2. Component 的掛載與移除（委派給 ComponentPool）
// 3. View 查詢（找出同時擁有指定元件的所有 entity）
//
// 設計取捨：
// - 用 std::type_index 作為 ComponentPool 的 key
//   優點：不需要手動為每個 Component 指定 ID，自動化
//   缺點：type_index 依賴 RTTI，有微小的效能開銷
//   對我們的規模（100+ entity）完全可以接受
//
// - 用 unordered_set 追蹤存活的 entity
//   優點：O(1) 查詢 alive 狀態
//   缺點：比 bitset 佔更多記憶體
//   但 bitset 需要預先知道最大 entity 數，不夠彈性
//
class Registry {
public:
    // --------------------------------------------------
    // Entity 生命週期
    // --------------------------------------------------

    // 創建新 entity，回傳唯一的遞增 ID
    EntityID create();

    // 銷毀 entity：移除它的所有 component，再從存活集合中刪除
    void destroy(EntityID entity);

    // 檢查 entity 是否仍然存活
    bool alive(EntityID entity) const;

    // --------------------------------------------------
    // Component 操作（模板方法，定義在 header 中）
    // --------------------------------------------------

    // 新增元件到 entity
    // 使用 variadic template + perfect forwarding
    // 這樣呼叫端可以寫：
    //   reg.addComponent<Transform>(e, 100.0f, 200.0f, 0.0f, 1.0f, 1.0f);
    // 而不需要手動建構 Transform 物件
    template <typename T, typename... Args>
    T& addComponent(EntityID entity, Args&&... args) {
        auto& pool = getOrCreatePool<T>();
        return pool.add(entity, T{std::forward<Args>(args)...});
    }

    // 取得 entity 的元件參照（可修改）
    template <typename T>
    T& getComponent(EntityID entity) {
        return getPool<T>().get(entity);
    }

    // 檢查 entity 是否擁有指定元件
    template <typename T>
    bool hasComponent(EntityID entity) const {
        auto it = m_pools.find(std::type_index(typeid(T)));
        if (it == m_pools.end()) return false;
        return static_cast<const ComponentPool<T>*>(it->second.get())->has(entity);
    }

    // 移除 entity 的指定元件
    template <typename T>
    void removeComponent(EntityID entity) {
        getPool<T>().remove(entity);
    }

    // --------------------------------------------------
    // View — 多元件查詢
    // --------------------------------------------------
    // view<Transform, RigidBody>([](EntityID e) { ... })
    //
    // 會找出同時擁有 Transform 和 RigidBody 的所有 entity，
    // 對每個 entity 呼叫 callback。
    //
    // 實作策略：
    // 用第一個模板參數的 pool 作為起點遍歷，
    // 對每個 entity 檢查是否也擁有其他元件。
    //
    // C++17 fold expression: (hasComponent<Ts>(entity) && ...)
    // 這會展開成：
    //   hasComponent<Transform>(entity) && hasComponent<RigidBody>(entity)
    // 如果任一個為 false，短路求值會直接跳過（不浪費時間）
    //
    // 進階優化（未實作）：
    // 可以先找最小的 pool 作為起點，減少遍歷次數。
    // 目前先用第一個參數的 pool，Phase 1 夠用。
    template <typename... Ts>
    void view(std::function<void(EntityID)> func) {
        // 用 viewImpl 把第一個類型拆出來
        viewImpl<Ts...>(func);
    }

private:
    // 拆開參數包：First 是第一個類型，用它的 pool 作遍歷起點
    // 這避免了 alias template + pack expansion 的 GCC 相容性問題
    template <typename First, typename... Rest>
    void viewImpl(std::function<void(EntityID)>& func) {
        auto* pool = getPoolPtr<First>();
        if (!pool) return;

        // 複製 entities 列表，因為 callback 中可能修改 pool
        auto entities = pool->entities();
        for (EntityID entity : entities) {
            // fold expression：檢查是否同時擁有所有指定元件
            if ((hasComponent<First>(entity) && ... && hasComponent<Rest>(entity))) {
                func(entity);
            }
        }
    }

    // 取得或建立指定類型的 ComponentPool
    // 如果 pool 不存在，自動建立一個新的
    template <typename T>
    ComponentPool<T>& getOrCreatePool() {
        auto key = std::type_index(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            auto& ref = *pool;
            m_pools[key] = std::move(pool);
            return static_cast<ComponentPool<T>&>(ref);
        }
        return static_cast<ComponentPool<T>&>(*it->second);
    }

    // 取得已存在的 pool（不建立新的，若不存在會拋異常）
    template <typename T>
    ComponentPool<T>& getPool() {
        auto key = std::type_index(typeid(T));
        return static_cast<ComponentPool<T>&>(*m_pools.at(key));
    }

    // 取得 pool 指標（若不存在回傳 nullptr）
    template <typename T>
    const ComponentPool<T>* getPoolPtr() const {
        auto key = std::type_index(typeid(T));
        auto it = m_pools.find(key);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const ComponentPool<T>*>(it->second.get());
    }

    // Entity ID 產生器：簡單遞增
    EntityID m_nextID = 0;

    // 存活 entity 集合
    std::unordered_set<EntityID> m_alive;

    // 所有 ComponentPool 的容器
    // key = type_index（由 typeid(T) 產生）
    // value = unique_ptr<IComponentPool>（型別擦除的 pool）
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

} // namespace duck
