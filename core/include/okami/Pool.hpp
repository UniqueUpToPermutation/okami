#pragma once

#include <vector>
#include <unordered_map>
#include <stack>

namespace okami::core {

    // A pool that produces items that can be indexed by an id
    // Pointers to items in the pool are not stable and may change
    // whenever an item is added/removed from the pool.
    template <typename IdType, typename ObjectType>
    class Pool {
    private:
        struct Entry {
            ObjectType mObject;
            IdType mId;
        };

        std::vector<Entry> mItems;
        std::unordered_map<IdType, size_t> mIdToIndex;

    public:
        inline Pool(uint32_t reserveSize = 64) {
            mItems.reserve(reserveSize);
        }

        inline ObjectType& Alloc(IdType id) {
            mItems.resize(mItems.size() + 1);
            auto& item = mItems[mItems.size() - 1];
            item.mId = id;
            mIdToIndex.emplace(id, mItems.size() - 1);
            return item.mObject;
        }

        inline void ForEach(const std::function<void(IdType, ObjectType&)>& func) {
            for (auto& [id, idx] : mIdToIndex) {
                func(id, mItems[idx].mObject);
            }
        }

        inline void ForEach(const std::function<void(ObjectType&)>& func) {
            for (auto& item : mItems) {
                func(item);
            }
        }

        ObjectType Remove(IdType id) {
            auto it = mIdToIndex.find(id);

            if (it != mIdToIndex.end()) {
                auto idx = it->second;

                auto& toRemove = mItems[idx];
                auto& toSwap = mItems[mItems.size() - 1];

                ObjectType result = std::move(toRemove.mObject);

                mIdToIndex.erase(it);
                mIdToIndex[toSwap.mId] = idx;

                std::swap(toRemove, toSwap);

                mItems.pop_back();

                return result;
            } else {
                throw std::runtime_error("Id not in pool!");
            }
        }

        inline void Dealloc(IdType id) {
            Remove(id);
        }

        inline ObjectType& Add(IdType id, ObjectType&& item) {
            auto& res = Alloc(id);
            res = std::move(item);
            return res;
        }

        inline ObjectType& Get(IdType id) {
            return mItems[mIdToIndex[id]].mObject;
        }

        inline ObjectType* TryGet(IdType id) {
            auto it = mIdToIndex.find(id);

            if (it == mIdToIndex.end()) {
                return nullptr;
            } else {
                return &mItems[it->second].mObject;
            }
        }
    };

    // A pool that produces items that can be indexed by an id
    // Pointers to items in this pool are stable.
    template <typename IdType, typename ObjectType>
    class StablePool {
    private:
        std::unordered_map<IdType, 
            std::pair<uint32_t, uint32_t>> mIndices;
        std::stack<
            std::pair<uint32_t, uint32_t>> mFreeIndices;

        uint32_t mLastBlockOccupancy = 0;
        uint32_t mBlockSize;

        std::vector<std::vector<ObjectType>> mPoolBlocks;

    public:
        inline StablePool(uint32_t blockSize = 64) :
            mBlockSize(blockSize),
            mLastBlockOccupancy(blockSize) {
        }

        void ForEach(const std::function<void(IdType, ObjectType&)>& func) {
            for (auto& [id, idx] : mIndices) {
                func(id, mPoolBlocks[idx.first][idx.second]);
            }
        }

        ObjectType& Alloc(IdType id) {
            std::pair<uint32_t, uint32_t> indx;

            if (!mFreeIndices.empty()) {
                indx = mFreeIndices.top();
                mFreeIndices.pop();
            } else {
                if (mLastBlockOccupancy == mBlockSize) {
                    mLastBlockOccupancy = 0;
                    mPoolBlocks.emplace_back(std::vector<ObjectType>(mBlockSize));
                }

                indx = std::make_pair(
                    (uint32_t)(mPoolBlocks.size() - 1),
                    mLastBlockOccupancy++);
            }

            mIndices.emplace(id, indx);

            return mPoolBlocks[indx.first][indx.second];
        }

        inline ObjectType& Add(IdType id, ObjectType&& item) {
            auto& res = Alloc(id);
            res = std::move(item);
            return res;
        }

        ObjectType Remove(IdType id) {
            auto it = mIndices.find(id);
            if (it != mIndices.end()) {
                auto& obj = mPoolBlocks[it->second.first][it->second.second];
                mFreeIndices.emplace(it->second);
                mIndices.erase(it);
                return std::move(obj);
            } else {
                throw std::runtime_error("Id not in pool!");
            }
        }

        inline void Dealloc(IdType id) {
            Remove(id);
        }

        inline ObjectType& Get(IdType id) {
            auto idx = mIndices[id];
            return mPoolBlocks[idx.first][idx.second];
        }

        inline ObjectType* TryGet(resource_id_t id) {
            auto idx = mIndices.find(id);
            if (idx != mIndices.end()) {
                return mPoolBlocks[idx.first][idx.second];
            } else {
                return nullptr;
            }
        }
    };
}