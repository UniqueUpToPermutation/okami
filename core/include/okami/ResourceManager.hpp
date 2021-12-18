#pragma once

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
    using resource_finalize_delegate_t = std::function<void(T* resource)>;

    template <typename T>
    using resource_destroy_delegate_t = std::function<void(T* resource)>;

    template <typename T>
    struct ResourceLoadRequest {
        std::filesystem::path mPath;
        resource_id_t mId;
        Promise<Handle<T>> mPromise;
        LoadParams<T> mParams;
        resource_load_delegate_t<T> mLoader;
        resource_finalize_delegate_t<T> mFinalizer;
    };

    template <typename T>
    struct ResourceFinalizeRequest {
        bool bHasPath;
        std::filesystem::path mPath;
        resource_id_t mId;
        Promise<Handle<T>> mPromise;
        Handle<T> mHandle;
        resource_finalize_delegate_t<T> mFinalizer;
    };

    template <typename T>
    struct ResourcePostFinalize {
        resource_id_t mId;
        bool bHasPath;
        std::filesystem::path mPath;
        Handle<T> mHandle;
    };

    template <typename T>
    void ResourceDestructorWrapper(void* destroyer, Resource* obj);

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
        MessagePipe<T*> mDeleteQueueBackend;
        MessagePipe<resource_id_t> mDeleteQueueFrontend;

        // Used to queue up resources for loading
        MessagePipe<ResourceLoadRequest<T>> mLoadRequestsBackend;

        // Load to backend
        MessagePipe<ResourceFinalizeRequest<T>> mFinalizeRequestsBackend;
        
        // Assume ownership of these handles
        MessagePipe<Handle<T>> mAssumeOwnershipBackend;

        // Backend to frontend
        MessagePipe<ResourcePostFinalize<T>> mLoadsFinishedFrontend;

        resource_destroy_delegate_t<T> mDestroyer;

        // Resources that have already loaded. (Backend)
        typedef std::unordered_set<T*> set_t; 
        set_t mOwnedResources;

        // Various lookups so we don't load resources twice. (Frontend)
        marl::mutex mFrontendMutex;
        typedef std::unordered_map<std::filesystem::path, T*, PathHash> path_to_resource_t;

        path_to_resource_t mPathToResource;
        std::unordered_map<resource_id_t,
            typename path_to_resource_t::iterator> mIdToResource;
        std::unordered_map<std::filesystem::path,
            Future<Handle<T>>, PathHash> mPathToResourceLoading;


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
        ResourceManager(const resource_destroy_delegate_t<T>& destroyer) :
            mDestroyer(destroyer) {
        }

        // Should be run on main thread!
        void Shutdown() {
            auto value = bShutdownCalled.exchange(true);
            if (!value) {
                {
                    marl::lock lock(mFrontendMutex);

                    mPathToResource.clear();
                    mPathToResourceLoading.clear();
                    mIdToResource.clear();
                }

                // Wait until all active tasks have finished
                mTaskCounter.wait();

                // Run the backend to perform final cleanup
                RunBackend(true);
            }
        }

        ~ResourceManager() {
            Shutdown();
        }

        // Should be run on main thread!
        void RunBackend(bool bFinal = false) {
            marl::lock lock(mBackendMutex);

            if (!bFinal) {
                mLoadRequestsBackend.ConsumerCollect(false);

                while (!mLoadRequestsBackend.ConsumerIsEmpty()) {
                    ResourceLoadRequest<T>& requestRef = mLoadRequestsBackend.ConsumerFront();
    
                    marl::schedule([
                        request = requestRef,
                        &finalizePipe = mFinalizeRequestsBackend,
                        &loadFinishedPipe = mLoadsFinishedFrontend,
                        &assumeOwnershipPipe = mAssumeOwnershipBackend,
                        taskCounter = mTaskCounter] {
                        defer(taskCounter.done());
                        
                        // Load the resource...
                        T result = request.mLoader(request.mPath, request.mParams);
                        Handle<T> resultHandle(std::move(result));
                        
                        if (request.mFinalizer) {
                            // Then send it off to the main thread for finalization...
                            ResourceFinalizeRequest<T> finalizeRequest;
                            finalizeRequest.bHasPath = true;
                            finalizeRequest.mFinalizer = request.mFinalizer;
                            finalizeRequest.mId = request.mId;
                            finalizeRequest.mPath = request.mPath;
                            finalizeRequest.mPromise = std::move(request.mPromise);
                            finalizeRequest.mHandle = resultHandle;
                            finalizePipe.ProducerEnqueue(std::move(finalizeRequest));
                        } else {
                            // Let the front-end know a new resource has been loaded!
                            ResourcePostFinalize<T> message;
                            message.mHandle = resultHandle;
                            message.mId = request.mId;
                            message.mPath = request.mPath;
                            message.bHasPath = true;
                            loadFinishedPipe.ProducerEnqueue(std::move(message));
                            assumeOwnershipPipe.ProducerEnqueue(std::move(resultHandle));
                        }
                    });
                    mLoadRequestsBackend.ConsumerPop();
                }
            }

            if (bFinal) {
                // Wait for all loads in progress to finish...
                mTaskCounter.wait();
            }

            mFinalizeRequestsBackend.ConsumerCollect(bFinal);

            while (!mFinalizeRequestsBackend.ConsumerIsEmpty()) {
                ResourceFinalizeRequest<T>& request = mFinalizeRequestsBackend.ConsumerFront();

                // Run finalizer on main thread!
                request.mFinalizer(request.mHandle.Ptr());

                // Finally fulfill the promise!
                request.mPromise.Set(request.mHandle);

                // Let the front-end know a new resource has been loaded!
                ResourcePostFinalize<T> message;
                message.mHandle = request.mHandle;
                message.mId = request.mId;
                message.mPath = request.mPath;
                
                mLoadsFinishedFrontend.ProducerEnqueue(std::move(message));
                mFinalizeRequestsBackend.ConsumerPop();
            }

            mAssumeOwnershipBackend.ConsumerCollect(bFinal);

            while (!mAssumeOwnershipBackend.ConsumerIsEmpty()) {
                mOwnedResources.emplace(mAssumeOwnershipBackend.ConsumerFront().Ptr());
                mFinalizeRequestsBackend.ConsumerPop();
            }

            CollectGarbage(bFinal);
        }

        void OnDestroyed(T* t) {
            // Make sure both the front-end and backend are informed of the deletion!
            mDeleteQueueFrontend.ProducerEnqueue(t->Id());
            mDeleteQueueBackend.ProducerEnqueue(t);
        }

        Future<Handle<T>> Load(
            const std::filesystem::path& path,
            const LoadParams<T>& params,
            resource_id_t newId,
            const resource_load_delegate_t<T>& loader,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (!bShutdownCalled) {
                return Future<Handle<T>>();
            }

            Handle<T> resource = nullptr;
            Future<Handle<T>> future;
            ResourceLoadRequest<T> request;

            bool bQueueLoad = false;

            {
                marl::lock lock(mFrontendMutex);

                mLoadsFinishedFrontend.ConsumerTryCollect();

                // Update the lookups with the finished loads
                while (!mLoadsFinishedFrontend.ConsumerIsEmpty()) {
                    auto& load = mLoadsFinishedFrontend.ConsumerFront();

                    auto it = mPathToResource.emplace(load.mPath, load.mHandle.Ptr());
                    mIdToResource[load.mId] = it.first;

                    mLoadsFinishedFrontend.ConsumerPop();
                }

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

                // Check if resource has been loaded
                if (it != mPathToResource.end()) {
                    resource = it->second;
                } else {

                    // Check if resource is currently loading
                    auto it2 = mPathToResourceLoading.find(path);

                    if (it2 != mPathToResourceLoading.end()) {
                        future = it2->second;
                    } else {

                        // Queue a new load if item is niether loaded or loading
                        bQueueLoad = true;
                        auto emplaceIt = mPathToResourceLoading.emplace(path, request.mPromise);
                    }
                }
            }

            if (resource) {
                request.mPromise.Set(resource);
                return request.mPromise;
            }

            if (bQueueLoad) {
                future = request.mPromise;
                {
                    request.mId = newId;
                    request.mParams = params;
                    request.mPath = path;
                    request.mLoader = loader;
                    request.mFinalizer = finalizer;
                    mLoadRequestsBackend.ProducerEnqueue(std::move(request));
                }
            }

            return future;
        }

        Future<Handle<T>> Add(T&& object,
            resource_id_t newId,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (!bShutdownCalled) {
                return Future<Handle<T>>();
            }

            Handle<T> handle(std::move(object));

            // Make sure that the destruction of this resource is reported to the manager
            handle->SetDestructor(&ResourceDestructorWrapper<T>, this);

            if (finalizer) {
                ResourceFinalizeRequest<T> request;
                request.mId = newId;
                request.bHasPath = false;
                request.mHandle = handle;
                request.mFinalizer = finalizer;

                Future<Handle<T>> future = request.mPromise;

                // Tell the backend to finalize this resource
                mFinalizeRequestsBackend.ProducerEnqueue(std::move(request));

                return future;
            } else {
                Handle<T> handleCopy = handle;
                mAssumeOwnershipBackend.ProducerEnqueue(std::move(handle));
                return Promise<Handle<T>>(std::move(handleCopy));
            }
        }

        Future<Handle<T>> Add(T&& object,
            const std::filesystem::path& path,
            resource_id_t newId,
            const resource_finalize_delegate_t<T>& finalizer) {

            if (!bShutdownCalled) {
                return Future<Handle<T>>();
            }

            Handle<T> handle(std::move(object));

            // Make sure that the destruction of this resource is reported to the manager
            handle->SetDestructor(&ResourceDestructorWrapper<T>, this);

            if (finalizer) {
                ResourceFinalizeRequest<T> request;
                request.mId = newId;
                request.bHasPath = true;
                request.mPath = path;
                request.mHandle = handle;
                request.mFinalizer = finalizer;

                Future<Handle<T>> future = request.mPromise;

                {
                    marl::lock lock(mFrontendMutex);
                    mPathToResourceLoading.emplace(path, request.mPromise);
                }

                // Tell the backend to finalize this resource
                mFinalizeRequestsBackend.ProducerEnqueue(std::move(request));

                return future;
            } else {
                
                {    
                    marl::lock lock(mFrontendMutex);
                    auto emplaceIt = mPathToResource.emplace(path, handle.Ptr());
                    mIdToResource[newId] = emplaceIt.first;
                }

                Handle<T> handleCopy = handle;
                mAssumeOwnershipBackend.ProducerEnqueue(std::move(handle));

                return Promise<Handle<T>>(std::move(handleCopy));
            }
        }
    };

    template <typename T>
    void ResourceDestructorWrapper(void* destroyer, Resource* obj) {
        T* obj_ = dynamic_cast<T*>(obj);
        ResourceManager<T>* destroyer_ = reinterpret_cast<ResourceManager<T>*>(destroyer);
        destroyer_->OnDestroyed(obj_);
    }
}