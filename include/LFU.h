#pragma once

#include <unordered_map>
#include <cmath>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

#include "ICachePolicy.h"

namespace bananaWebServer
{
template<typename Key, typename Value> class LFUCache;

template<typename Key, typename Value>
class FreqList
{
private:
    struct Node
    {
        int freq;
        Key key;
        Value value;
        std::weak_ptr<Node> pre;
        std::shared_ptr<Node> next;
        Node() : freq(1),next(nullptr){};
        Node(Key k,Value v) : freq(1),key(k),value(v) {}; 
    };

    int freq_;
    using NodePtr = std::shared_ptr<Node>;
    NodePtr head_;
    NodePtr tail_;
    
public:
    explicit FreqList(int freq) : freq_(freq)
    {
        head_ = std::make_shared<Node>();
        tail_ = std::make_shared<Node>();
        head_->next = tail_;
        tail_->pre = head_;
    }

    bool isEmpty()
    {
        return head_->next == tail_;
    }

    void addNode(NodePtr node)
    {
        if(!node || !head_ || !tail_)
        {
            return ;
        }
        node->next = tail_;
        node->pre = tail_->pre;
        tail_->pre.lock()->next = node;
        tail_->pre = node;
    }

    void removeNode(NodePtr node)
    {
        if(!node || !head_ || !tail_) return ;
        if(node->pre.expired() || !node->next) return ;
        auto pre = node->pre.lock();
        pre->next = node->next;
        node->next->pre = pre;
        node->next = nullptr;
    }

    NodePtr getFirstNode() { return head_->next; };

    friend class LFUCache<Key,Value>;
};

template<typename Key, typename Value>
class LFUCache : public ICachePolicy<Key,Value>
{
public:
    using Node = typename FreqList<Key,Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key,NodePtr>;

    LFUCache(int capacity, int maxAverageNum = 10) : capacity_(capacity), maxAverageNum_(maxAverageNum), minFreq_(INT8_MAX), curAverageNum_(0), curTotalNum_(0)
    {}

    ~LFUCache() override = default;

    void put(Key key,Value value) override
    {
        if (capacity_ == 0)
        {
            return ;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if(it != nodeMap_.end())
        {
            it->second->value = value;
            getInternal(it->second,value);
            return ;
        }
        putInternal(key,value);
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if(it != nodeMap_.end())
        {
            getInternal(it->second,value);
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key,value);
        return value;
    }

    void purge()
    {
      nodeMap_.clear();
      freqToFreqList_.clear();
    }
private:
    void putInternal(Key key, Value value);
    void getInternal(NodePtr node, Value& value);

    void addToFreqList(NodePtr node);
    void removeFromFreqList(NodePtr node);
    void kickOut();

    void increaceFreqNum();
    void decreaceFreqNum(int num);
    void handleOverMaxAverageNum();
    void updateMinFreq();

private:
    int                                            capacity_; // 缓存容量
    int                                            minFreq_; // 最小访问频次(用于找到最小访问频次结点)
    int                                            maxAverageNum_; // 最大平均访问频次
    int                                            curAverageNum_; // 当前平均访问频次
    int                                            curTotalNum_; // 当前访问所有缓存次数总数 
    std::mutex                                     mutex_; // 互斥锁
    NodeMap                                        nodeMap_; // key 到 缓存节点的映射
    std::unordered_map<int, FreqList<Key, Value>*> freqToFreqList_;// 访问频次到该频次链表的映射
};

template <typename Key, typename Value>
void LFUCache<Key, Value>::putInternal(Key key, Value value)
{
    if(nodeMap_.size() == capacity_)
    {
        kickOut();
    }
    NodePtr node = std::make_shared<Node>(key,value);
    nodeMap_[key] = node;
    addToFreqList(node);
    increaceFreqNum();
    minFreq_ = std::min(1,minFreq_);
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::getInternal(NodePtr node, Value &value)
{
    value = node->value;
    removeFromFreqList(node);
    node->freq++;
    addToFreqList(node);
    if(node->freq-- == minFreq_ && freqToFreqList_[node->freq]->isEmpty())
    {
        minFreq_++;
    }
    increaceFreqNum();
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::addToFreqList(NodePtr node)
{
    if(!node)
    {
        return ;
    }
    auto freq = node->freq;
    if(freqToFreqList_.find(freq) == freqToFreqList_.end())
    {
        freqToFreqList_[freq] = new FreqList<Key,Value>(freq);
    }
    freqToFreqList_[freq]->addNode(node);
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::removeFromFreqList(NodePtr node)
{
    if(!node)
    {
        return;
    }
    auto freq = node->freq;
    freqToFreqList_[freq]->removeNode(node);
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::kickOut()
{
    NodePtr node = freqToFreqList_[minFreq_]->getFirstNode();
    removeFromFreqList(node);
    nodeMap_.erase(node->key);
    decreaceFreqNum(node->freq);
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::increaceFreqNum()
{
    curTotalNum_++;
    if(nodeMap_.empty())
    {
        curAverageNum_ = 0;
    }
    else
    {
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
    }
    if(curAverageNum_ > maxAverageNum_)
    {
        handleOverMaxAverageNum();
    }
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::decreaceFreqNum(int num)
{
    curTotalNum_ -= num;
    if(nodeMap_.empty())
    {
        curAverageNum_ = 0;
    }
    else
    {
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
    }
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::handleOverMaxAverageNum()
{   
    if(nodeMap_.empty())
    {
        return ;
    }

    for(auto it = nodeMap_.begin();it != nodeMap_.end();it++)
    {
        if(!it->second)
        {
            continue;
        }
        NodePtr node = it->second;
        removeFromFreqList(it->second);
        it->second->freq -= maxAverageNum_/2;
        if (node->freq < 1) node->freq = 1;
        addToFreqList(it->second);
    }
    updateMinFreq();
}


template <typename Key, typename Value>
void LFUCache<Key, Value>::updateMinFreq()
{
    minFreq_ = INT8_MAX;
    for(auto& pair : freqToFreqList_)
    {
        if(pair.second && !pair.second->isEmpty())
        {
            minFreq_ = std::min(minFreq_,pair.first);
        }
    }
    if(minFreq_ == INT8_MAX)
    {
        minFreq_ = 1;
    }
}


template<typename Key, typename Value>
class HashLFUCache
{
public:
    HashLFUCache(size_t capacity, int sliceNum, int maxAverageNum = 10) : capacity_(capacity), sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
    {
        size_t sliceSize = std::ceil(capacity / static_cast<double>(sliceNum));
        for (int i = 0; i < sliceNum; i++)
        {
            lfuCacheSlices.push_back(new LFUCache<Key,Value>(capacity,maxAverageNum));
        }
    }

    void put(Key key, Value value)
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lfuCacheSlices[sliceIndex]->put(key,value);
    }

    bool get(Key key, Value& value)
    {
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lfuCacheSlices[sliceIndex]->get(key,value);
    }

    Value get(Key key)
    {
        Value value{};
        get(key,value);
        return value;
    }

    void purge()
    {
        for (auto& lfuSliceCache : lfuCacheSlices)
        {
            lfuSliceCache->purge();
        }
    }
private:
    // 将key计算成对应哈希值
    size_t Hash(Key key)
    {
        std::hash<Key> hashFunc;
        return hashFunc(key);
    }

private:
    size_t capacity_;
    int sliceNum_;
    std::vector<std::unique_ptr<LFUCache<Key,Value>>> lfuCacheSlices;
};
} // namespace bananaWebServer
