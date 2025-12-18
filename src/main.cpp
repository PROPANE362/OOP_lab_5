#include <iostream>
#include "fixed_resource.hpp"
#include "dyn_array.hpp"
using namespace std;

struct Point {
    int x, y, z;
    Point() : x(0), y(0), z(0) {}
    Point(int a, int b, int c) : x(a), y(b), z(c) {}
};

int main() {
    FixedResource res(4096);
    pmr::polymorphic_allocator<int> alloc(&res);

    cout << "=== int ===" << endl;
    {
        DynArray<int> arr(alloc);
        for (int i = 0; i < 10; ++i) {
            arr.push_back(i * i);
        }
        cout << "razmer: " << arr.size() << endl;
        cout << "cho vnutri: ";
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            cout << *it << " ";
        }
        cout << endl;
        cout << "ispolzovani bloki: " << res.used_count() << endl;
    }
    cout << "svobodni bloki: " << res.free_count() << endl;

    cout << "\n=== Tochke ===" << endl;
    {
        pmr::polymorphic_allocator<Point> palloc(&res);
        DynArray<Point> pts(palloc);
        pts.emplace_back(1, 2, 3);
        pts.emplace_back(4, 5, 6);
        pts.push_back(Point(7, 8, 9));

        cout << "size: " << pts.size() << endl;
        for (const auto& p : pts) {
            cout << "(" << p.x << "," << p.y << "," << p.z << ") ";
        }
        cout << endl;
    }

    cout << "\n=== pereispolzovatb ===" << endl;
    {
        DynArray<int> arr2(alloc);
        arr2.push_back(100);
        arr2.push_back(200);
        cout << "pereispolzovani, razmer: " << arr2.size() << endl;
        cout << "svobodni bloki teperb: " << res.free_count() << endl;
    }

    return 0;
}
