#ifndef B_TREE
#define B_TREE

#include <vector>
#include <queue>
#include <iostream>
#include <tuple>
#include <sstream>

template <class Key>
struct Node {

    std::vector<Key> keys_;
    std::vector<Node<Key> *> children_;

    int NumKeys() {
        return keys_.size();
    }

    int NumChildren() {
        return children_.size();
    }

    void RemoveKeyAt(int idx) {
        keys_.erase(keys_.begin() + idx);
        children_.erase(children_.begin() + idx + 1);
    }

    void InsertKeyAt(Key key, int idx) {
        keys_.insert(keys_.begin() + idx, key);
    }

    void InsertChildAt(Node<Key> * child, int idx) {
        children_.insert(children_.begin() + idx, child);
    }

    std::pair<Key, Node<Key> *> Split() {

        Node<Key> * new_child = new Node<Key>();

        int median_idx = NumKeys() / 2;
        Key median = keys_[median_idx];

        for (int i = median_idx + 1; i < keys_.size(); i++)
        {
            new_child->keys_.push_back(keys_[i]);
            new_child->children_.push_back(children_[i]);
        }
        new_child->children_.push_back(children_.back());

        int remove_count = keys_.size() - median_idx;
        for (int i = 0; i < remove_count; i++)
        {
            keys_.pop_back();
            children_.pop_back();
        }

        return {median, new_child};
    }

    Node() = default;

    explicit Node (Key key) {
        keys_ = {key};
        children_ = {nullptr, nullptr};
    }
};

template <class Key>
class BTree {

    const int kMaxDegree_;
    const int kMaxKeys_;
    const int kMinKeys_;
    Node<Key> * root_;
    int num_nodes_;
    int num_keys_;
    int height_;

    void RecursiveDelete(Node<Key> * root) {
        if (root == nullptr)
        {
            return;
        }

        for (const auto & child : root->children_)
        {
            RecursiveDelete(child);
        }

        delete root;
    }

    int FindKeysIdx(Node<Key> * root, Key key) {
        int idx = 0;
        while (idx < root->NumKeys() && root->keys_[idx] < key) idx++;
        return idx;
    }

    bool IsLeafNode(Node<Key> * root) {
        for (const auto & child : root->children_)
        {
            if (child != nullptr)
            {
                return false;
            }
        }
        return true;
    }

    Key InOrderSuccessor(Key key) {
        Node<Key> * itr = Find(key);

        int idx = FindKeysIdx(itr, key);
        itr = itr->children_[idx + 1];

        while (!IsLeafNode(itr))
        {
            itr = itr->children_.front();
        }

        return itr->keys_.front();
    }

    void FindAndReplace(Key to_replace, Key new_key) {
        Node<Key> * root = Find(to_replace);
        int idx = FindKeysIdx(root, to_replace);
        root->keys_[idx] = new_key;
    }

    Node<Key> * SearchNode(Node<Key> * root, Key key) {

        int idx = FindKeysIdx(root, key);

        if (idx < root->NumKeys() && root->keys_[idx] == key)
        {
            return root;
        }

        if (IsLeafNode(root))
        {
            return nullptr;
        }

        return SearchNode(root->children_[idx], key);
    }

    std::pair<Key, Node<Key> *> InsertAndSplit(Node<Key> * root, Key new_key, Node<Key> * new_child, int idx) {

        root->InsertKeyAt(new_key, idx);
        root->InsertChildAt(new_child, idx + 1);

        if (root->NumKeys() > kMaxKeys_)
        {
            num_nodes_++;
            return root->Split();
        }
        else
        {
            return {NULL, nullptr};
        }
    }

