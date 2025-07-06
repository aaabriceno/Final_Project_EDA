#pragma once
#include "boundingbox.h"
#include <cstddef>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <vector>
#include <iostream>

/*!
 * \brief Clase de plantilla RStarTree con verificaciones de seguridad
 * \param[in] LeafType - tipo almacenado en la hoja de un árbol
 * \param[in] dimensions - número de dimensiones, área limitada
 * \param[in] min_child_items - número mínimo de hijos de nodo
 * \param[in] max_child_items - número máximo de hijos de nodo
 *  */
template <typename LeafType, std::size_t dimensions,
                std::size_t min_child_items, std::size_t max_child_items>

class RStarTree {
  using BoundingBox = RStarBoundingBox<dimensions>;

private:
  struct TreePart {
      BoundingBox box;
      virtual ~TreePart() = default;
  };

  struct Node : public TreePart {
      std::vector<TreePart *> items;
      int hasleaves{false};
      ~Node() {
          for (auto item : items) {
              if (item) {
                  if (hasleaves) {
                      delete static_cast<Leaf*>(item);
                  } else {
                      delete static_cast<Node*>(item);
                  }
              }
          }
      }
  };

  struct Leaf : public TreePart {
      LeafType value;
  };

  struct SplitParameters {
      int index{-1};
      int axis{-1};
      axis_type type{axis_type::lower};
  };

public:
  /*!
     * \brief Una hoja cuyas instancias están en una matriz.,
     *  devuelto en el método de búsqueda, con un área delimitadora constante,
     * acceder a campos utilizando métodos
     * */
  class LeafWithConstBox {
  public:
      LeafWithConstBox(Leaf *leaf_) : leaf(leaf_) {}
      /*!
       * \brief Returns a constant reference to the bounding box
       * */
      const BoundingBox &get_box() const { return leaf->box; }
      const BoundingBox &get_box() { return leaf->box; }
      const LeafType &get_value() const { return leaf->value; }
      /*!
       * \brief Devuelve una referencia al valor de la hoja.
       * */
      LeafType &get_value() { return leaf->value; }
      bool operator<(const LeafWithConstBox &rhs) const {
          if (get_value() == rhs.get_value()) {
            return get_box() < rhs.get_box();
          }
          return get_value() < rhs.get_value();
      }
      bool operator==(const LeafWithConstBox &rhs) const {
          return get_value() == rhs.get_value() && get_box() == rhs.get_box();
      }
      bool operator!=(const LeafWithConstBox &rhs) const {
          return !operator==(rhs);
      }

  private:
      Leaf *leaf{nullptr};
  };

public:
  //! \brief Constructor RStarTree, comprueba que los parámetros de la plantilla
  //! sean correctos
  RStarTree() {
      if (dimensions <= 0 || max_child_items < min_child_items) {
          throw std::invalid_argument("Parámetros inválidos");
      }
  }
  ~RStarTree() { 
      if (tree_root) {
          delete_tree(tree_root); 
      }
  }

  /*!
     * \brief Method for inserting a leaf into a tree
     * \param[in] leaf - leaf with value
     * \param[in] box - bounding area
     *  */
  void insert(const LeafType leaf, const BoundingBox &box) {
      size_++;
      Leaf *new_leaf = new Leaf;
      new_leaf->value = leaf;
      new_leaf->box = box;
      if (!tree_root) {
          tree_root = new Node();
          tree_root->hasleaves = true;
          tree_root->items.reserve(min_child_items);
          tree_root->items.push_back(new_leaf);
      } else {
          choose_leaf_and_insert(new_leaf, tree_root);
      }
      used_deeps.clear();
  }

  /*!
     * \brief Method for searching objects in an area, returning an array of
     * leaves \param[in] box - search area
     *  */
  std::vector<LeafWithConstBox> find_objects_in_area(const BoundingBox &box) {
      std::vector<LeafWithConstBox> leafs;
      if (tree_root) {
          find_leaf(box, leafs, tree_root);
      }
      return leafs;
  }

  /*!
     * \brief Method for deleting objects in an area
     * \param[in] box - search area
     *  */
  void delete_objects_in_area(const BoundingBox &box) {
      if (tree_root) {
          delete_leafs(box, tree_root);
      }
  }

