#include <gtest/gtest.h>
#include "fixed_resource.hpp"
#include "dyn_array.hpp"

struct Data {
    int a;
    double b;
    char c;
    Data() : a(0), b(0.0), c(0) {}
    Data(int x, double y, char z) : a(x), b(y), c(z) {}
    bool operator==(const Data& o) const { return a == o.a && b == o.b && c == o.c; }
};

class FixedResourceTest : public ::testing::Test {
protected:
    FixedResource res{1024};
};

TEST_F(FixedResourceTest, Allocate) {
    void* p = res.allocate(64, 8);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(res.used_count(), 1);
}

TEST_F(FixedResourceTest, Deallocate) {
    void* p = res.allocate(64, 8);
    res.deallocate(p, 64, 8);
    EXPECT_EQ(res.used_count(), 0);
    EXPECT_EQ(res.free_count(), 1);
}

TEST_F(FixedResourceTest, Reuse) {
    void* p1 = res.allocate(64, 8);
    res.deallocate(p1, 64, 8);
    void* p2 = res.allocate(32, 8);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(res.free_count(), 0);
}

TEST_F(FixedResourceTest, OutOfMemory) {
    EXPECT_THROW(res.allocate(2048, 8), std::bad_alloc);
}

TEST_F(FixedResourceTest, MultipleAllocations) {
    for (int i = 0; i < 10; ++i) {
        res.allocate(32, 8);
    }
    EXPECT_EQ(res.used_count(), 10);
}

class DynArrayTest : public ::testing::Test {
protected:
    FixedResource res{4096};
    std::pmr::polymorphic_allocator<int> alloc{&res};
};

TEST_F(DynArrayTest, PushBack) {
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST_F(DynArrayTest, EmplaceBack) {
    std::pmr::polymorphic_allocator<Data> dalloc(&res);
    DynArray<Data> arr(dalloc);
    arr.emplace_back(1, 2.5, 'a');
    EXPECT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0].a, 1);
    EXPECT_DOUBLE_EQ(arr[0].b, 2.5);
    EXPECT_EQ(arr[0].c, 'a');
}

TEST_F(DynArrayTest, PopBack) {
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    arr.pop_back();
    EXPECT_EQ(arr.size(), 1);
}

TEST_F(DynArrayTest, Clear) {
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    arr.clear();
    EXPECT_TRUE(arr.empty());
}

TEST_F(DynArrayTest, At) {
    DynArray<int> arr(alloc);
    arr.push_back(42);
    EXPECT_EQ(arr.at(0), 42);
    EXPECT_THROW(arr.at(1), std::out_of_range);
}

TEST_F(DynArrayTest, Reserve) {
    DynArray<int> arr(alloc);
    arr.reserve(100);
    EXPECT_GE(arr.capacity(), 100);
}

TEST_F(DynArrayTest, Iterator) {
    DynArray<int> arr(alloc);
    for (int i = 0; i < 5; ++i) arr.push_back(i);
    int sum = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 10);
}

TEST_F(DynArrayTest, RangeFor) {
    DynArray<int> arr(alloc);
    for (int i = 0; i < 5; ++i) arr.push_back(i * 2);
    int idx = 0;
    for (const auto& v : arr) {
        EXPECT_EQ(v, idx * 2);
        ++idx;
    }
}

TEST_F(DynArrayTest, CopyConstructor) {
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    DynArray<int> arr2(arr);
    EXPECT_EQ(arr2.size(), 2);
    EXPECT_EQ(arr2[0], 1);
    EXPECT_EQ(arr2[1], 2);
}

TEST_F(DynArrayTest, MoveConstructor) {
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    DynArray<int> arr2(std::move(arr));
    EXPECT_EQ(arr2.size(), 2);
    EXPECT_TRUE(arr.empty());
}

TEST_F(DynArrayTest, StructType) {
    std::pmr::polymorphic_allocator<Data> dalloc(&res);
    DynArray<Data> arr(dalloc);
    arr.push_back(Data(1, 1.1, 'x'));
    arr.push_back(Data(2, 2.2, 'y'));
    EXPECT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0], Data(1, 1.1, 'x'));
}

TEST_F(DynArrayTest, FrontBack) {
    DynArray<int> arr(alloc);
    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    EXPECT_EQ(arr.front(), 10);
    EXPECT_EQ(arr.back(), 30);
}

TEST(IteratorTest, ForwardIteratorConcept) {
    FixedResource res(1024);
    std::pmr::polymorphic_allocator<int> alloc(&res);
    DynArray<int> arr(alloc);
    arr.push_back(1);
    
    auto it = arr.begin();
    static_assert(std::is_same_v<decltype(it)::iterator_category, std::forward_iterator_tag>);
    
    auto it2 = it++;
    EXPECT_EQ(*it2, 1);
    EXPECT_EQ(it, arr.end());
}

TEST(IteratorTest, ConstIteratorCorrectness) {
    FixedResource res(1024);
    std::pmr::polymorphic_allocator<int> alloc(&res);
    DynArray<int> arr(alloc);
    arr.push_back(42);
    
    const DynArray<int>& carr = arr;
    auto cit = carr.begin();
    static_assert(std::is_same_v<decltype(*cit), const int&>);
    static_assert(std::is_same_v<decltype(cit)::iterator_category, std::forward_iterator_tag>);
    EXPECT_EQ(*cit, 42);
}

TEST(IteratorTest, IteratorToConstIteratorConversion) {
    FixedResource res(1024);
    std::pmr::polymorphic_allocator<int> alloc(&res);
    DynArray<int> arr(alloc);
    arr.push_back(1);
    arr.push_back(2);
    
    DynArray<int>::iterator it = arr.begin();
    DynArray<int>::const_iterator cit = it;
    EXPECT_EQ(*cit, 1);
    
    DynArray<int>::const_iterator cend = arr.end();
    EXPECT_NE(cit, cend);
    ++cit;
    ++cit;
    EXPECT_EQ(cit, cend);
}

TEST(MemoryReuseTest, FullCycle) {
    FixedResource res(512);
    std::pmr::polymorphic_allocator<int> alloc(&res);
    
    {
        DynArray<int> arr(alloc);
        for (int i = 0; i < 10; ++i) arr.push_back(i);
    }
    
    EXPECT_GT(res.free_count(), 0);
    
    {
        DynArray<int> arr2(alloc);
        arr2.push_back(100);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
