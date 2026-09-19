#include "hsl/arena.hpp"
#include <cassert>
#include <string>

struct Node {
    std::string value;
    std::size_t parent;
};

int main() {
    hsl::Arena<Node> arena;
    const auto root = arena.create(Node{"root", 0});
    const auto child = arena.create(Node{"child", root});
    assert(arena.size() == 2);
    assert(arena.get(root).value == "root");
    assert(arena.get(child).parent == root);
    const Node* stable = &arena.get(root);
    for (int index = 0; index < 10000; ++index) arena.create(Node{"node", root});
    assert(stable == &arena.get(root));
    assert(stable->value == "root");
}
