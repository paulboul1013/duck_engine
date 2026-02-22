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
#include <cassert>
#include <cstdio>

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

int main() {
    std::printf("=== ComponentPool 測試 ===\n");
    test_add_get();
    test_remove();
    test_tag_component();
    std::printf("=== 全部通過 ===\n");
    return 0;
}