  /*!
     * \brief Method for recording a tree in binary format
     * \param[in] file - stream to which the tree is written
     *  */
  void write_in_binary_file(std::fstream &file) {
      if (!tree_root) return;
      
      int dimensions_ = dimensions;
      int min_child_items_ = min_child_items;
      int max_child_items_ = max_child_items;
      file.write(reinterpret_cast<const char *>(&dimensions_),
                         sizeof(dimensions_));
      file.write(reinterpret_cast<const char *>(&min_child_items_),
                         sizeof(min_child_items_));
      file.write(reinterpret_cast<const char *>(&max_child_items_),
                         sizeof(max_child_items_));
      std::queue<Node *> nodes; // BFS
      nodes.push(tree_root);
      while (!nodes.empty() && !nodes.front()->hasleaves) {
          write_node(nodes.front(), file);
          for (size_t i = 0; i < nodes.front()->items.size(); i++) {
            if (nodes.front()->items[i]) {
                nodes.push(static_cast<Node *>(nodes.front()->items[i]));
            }
          }
          nodes.pop();
      }
      std::vector<Leaf *> leafs;
      while (!nodes.empty()) {
          for (long long i = 0; i < nodes.front()->items.size(); i++) {
            if (nodes.front()->items[i]) {
                leafs.push_back(static_cast<Leaf *>(nodes.front()->items[i]));
            }
          }
          write_node(nodes.front(), file);
          nodes.pop();
      }
      for (auto &leaff : leafs) {
          if (leaff) {
              write_leaf(leaff, file);
          }
      }
  }

  /*!
     * \brief Method for reading a tree in binary format
     * \param[in] file - the stream from which the tree is read
     *  */
  void read_from_binary_file(std::fstream &file) {
      int dimensions_ = dimensions;
      int min_child_items_ = min_child_items;
      int max_child_items_ = max_child_items;
      file.read(reinterpret_cast<char *>(&dimensions_), sizeof(dimensions_));
      file.read(reinterpret_cast<char *>(&min_child_items_),
                      sizeof(min_child_items_));
      file.read(reinterpret_cast<char *>(&max_child_items_),
                      sizeof(max_child_items_));
      if (dimensions_ != dimensions || min_child_items != min_child_items_ ||
            max_child_items != max_child_items_)
          throw std::invalid_argument("Parámetros de archivo no coinciden");
      if (tree_root) {
          delete_tree(tree_root);
      }
      tree_root = read_node(file);
      std::queue<Node *> nodes; // same as BFS
      nodes.push(tree_root);
      while (!nodes.empty()) {
          for (size_t i = 0; i < nodes.front()->items.size(); i++) {
            Node *new_node = read_node(file);
            nodes.front()->items[i] = new_node;
            nodes.push(new_node);
          }
          nodes.pop();
          if (nodes.empty() || nodes.front()->hasleaves)
            break;
      }
      while (!nodes.empty()) {
          for (size_t i = 0; i < nodes.front()->items.size(); i++) {
            Leaf *new_leaf = read_leaf(file);
            nodes.front()->items[i] = new_leaf;
          }
          nodes.pop();
      }
  }

private:
  void write_node(Node *node, std::fstream &file) {
      if (!node) return;
      
      size_t _size = node->items.size();
      file.write(reinterpret_cast<const char *>(&_size), sizeof(_size));
      file.write(reinterpret_cast<const char *>(&(node->hasleaves)),
                         sizeof(node->hasleaves));
      for (size_t axis = 0; axis < dimensions; axis++) {
          file.write(reinterpret_cast<const char *>(&(node->box.max_edges[axis])),
                           sizeof(node->box.max_edges[axis]));
          file.write(reinterpret_cast<const char *>(&(node->box.min_edges[axis])),
                           sizeof(node->box.min_edges[axis]));
      }
  }

  void write_leaf(Leaf *leaf, std::fstream &file) {
      if (!leaf) return;
      
      for (size_t axis = 0; axis < dimensions; axis++) {
          file.write(reinterpret_cast<char *>(&(leaf->box.max_edges[axis])),
                           sizeof(leaf->box.max_edges[axis]));
          file.write(reinterpret_cast<char *>(&(leaf->box.min_edges[axis])),
                           sizeof(leaf->box.min_edges[axis]));
      }
      file.write(reinterpret_cast<char *>(&(leaf->value)), sizeof(LeafType));
      size_--;
  }