    std::pair<Key, Node<Key> *> RecursiveInsert(Node<Key> * root, Key key) {

        int idx = FindKeysIdx(root, key);

        if (idx < root->NumKeys() && root->keys_[idx] == key)
        {
            return {NULL, nullptr};
        }

        if (IsLeafNode(root))
        {
            return InsertAndSplit(root, key, nullptr, idx);
        }

        auto split_result = RecursiveInsert(root->children_[idx], key);
        Key new_key = split_result.first;
        Node<Key> * new_child = split_result.second;

        if (new_child != nullptr)
        {
            return InsertAndSplit(root, new_key, new_child, idx);
        }
        else
        {
            return split_result;
        }
    }

    void RotateLeft(Node<Key> * parent, int idx) {
        Node<Key> * r_sibling = parent->children_[idx + 1];
        Node<Key> * l_sibling = parent->children_[idx];
        Key separator = parent->keys_[idx];

        l_sibling->keys_.push_back(separator);
        l_sibling->children_.push_back(r_sibling->children_.front());

        parent->keys_[idx] = r_sibling->keys_.front();
        r_sibling->keys_.erase(r_sibling->keys_.begin());
        r_sibling->children_.erase(r_sibling->children_.begin());
    }

    void RotateRight(Node<Key> * parent, int idx) {
        Node<Key> * l_sibling = parent->children_[idx - 1];
        Node<Key> * r_sibling = parent->children_[idx];
        Key separator = parent->keys_[idx - 1];

        r_sibling->keys_.insert(r_sibling->keys_.begin(), separator);
        r_sibling->children_.insert(r_sibling->children_.begin(), l_sibling->children_.back());

        parent->keys_[idx - 1] = l_sibling->keys_.back();
        l_sibling->keys_.pop_back();
        l_sibling->children_.pop_back();
    }

    void ConcatenateNodes(Node<Key> * l_sibling, Node<Key> * r_sibling) {
        while (!r_sibling->keys_.empty())
        {
            Key front_key = r_sibling->keys_.front();
            Node<Key> * front_child = r_sibling->children_.front();

            l_sibling->keys_.push_back(front_key);
            l_sibling->children_.push_back(front_child);

            r_sibling->keys_.erase(r_sibling->keys_.begin());
            r_sibling->children_.erase(r_sibling->children_.begin());
        }
        Node<Key> * front_child = r_sibling->children_.front();
        l_sibling->children_.push_back(front_child);
    }

    void MergeSiblings(Node<Key> * parent, int idx) {
        Node<Key> * l_sibling;
        Node<Key> * r_sibling;
        Key separator;

        bool has_right_sibling = idx + 1 < parent->NumChildren();

        if (has_right_sibling)
        {
            l_sibling = parent->children_[idx];
            r_sibling = parent->children_[idx + 1];
            separator = parent->keys_[idx];
        }
        else
        {
            l_sibling = parent->children_[idx - 1];
            r_sibling = parent->children_[idx];
            separator = parent->keys_[idx - 1];
        }

        l_sibling->keys_.push_back(separator);

        ConcatenateNodes(l_sibling, r_sibling);

        int to_remove_idx = FindKeysIdx(parent, separator);
        parent->RemoveKeyAt(to_remove_idx);

        if (parent == root_ && parent->keys_.empty())
        {
            delete root_;
            root_ = l_sibling;

            num_nodes_--;
            height_--;
        }
    }

    void BalanceTree(Node<Key> * parent, int idx) {

        bool rotate_left = idx + 1 < parent->NumChildren() && parent->children_[idx + 1]->NumKeys() >  kMinKeys_;
        bool rotate_right = idx - 1 >= 0 && parent->children_[idx - 1]->NumKeys() > kMinKeys_;

        if (rotate_left)
        {
            RotateLeft(parent, idx);
        }
        else if (rotate_right)
        {
            RotateRight(parent, idx);
        }
        else
        {
            MergeSiblings(parent, idx);
        }
    }

    void BalanceIfNeeded(Node<Key> * parent, Node<Key> * root, int root_idx) {
        if (parent != nullptr && root->NumKeys() < kMinKeys_)
        {
            BalanceTree(parent, root_idx);
        }
    }

