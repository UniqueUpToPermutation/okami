#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/Hashers.hpp>

#include <marl/waitgroup.h>
#include <marl/defer.h>

namespace okami::core {

    template <typename T>
    struct ResourceEntry {
        bool bHasPath;
        std::filesystem::path mPath;
        resource_id_t mId;
        std::unique_ptr<T> mResource;

        bool inline operator==(const ResourceEntry<T>& other) {
            return mId == other.mId;
        }

        struct Hasher {
            std::size_t operator()(const ResourceEntry& k) const {
                return (std::hash<resource_id_t>()(k.mId));
            }
        };
    };

    template <typename frontendT>
    using resource_load_delegate_t = std::function<frontendT(
        const std::filesystem::path& path,
        const LoadParams<frontendT>& params)>;

    template <typename frontendT, typename backendT>
    using resource_finalize_delegate_t = std::function<
        void(const frontendT& frontendIn, 
            frontendT& frontendOut, 
            backendT& backend)>;

    template <typename backendT>
    using resource_destroy_delegate_t = std::function<
        void(backendT& resource)>;

    template <typename frontendT, typename backendT>
    using resource_construct_backend_delegate_t = std::function<
        backendT(const frontendT&)>;

    template <typename frontendT>
    struct ResourceLoadRequest {
        std::filesystem::path mPath;
        LoadParams<frontendT> mParams;
        frontendT* mFrontend;
        resource_id_t mId;
    };

    template <typename frontendT>
    struct ResourceFinalizeRequest {
        std::unique_ptr<frontendT> mFrontendProxy;
        frontendT* mFrontend;
        resource_id_t mId;
    };

    /*
        The ResourceBackend should be run on the main thread.
        However, it will spawn loading tasks on different threads.
    */
    template <typename frontendT, typename backendT>
    class ResourceBackend : public IResourceBackend<frontendT> {
    private:
        std::atomic<bool> bShutdownCalled = false;
        std::atomic<uint> mPendingLoads = 0;
        std::atomic<uint> mPendingFinalizes = 0;
        marl::WaitGroup mTaskCounter;

        // A queue of items set to be deleted.
        std::set<resource_id_t> mDeletionQueue;

        // Used to queue up resources for loading
        std::queue<ResourceLoadRequest<frontendT>> mLoadRequests;

        // Used to queue up resources for finalization (move to GPU)
        MessagePipe<ResourceFinalizeRequest<frontendT>> mFinalizeRequests;
        
        resource_construct_backend_delegate_t<frontendT, backendT> mConstructor;
        resource_load_delegate_t<frontendT> mLoader;
        resource_finalize_delegate_t<frontendT, backendT> mFinalizer;
        resource_destroy_delegate_t<backendT> mDestroyer;

        Pool<resource_id_t, backendT> mPool;
        std::unordered_map<resource_id_t, backendT&> mIdToResource;

    public:
        ResourceBackend(
            resource_construct_backend_delegate_t<frontendT, backendT> constructor,
            resource_load_delegate_t<frontendT> loader,
            resource_finalize_delegate_t<frontendT, backendT> finalizer,
            resource_destroy_delegate_t<backendT> destroyer) :
            mConstructor(constructor),
            mLoader(loader),
            mFinalizer(finalizer),
            mDestroyer(destroyer) {
        }

        inline void ForEach(const std::function<void(backendT&)>& func) {
            mPool.ForEach(func);
        }

        inline void ForEach(const std::function<void(resource_id_t, backendT&)>& func) {
            mPool.ForEach(func);
        }

        void Run() {
            // Run deletion
            for (auto id : mDeletionQueue) {
                auto& resource = mPool.Get(id);
                mDestroyer(resource);
                mPool.Dealloc(id);
            }

            mDeletionQueue.clear();
            mFinalizeRequests.ConsumerCollect();

            while (!mFinalizeRequests.ConsumerIsEmpty()) {
                ResourceFinalizeRequest<frontendT>& msg = 
                    mFinalizeRequests.ConsumerFront();

                auto target = mPool.TryGet(msg.mId);

                // Target hasn't been disposed of yet.
                if (target) {
                    if (msg.mFrontendProxy)
                        mFinalizer(*msg.mFrontendProxy, *msg.mFrontend, *target);
                    else 
                        mFinalizer(*msg.mFrontend, *msg.mFrontend, *target);
                }

                mFinalizeRequests.ConsumerPop();
                --mPendingFinalizes;
            }

            while (!mLoadRequests.empty()) {
                ResourceLoadRequest<frontendT>& loadRequest = 
                    mLoadRequests.front();

                mPendingLoads++;
                mTaskCounter.add();
                marl::schedule([
                    &pendingTasks = mPendingLoads,
                    &pendingFinalizes = mPendingFinalizes,
                    taskCounter = mTaskCounter,
                    request = loadRequest,
                    &finalizePipe = mFinalizeRequests,
                    loader = mLoader]() {
                    defer(taskCounter.done());
                    defer(pendingTasks--);

                    ResourceFinalizeRequest<frontendT> msg;

                    msg.mFrontend = request.mFrontend;
                    msg.mFrontendProxy = std::make_unique<frontendT>
                        (loader(request.mPath, request.mParams));
                    msg.mId = request.mId;

                    ++pendingFinalizes;
                    finalizePipe.ProducerEnqueue(std::move(msg));
                });

                mLoadRequests.pop();
            }
        }

        inline bool IsIdle() {
            return mLoadRequests.size() + 
                mPendingLoads +
                mPendingFinalizes +
                mDeletionQueue.size() == 0;
        }

        inline marl::WaitGroup& LoadCounter() {
            return mTaskCounter;
        }

        inline backendT* TryGet(resource_id_t id) {
            return mPool.TryGet(id);
        }

        inline backendT& Get(resource_id_t id) {
            return mPool.Get(id);
        }

        // Delete everything that should be destroyed.
        void Shutdown() {
            bShutdownCalled = true;
            mTaskCounter.wait();
            Run();
        }

        // Implementation should be thread safe!
        void NotifyAdd(resource_id_t id, frontendT& frontend) override {
            auto& backend = mPool.Alloc(id);
            backend = mConstructor(frontend);

            if (frontend.HasLoadParams()) {
                ResourceLoadRequest<frontendT> msg;
                msg.mId = id;
                msg.mParams = frontend.GetLoadParams();
                msg.mPath = frontend.GetPath();
                msg.mFrontend = &frontend;

                mLoadRequests.emplace(std::move(msg));
            } else {
                ResourceFinalizeRequest<frontendT> msg;
                msg.mFrontend = &frontend;
                msg.mFrontendProxy = nullptr;
                msg.mId = id;

                mPendingFinalizes++;
                mFinalizeRequests.ProducerEnqueue(std::move(msg));
            }
        }

        void NotifyDestroy(resource_id_t id, frontendT& frontend) override {
            mDeletionQueue.emplace(id);
        }
    };
}