  Node *read_node(std::fstream &file) {
      size_t _size;
      file.read(reinterpret_cast<char *>(&_size), sizeof(_size));
      Node *new_node = new Node;
      new_node->items.resize(_size);
      file.read(reinterpret_cast<char *>(&(new_node->hasleaves)),
                      sizeof(new_node->hasleaves));
      for (size_t axis = 0; axis < dimensions; axis++) {
          file.read(reinterpret_cast<char *>(&(new_node->box.max_edges[axis])),
                          sizeof(new_node->box.max_edges[axis]));
          file.read(reinterpret_cast<char *>(&(new_node->box.min_edges[axis])),
                          sizeof(new_node->box.min_edges[axis]));
      }
      return new_node;
  }

  Leaf *read_leaf(std::fstream &file) {
      Leaf *new_leaf = new Leaf;
      for (size_t axis = 0; axis < dimensions; axis++) {
          file.read(reinterpret_cast<char *>(&(new_leaf->box.max_edges[axis])),
                          sizeof(new_leaf->box.max_edges[axis]));
          file.read(reinterpret_cast<char *>(&(new_leaf->box.min_edges[axis])),
                          sizeof(new_leaf->box.min_edges[axis]));
      }
      file.read(reinterpret_cast<char *>(&(new_leaf->value)), sizeof(LeafType));
      size_++;
      return new_leaf;
  }

