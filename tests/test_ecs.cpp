// ============================================================
// ECS 單元測試
// ============================================================
// 為什麼要寫測試？
// ECS 是引擎的核心資料結構，如果 ComponentPool 的 add/remove/get 有 bug，
// 上層所有 System 都會出錯。在這裡用 assert 確保基本操作正確，
// 比在遊戲運行時才發現問題高效得多。
//
// 注意：這個測試不需要 OpenGL/SDL2，是純 CPU 邏輯測試，
// 所以即使在無 display 的 WSL2 環境也能跑。

#include "ecs/Entity.h"
#include "ecs/ComponentPool.h"
#include "ecs/Components.h"
#include "ecs/Registry.h"
#include <cassert>
#include <cstdio>
#include <vector>

// --------------------------------------------------
// 測試 1：基本的新增與取得
// --------------------------------------------------
void test_add_get() {
    duck::ComponentPool<duck::Transform> pool;
    duck::EntityID e1 = 0;

    pool.add(e1, {100.0f, 200.0f, 0.0f, 1.0f, 1.0f});

    assert(pool.has(e1));
    assert(pool.get(e1).x == 100.0f);
    assert(pool.get(e1).y == 200.0f);
    assert(pool.size() == 1);

    std::printf("  [PASS] test_add_get\n");
}

// --------------------------------------------------
// 測試 2：Swap-and-Pop 刪除
// --------------------------------------------------
// 這是最重要的測試！確認刪除中間元素後：
// - 被刪除的 entity 確實不見了
// - 其他 entity 的資料沒有被破壞
// - size 正確減少
void test_remove() {
    duck::ComponentPool<duck::Transform> pool;
    pool.add(0, {1, 2, 0, 1, 1});   // index 0
    pool.add(1, {3, 4, 0, 1, 1});   // index 1
    pool.add(2, {5, 6, 0, 1, 1});   // index 2

    pool.remove(1); // 刪除 index 1，entity 2 會被搬到 index 1

    assert(!pool.has(1));        // entity 1 已被刪除
    assert(pool.has(0));         // entity 0 還在
    assert(pool.has(2));         // entity 2 還在
    assert(pool.size() == 2);   // 只剩 2 個

    // 確認 entity 2 的資料在 swap 後仍然正確
    assert(pool.get(2).x == 5.0f);

    std::printf("  [PASS] test_remove\n");
}

// --------------------------------------------------
// 測試 3：Tag Component（空結構體）
// --------------------------------------------------
// 確認即使是 sizeof=1 的空結構體，
// add/has/remove 也能正常運作
void test_tag_component() {
    duck::ComponentPool<duck::InputControlled> pool;
    pool.add(42, {});

    assert(pool.has(42));
    assert(!pool.has(0));

    pool.remove(42);
    assert(!pool.has(42));

    std::printf("  [PASS] test_tag_component\n");
}

// ============================================================
// Registry 測試
// ============================================================

// --------------------------------------------------
// 測試 4：Entity 創建與銷毀
// --------------------------------------------------
// 確認：
// - create() 回傳遞增的 ID
// - destroy() 後 alive() 回傳 false
// - destroy() 也會清除該 entity 的所有 component
void test_registry_create_destroy() {
    duck::Registry reg;
    auto e1 = reg.create();  // 應該是 0
    auto e2 = reg.create();  // 應該是 1

    assert(reg.alive(e1));
    assert(reg.alive(e2));

    reg.destroy(e1);
    assert(!reg.alive(e1));
    assert(reg.alive(e2));

    std::printf("  [PASS] test_registry_create_destroy\n");
}

// --------------------------------------------------
// 測試 5：Component 掛載與查詢
// --------------------------------------------------
// 確認 addComponent/getComponent/hasComponent 的基本流程
void test_registry_components() {
    duck::Registry reg;
    auto e = reg.create();

    // addComponent 使用 variadic template，直接傳欄位值
    reg.addComponent<duck::Transform>(e, 10.0f, 20.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(e, 0.0f, 0.0f, 1.0f, 0.9f);

    assert(reg.hasComponent<duck::Transform>(e));
    assert(reg.hasComponent<duck::RigidBody>(e));
    assert(!reg.hasComponent<duck::InputControlled>(e));  // 沒加過的

    auto& tf = reg.getComponent<duck::Transform>(e);
    assert(tf.x == 10.0f);

    std::printf("  [PASS] test_registry_components\n");
}

// --------------------------------------------------
// 測試 6：View 多元件查詢
// --------------------------------------------------
// 這是 ECS 最關鍵的功能！
// System 會用 view 來找出「同時擁有多種元件」的 entity
//
// 場景：
//   e1: Transform + RigidBody          ← 應該被 view 找到
//   e2: 只有 Transform                 ← 不應該被找到
//   e3: Transform + RigidBody          ← 應該被 view 找到
void test_registry_view() {
    duck::Registry reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    auto e3 = reg.create();

    reg.addComponent<duck::Transform>(e1, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(e1, 0.0f, 0.0f, 1.0f, 0.9f);

    reg.addComponent<duck::Transform>(e2, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    // e2 沒有 RigidBody

    reg.addComponent<duck::Transform>(e3, 3.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::RigidBody>(e3, 0.0f, 0.0f, 1.0f, 0.9f);

    // view<Transform, RigidBody> 應該只找到 e1 和 e3
    std::vector<duck::EntityID> result;
    reg.view<duck::Transform, duck::RigidBody>([&](duck::EntityID e) {
        result.push_back(e);
    });

    assert(result.size() == 2);

    bool hasE1 = false, hasE3 = false;
    for (auto id : result) {
        if (id == e1) hasE1 = true;
        if (id == e3) hasE3 = true;
    }
    assert(hasE1 && hasE3);

    std::printf("  [PASS] test_registry_view\n");
}

// --------------------------------------------------
// 測試 7：destroy 後 view 不會遍歷到已銷毀的 entity
// --------------------------------------------------
void test_registry_destroy_removes_from_view() {
    duck::Registry reg;
    auto e1 = reg.create();
    auto e2 = reg.create();

    reg.addComponent<duck::Transform>(e1, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    reg.addComponent<duck::Transform>(e2, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    reg.destroy(e1);

    std::vector<duck::EntityID> result;
    reg.view<duck::Transform>([&](duck::EntityID e) {
        result.push_back(e);
    });

    assert(result.size() == 1);
    assert(result[0] == e2);

    std::printf("  [PASS] test_registry_destroy_removes_from_view\n");
}

int main() {
    std::printf("=== ComponentPool 測試 ===\n");
    test_add_get();
    test_remove();
    test_tag_component();

    std::printf("\n=== Registry 測試 ===\n");
    test_registry_create_destroy();
    test_registry_components();
    test_registry_view();
    test_registry_destroy_removes_from_view();

    std::printf("\n=== 全部通過 ===\n");
    return 0;
}