    void RecursiveRemove(Node<Key> * parent, Node<Key> * root, int root_idx, Key key) {

        int idx = FindKeysIdx(root, key);

        if (idx < root->NumKeys() && root->keys_[idx] == key)
        {
            root->RemoveKeyAt(idx);
            BalanceIfNeeded(parent, root, root_idx);
            return;
        }

        RecursiveRemove(root, root->children_[idx], idx, key);
        BalanceIfNeeded(parent, root, root_idx);
    }

public:

    explicit BTree(const int kMaxDegree) : kMaxDegree_(kMaxDegree), kMaxKeys_(kMaxDegree - 1), kMinKeys_(((kMaxDegree / 2) + (kMaxDegree % 2)) - 1) {

        if (kMaxDegree < 3)
        {
            throw std::invalid_argument("The B tree must have a maximum degree of at least 3.");
        }

        root_ = nullptr;
        num_nodes_ = 0;
        num_keys_ = 0;
        height_ = 0;
    }

    ~BTree() {
        Clear();
    }

    Node<Key> * Find(Key key) {
        if (Empty())
        {
            return nullptr;
        }
        else
        {
            return SearchNode(root_, key);
        }
    }

    void Insert(Key key) {
        if (Empty())
        {
            root_ = new Node<Key>(key);
            num_nodes_++;
        }
        else
        {
            auto split_result = RecursiveInsert(root_, key);
            Key new_key = split_result.first;
            Node<Key> * new_child = split_result.second;

            if (new_child != nullptr)
            {
                Node<Key> * new_root = new Node<Key>(new_key);

                std::vector<Node<Key> *> children = {root_, new_child};
                new_root->children_ = children;

                root_ = new_root;

                num_nodes_++;
                height_++;
            }
        }
        num_keys_++;
    }

    void Remove(Key key) {

        Node<Key> * to_remove_from = Find(key);

        if (to_remove_from == nullptr)
        {
            return;
        }
        else if (NumKeys() == 1)
        {
            delete root_;
            root_ = nullptr;
            num_nodes_--;
        }
        else if (IsLeafNode(to_remove_from))
        {
            RecursiveRemove(nullptr, root_, NULL, key);
        }
        else
        {
            Key new_separator = InOrderSuccessor(key);
            RecursiveRemove(nullptr, root_, NULL, new_separator);
            FindAndReplace(key, new_separator);
        }
        num_keys_--;
    }

    void Clear() {
        RecursiveDelete(root_);
        root_ = nullptr;
        num_nodes_ = 0;
        num_keys_ = 0;
        height_ = 0;
    }

    int NumNodes() {
        return num_nodes_;
    }

    int NumKeys() {
        return num_keys_;
    }

    int Height() {
        return height_;
    }

    bool Empty() {
        return num_keys_ == 0;
    }

    void PrintBFSTraversal() {

        if (Empty())
        {
            std::cout << "[]" << std::endl;
            return;
        }

        std::vector<std::vector<std::string>> tree_levels(height_ + 1);
        std::queue<std::tuple<Node<Key> *, Node<Key> *, int>> q;
        q.push({root_, nullptr, 0});

        while (!q.empty())
        {
            auto node_info = q.front();

            Node<Key> * node = std::get<0>(node_info);
            Node<Key> * parent = std::get<1>(node_info);
            int level = std::get<2>(node_info);

            q.pop();

            std::stringstream ss;
            ss << "[ID=" << node << " ParentID=" << parent << " Level=" << level << " Keys=(";
            for (const auto & key : node->keys_)
            {
                ss << key << " ";
            }
            ss << ")]";

            tree_levels[level].push_back(ss.str());

            for (const auto & child : node->children_)
            {
                if (child != nullptr)
                {
                    q.push({child, node, level + 1});
                }
            }
        }

        for (const auto & level : tree_levels)
        {
            for (const auto & node : level)
            {
                std::cout << node;
            }
            std::cout << std::endl;
        }
    }
};

#endif
