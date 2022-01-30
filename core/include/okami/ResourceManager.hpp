#pragma once

#include <okami/PlatformDefs.hpp>
#include <okami/ResourceInterface.hpp>
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

    template <typename T>
    using resource_load_delegate_t = std::function<T(
        const std::filesystem::path& path,
        const LoadParams<T>& params)>;

    template <typename T>
    using resource_finalize_delegate_t = std::function<void(WeakHandle<T> resource)>;

    template <typename T>
    using resource_destroy_delegate_t = std::function<void(WeakHandle<T> resource)>;

    template <typename T>
    using resource_construct_delegate_t = std::function<T()>;

    template <typename T>
    struct ResourceLoadRequest {
        std::filesystem::path mPath;
        resource_id_t mId;
        Handle<T> mResource;
        LoadParams<T> mParams;
        resource_load_delegate_t<T> mLoader;
        resource_finalize_delegate_t<T> mFinalizer;
    };

    template <typename T>
    struct ResourceFinalizeRequest {
        bool bHasPath;
        std::filesystem::path mPath;
        resource_id_t mId;
        Handle<T> mHandle;
        resource_finalize_delegate_t<T> mFinalizer;
    };

    template <typename T>
    void ResourceDestructorWrapper(void* destroyer, WeakHandle<Resource> obj);

    /*
        The ResourceManager is a thread-safe way of managing the lifetime and usage
        of resources. The ResourceManager has both a front-end and a back-end. The
        front-end can be called from any thread, while the back-end must always be
        called from only a single thread. 

        The ResourceManager expect you to provide three functions for:
            Loading: This proceedure should load a resource into memory on the CPU.
                May be called from any thread.
            Finalization: This proceedure should take a resource in CPU memory and
                move it to GPU memory (if necessary). Is this is a CPU resource,
                then this should do nothing. Finalization is gauranteed to be
                called from the main thread.
            Destruction: This proceedure should take a resource and destroy both
                the front and back-ends of the resource.
    */
    template <typename T>
    class ResourceManager {
    private:
        std::atomic<bool> bShutdownCalled = false;
        marl::WaitGroup mTaskCounter;

        marl::mutex mBackendMutex;

        // Used to queue up resources for deletion (called by Release())
        MessagePipe<WeakHandle<T>> mDeleteQueueBackend;
        MessagePipe<resource_id_t> mDeleteQueueFrontend;

        // Used to queue up resources for loading
        MessagePipe<ResourceLoadRequest<T>> mLoadRequestsBackend;

        // Used to queue up resources for finalization (move to GPU)
        MessagePipe<ResourceFinalizeRequest<T>> mFinalizeRequestsBackend;
        
        resource_construct_delegate_t<T> mConstructor;
        resource_destroy_delegate_t<T> mDestroyer;

        // Resources that have already loaded. (Backend)
        typedef std::unordered_set<WeakHandle<T>,
            typename WeakHandle<T>::Hasher> set_t; 
        set_t mOwnedResources;

        // Various lookups so we don't load resources twice. (Frontend)
        marl::mutex mFrontendMutex;
        typedef std::unordered_map<std::filesystem::path, 
            WeakHandle<T>, PathHash> path_to_resource_t;

        path_to_resource_t mPathToResource;
        std::unordered_map<resource_id_t,
            typename path_to_resource_t::iterator> mIdToResource;

        void CollectGarbage(bool bBlock = false) {
            mDeleteQueueBackend.ConsumerCollect(bBlock);

            while (!mDeleteQueueBackend.ConsumerIsEmpty()) {
                auto& resource = mDeleteQueueBackend.ConsumerFront();
                mOwnedResources.erase(resource);
                mDestroyer(resource);
                mDeleteQueueBackend.ConsumerPop();
            }
        }

    public:
        ResourceManager(
            const resource_construct_delegate_t<T>& constructor,
            const resource_destroy_delegate_t<T>& destroyer) :
            mConstructor(constructor),
            mDestroyer(destroyer) {
        }

        // Should be run on main thread!
        void Shutdown() {
            auto value = bShutdownCalled.exchange(true);
            if (!value) {
                {
                    marl::lock lock(mFrontendMutex);

                    mPathToResource.clear();
                    mIdToResource.clear();
                }

                // Run the backend
                RunBackend(true);

                // Queue all owned resources up for destruction
                bool bUncollectedResources = false;
                for (auto resource : mOwnedResources) {
                    mDeleteQueueBackend.ProducerEnqueue(resource);
                    mDeleteQueueFrontend.ProducerEnqueue(resource->Id());
                    bUncollectedResources = true;
                }

                // Collect all garbage
                if (bUncollectedResources) {
                    CollectGarbage(true);
                }
            }
        }

        ~ResourceManager() {
            Shutdown();
        }

        void ScheduleBackend(marl::WaitGroup group, bool bBlock = false) {
            group.add();
            marl::Task managerUpdates([this, bBlock, group] {
                defer(group.done());
                RunBackend(bBlock);
            }, marl::Task::Flags::SameThread);
            marl::schedule(std::move(managerUpdates));
        }

        // Should be run on main thread!
        void RunBackend(bool bBlock = false) {
            marl::lock lock(mBackendMutex);

            mLoadRequestsBackend.ConsumerCollect(bBlock);

            while (!mLoadRequestsBackend.ConsumerIsEmpty()) {
                ResourceLoadRequest<T>& requestRef = mLoadRequestsBackend.ConsumerFront();

                mTaskCounter.add();
                marl::schedule([
                    request = requestRef,
                    &finalizePipe = mFinalizeRequestsBackend,
                    taskCounter = mTaskCounter] {
                    defer(taskCounter.done());
                    
                    // Load the resource...
                    *request.mResource = request.mLoader(request.mPath, request.mParams);
                    
                    // Then send it off to the main thread for finalization...
                    ResourceFinalizeRequest<T> finalizeRequest;
                    finalizeRequest.bHasPath = true;
                    finalizeRequest.mFinalizer = request.mFinalizer;
                    finalizeRequest.mId = request.mId;
                    finalizeRequest.mPath = request.mPath;
                    finalizeRequest.mHandle = request.mResource;
                    finalizePipe.ProducerEnqueue(std::move(finalizeRequest));
                });
                mLoadRequestsBackend.ConsumerPop();
            }

            if (bBlock) {
                // Wait for all loads in progress to finish...
                mTaskCounter.wait();
            }

            mFinalizeRequestsBackend.ConsumerCollect(bBlock);

            while (!mFinalizeRequestsBackend.ConsumerIsEmpty()) {
                ResourceFinalizeRequest<T>& request = mFinalizeRequestsBackend.ConsumerFront();

                // Run finalizer on main thread!
                if (request.mFinalizer)
                    request.mFinalizer(request.mHandle);

                mOwnedResources.emplace(request.mHandle);

                // Let everyone know this resource is ready to be used.
                request.mHandle->OnLoadEvent().signal();

                mFinalizeRequestsBackend.ConsumerPop();
            }

            // Collect garbage
            CollectGarbage(bBlock);
        }

        void OnDestroyed(WeakHandle<T> t) {
            // Make sure both the front-end and backend are informed of the deletion!
            mDeleteQueueFrontend.ProducerEnqueue(t->Id());
            mDeleteQueueBackend.ProducerEnqueue(t);
        }

        Handle<T> Load(
            const std::filesystem::path& path,
            const LoadParams<T>& params,
            resource_id_t newId,
            const resource_load_delegate_t<T>& loader,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (bShutdownCalled) {
                return nullptr;
            }

            Handle<T> resource = nullptr;
            ResourceLoadRequest<T> request;

            bool bQueueLoad = false;

            {
                marl::lock lock(mFrontendMutex);

                // Force the collection of any deletion messages
                if (mDeleteQueueFrontend.HasMessages()) {
                    mDeleteQueueFrontend.ConsumerCollect();
                }

                // Update the lookups with any deleted items
                while (!mDeleteQueueFrontend.ConsumerIsEmpty()) {
                    auto toDelete = mDeleteQueueFrontend.ConsumerFront();

                    auto it = mIdToResource.find(toDelete);

                    if (it != mIdToResource.end()) {
                        mPathToResource.erase(it->second);
                        mIdToResource.erase(it);
                    }

                    mDeleteQueueFrontend.ConsumerPop();
                }

                // Find the resource
                auto it = mPathToResource.find(path);

                if (it != mPathToResource.end()) {
                    resource = it->second;
                } else {
                    // Queue a new load if item is niether loaded or loading
                    bQueueLoad = true;
                    resource = Handle<T>(mConstructor(), this, &ResourceDestructorWrapper<T>);
                    auto emplace_it = mPathToResource.emplace_hint(it, path, resource);
                    mIdToResource.emplace(newId, emplace_it);
                }
            }

            if (bQueueLoad) {
                request.mId = newId;
                request.mParams = params;
                request.mPath = path;
                request.mLoader = loader;
                request.mFinalizer = finalizer;
                request.mResource = resource;
                mLoadRequestsBackend.ProducerEnqueue(std::move(request));
            }

            return resource;
        }

        Handle<T> Add(T&& object,
            resource_id_t newId,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (bShutdownCalled) {
                return nullptr;
            }

            // Make sure that the destruction of this resource is reported to the manager
            Handle<T> handle(std::move(object), this, &ResourceDestructorWrapper<T>);

            ResourceFinalizeRequest<T> request;
            request.mId = newId;
            request.bHasPath = false;
            request.mHandle = handle;
            request.mFinalizer = finalizer;

            // Tell the backend to finalize this resource
            mFinalizeRequestsBackend.ProducerEnqueue(std::move(request));

            return handle;
        }

        Handle<T> Add(T&& object,
            const std::filesystem::path& path,
            resource_id_t newId,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (bShutdownCalled) {
                return nullptr;
            }

            // Make sure that the destruction of this resource is reported to the manager
            Handle<T> handle(std::move(object), this, &ResourceDestructorWrapper<T>);

            ResourceFinalizeRequest<T> request;
            request.mId = newId;
            request.bHasPath = true;
            request.mPath = path;
            request.mHandle = handle;
            request.mFinalizer = finalizer;

            {
                marl::lock lock(mFrontendMutex);
                auto it = mPathToResource.emplace(path, handle);
                mIdToResource.emplace(newId, it.first);
            }

            // Tell the backend to finalize this resource
            mFinalizeRequestsBackend.ProducerEnqueue(std::move(request));

            return handle;
        }
    };

    template <typename T>
    void ResourceDestructorWrapper(void* destroyer, WeakHandle<Resource> obj) {
        WeakHandle<T> objCast = obj.TryCast<T>();
        ResourceManager<T>* destroyer_ = reinterpret_cast<ResourceManager<T>*>(destroyer);
        destroyer_->OnDestroyed(objCast);
    }
}