  void find_leaf(const BoundingBox &box, std::vector<LeafWithConstBox> &leafs,
                           Node *node) {
      if (!node) return;
      
      if (node->hasleaves) {
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i]) {
                Leaf *temp_leaf = static_cast<Leaf *>(node->items[i]);
                if (box.is_intersected((temp_leaf->box))) {
                    leafs.push_back({temp_leaf});
                }
            }
          }
      } else {
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i]) {
                Node *temp_node = static_cast<Node *>(node->items[i]);
                if (box.is_intersected((temp_node->box))) {
                    find_leaf(box, leafs, temp_node);
                }
            }
          }
      }
  }

  void delete_leafs(const BoundingBox &box, Node *node) {
      if (!node) return;
      
      if (node->hasleaves) { // If the children of an area are leaves, then all
                                             // children are tested.
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i] && box.is_intersected((node->items[i]->box))) {
                std::swap(node->items[i],
                                node->items.back()); // changing from the last one
                delete node->items.back();     // delete the last one
                node->items.pop_back();
                i--;
            }
          }
          node->box.reset();
      } else {
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i] && box.is_intersected((node->items[i]->box))) {
                delete_leafs(box,
                                     static_cast<Node *>(
                                             node->items[i])); // we call the method from those
                                                                           // eedges whose regions intersect
            }
          }
      }
      for (size_t i = 0; i < node->items.size(); i++) {
          if (node->items[i]) {
              node->box.stretch(node->items[i]->box);
          }
      }
  }

  Node *choose_leaf_and_insert(Leaf *leaf, Node *node, int deep = 0) {
      if (!node || !leaf) return nullptr;
      
      node->box.stretch(leaf->box);
      if (node->hasleaves) { // If the children of the node are leaves, add a
                                             // leaf to the array
          node->items.push_back(leaf);
      } else {
          Node *new_node = choose_leaf_and_insert(
                leaf, choose_subtree(node, leaf->box), deep + 1);
          if (!new_node) { // choose_leaf_and_insert will return the location, it
                                     // will be inserted into the children's array
            return nullptr;
          }
          node->items.push_back(new_node);
      }
      if (node->items.size() > max_child_items) {
          return overflow_treatment(
                node, deep); // If the number of children is greater than
                                     // max_child_items, the node must be divided.
      }
      return nullptr;
  }
  
  Node *choose_node_and_insert(Node *node, Node *parent_node, int required_deep,
                                                   int deep = 0) {
      if (!parent_node || !node) return nullptr;
      
      parent_node->box.stretch(node->box);
      if (deep ==
            required_deep) { // Similar to choose_leaf_and_insert, only for nodes
          parent_node->items.push_back(
                node); // The location is set at a predetermined depth so that the
                           // tree remains perfectly balanced.
      } else {
          Node *new_node =
                choose_node_and_insert(node, choose_subtree(parent_node, node->box),
                                                       required_deep, deep + 1);
          if (!new_node)
            return nullptr;
          parent_node->items.push_back(new_node);
      }
      if (parent_node->items.size() > max_child_items) {
          return overflow_treatment(parent_node, deep);
      }
      return nullptr;
  }
  
  Node *choose_subtree(Node *node, const BoundingBox &box) {
      if (!node || node->items.empty()) return nullptr;
      
      std::vector<TreePart *> overlap_preferable_nodes;
      if (node->items[0] && static_cast<Node *>(node->items[0])
                    ->hasleaves) { // If the child nodes are terminal nodes, the node
                                             // with the smallest overlap is searched for
          int min_overlap_enlargement(std::numeric_limits<int>::max());
          int overlap_enlargement(0);
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i]) {
                TreePart *temp = (node->items[i]);
                overlap_enlargement = box.area() - box.overlap(temp->box);
                if (overlap_enlargement < min_overlap_enlargement) {
                    min_overlap_enlargement = overlap_enlargement;
                    overlap_preferable_nodes.resize(0);
                    overlap_preferable_nodes.push_back(temp);
                } else {
                    if (overlap_enlargement == min_overlap_enlargement) {
                        overlap_preferable_nodes.push_back(temp);
                    }
                }
            }
          }
          if (overlap_preferable_nodes.size() ==
                1) { // If there's only one node, it's a special node.
            return static_cast<Node *>(overlap_preferable_nodes.front());
          } // If not, then a space with the smallest possible increase in area is
            // searched for
      } else { // If the child nodes are not terminal, keep them in the array
          overlap_preferable_nodes.reserve(node->items.size());
          for (auto item : node->items) {
              if (item) {
                  overlap_preferable_nodes.push_back(item);
              }
          }
      }
      int min_area_enlargement =
            std::numeric_limits<int>::max(); // for both terminal and nonterminal
      int area_enlargement(0);             // subsequent steps are the same
      std::vector<TreePart *> area_preferable_nodes;
      for (size_t i = 0; i < overlap_preferable_nodes.size(); i++) {
          if (overlap_preferable_nodes[i]) {
              BoundingBox temp(box);
              temp.stretch(overlap_preferable_nodes[i]->box);
              area_enlargement = temp.area() - box.area();
              if (min_area_enlargement > area_enlargement) {
                min_area_enlargement = area_enlargement;
                area_preferable_nodes.resize(0);
                area_preferable_nodes.push_back(overlap_preferable_nodes[i]);
              } else {
                if (min_area_enlargement == area_enlargement) {
                    area_preferable_nodes.push_back(overlap_preferable_nodes[i]);
                }
              }
          }
      }
      if (area_preferable_nodes.size() ==
            1) { // If there is only one minimum-increase-area node, it will be
                     // returned
          return static_cast<Node *>(area_preferable_nodes.front());
      }
      TreePart *min_area_node{nullptr}; // Looking for a node among the remaining
                                                              // ones with the smallest possible area
      int min_area(std::numeric_limits<int>::max());
      for (size_t i = 0; i < area_preferable_nodes.size(); i++) {
          if (area_preferable_nodes[i] && min_area > area_preferable_nodes[i]->box.area()) {
            min_area_node = area_preferable_nodes[i];
          }
      }
      return static_cast<Node *>(min_area_node);
  }
  
  Node *overflow_treatment(Node *node, int deep) {
      if (!node) return nullptr;
      
      if (used_deeps.count(deep) == 0 &&
            tree_root != node) { // The reinsertion method can be used only once
                                               // per depth and not for the root.
          forced_reinsert(node, deep);
          return nullptr;
      }
      Node *splitted_node =
            split(node); // deletes unnecessary children from the node and returns
                                   // the location with them
      if (node == tree_root) { // If node is the root of a tree, it grows one
                                               // level upward
          Node *temp = new Node;
          temp->hasleaves = false;
          temp->items.reserve(min_child_items);
          temp->items.push_back(tree_root);
          temp->items.push_back(splitted_node);
          tree_root = temp;
          tree_root->box.reset();
          tree_root->box.stretch(temp->items[0]->box);
          tree_root->box.stretch(temp->items[1]->box);

          return nullptr;
      }
      return splitted_node; // Otherwise, the location is returned for the
                                          // insertion of the children into the parent's
                                          // array.
  }
  
  Node *split(Node *node) {
      if (!node) return nullptr;
      
      SplitParameters params = choose_split_axis_and_index(
            node); // The most optimal index and axis are selected
      std::sort(node->items.begin(), node->items.end(),
                      [&params](auto lhs, auto rhs) {
                          if (!lhs || !rhs) return false;
                          return lhs->box.value_of_axis(params.axis, params.type) <
                                     rhs->box.value_of_axis(params.axis, params.type);
                      });
      Node *new_Node = new Node;
      new_Node->items.reserve(max_child_items + 1 - min_child_items -
                                              params.index);
      new_Node->hasleaves = node->hasleaves;
      std::copy(node->items.begin() + min_child_items + params.index,
                      node->items.end(), std::back_inserter(new_Node->items));
      node->items.erase(node->items.begin() + min_child_items + params.index,
                                    node->items.end());
      new_Node->box.reset();
      node->box.reset();
      for (auto &w : node->items) {
          if (w) {
              node->box.stretch(w->box);
          }
      }
      for (auto &w : new_Node->items) {
          if (w) {
              new_Node->box.stretch(w->box);
          }
      }
      return new_Node;
  }
  
  void forced_reinsert(Node *node,
                                     int deep) { // Some of the children of the given tree
                                                         // are reinserted into the tree
      if (!node) return;
      
      double p = 0.3; // Percentage of children that will be deleted from the
                                // node location
      int number = node->items.size() * p;
      std::sort(node->items.begin(), node->items.end(),
                      [&node](auto lhs, auto rhs) {
                          if (!lhs || !rhs) return false;
                          return lhs->box.dist_between_centers(node->box) <
                                     rhs->box.dist_between_centers(node->box);
                      });
      std::vector<TreePart *> forced_reinserted_nodes;
      forced_reinserted_nodes.reserve(number);
      std::copy(node->items.rbegin(), node->items.rbegin() + number,
                      std::back_inserter(forced_reinserted_nodes));
      node->items.erase(node->items.end() - number, node->items.end());
      node->box.reset();
      used_deeps.insert(deep);
      for (TreePart *w : node->items) {
          if (w) {
              node->box.stretch(w->box);
          }
      }
      if (node->hasleaves) // If the children of the top are leaves, they are
                                         // inserted again using the method
                                         // choose_leaf_and_insert.
          for (TreePart *w : forced_reinserted_nodes) {
            if (w) {
                choose_leaf_and_insert(static_cast<Leaf *>(w), tree_root, 0);
            }
          }
      else { // If nodes - by the method choose_node_and_insert to a specified
                 // depth
          for (TreePart *w : forced_reinserted_nodes) {
            if (w) {
                choose_node_and_insert(static_cast<Node *>(w), tree_root, deep, 0);
            }
          }
      }
  }

  SplitParameters choose_split_axis_and_index(Node *node) {
      if (!node) return SplitParameters();
      
      int min_margin(std::numeric_limits<int>::max());
      int distribution_count(max_child_items - 2 * min_child_items + 2);
      SplitParameters params;
      BoundingBox b1, b2;
      for (int axis(0); axis < dimensions;
               axis++) { // the axis of the best allocation is selected
          for (int i(0); i < 2; i++) { // on the larger or on the smaller border
            axis_type type;
            if (i == 0) {
                type = axis_type::lower;
            } else {
                type = axis_type::upper;
            }
            std::sort(node->items.begin(), node->items.end(),
                              [&axis, &type](auto &lhs, auto &rhs) {
                                if (!lhs || !rhs) return false;
                                return lhs->box.value_of_axis(axis, type) <
                                             rhs->box.value_of_axis(axis, type);
                              });
            for (int k(0); k < distribution_count; k++) {
                int area(0);
                b1.reset();
                b2.reset();
                for (int i = 0; i < min_child_items + k; i++) {
                    if (i < node->items.size() && node->items[i]) {
                        b1.stretch(node->items[i]->box);
                    }
                }
                for (int i = min_child_items + k; i < max_child_items + 1; i++) {
                    if (i < node->items.size() && node->items[i]) {
                        b2.stretch(node->items[i]->box);
                    }
                }
                int margin = b1.margin() + b2.margin();
                if (margin < min_margin) {
                    min_margin = margin;
                    params.index = k;
                    params.axis = axis;
                    params.type = type;
                }
            }
          }
      }
      return params;
  }

  void delete_tree(Node *node) {
      if (!node) return;
      
      if (node->hasleaves) {
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i]) {
                delete node->items[i];
            }
          }
      } else {
          for (size_t i = 0; i < node->items.size(); i++) {
            if (node->items[i]) {
                delete_tree(static_cast<Node *>(node->items[i]));
            }
          }
      }
      delete node;
  }

private:
  std::unordered_set<int>
          used_deeps; //<boundaries used during the current insertion
  Node *tree_root{nullptr};
  std::size_t size_{0}; //<number of leaves
}; 