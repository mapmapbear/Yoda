#pragma once
#include <memory>
#include <vector>

namespace Yoda {

template <typename T> class TreeNode {
public:
  T value;
  std::vector<std::shared_ptr<TreeNode<T>>> children;
  TreeNode(T val) : value(val) {}
  void addChild(T val) {
    children.push_back(std::make_shared<TreeNode<T>>(val));
  }
};

template <typename T> class Tree {
public:
  std::shared_ptr<TreeNode<T>> root;

  Tree(T rootValue) { root = std::make_shared<TreeNode<T>>(rootValue); }

};
} // namespace Yoda
