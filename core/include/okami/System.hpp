#pragma once 

#include <entt/entt.hpp>
#include <marl/waitgroup.h>
#include <marl/event.h>
#include <marl/defer.h>

#include <okami/PlatformDefs.hpp>
#include <okami/Frame.hpp>
#include <okami/Hashers.hpp>
#include <okami/Clock.hpp>

namespace okami::core {
    class SyncObject;

    class Frame;

    class InterfaceCollection;

    class ISystem {
    public:
        // Initialize the system and load all required resources.
        virtual void Startup(marl::WaitGroup& waitGroup) = 0;

        // Register all interfaces to the specified interface collection
        virtual void RegisterInterfaces(InterfaceCollection& interfaces) = 0;
        
        // Destroy the system and free all used resources.
        virtual void Shutdown() = 0;

        // Load all underlying resources for the specified frame.
        virtual void LoadResources(marl::WaitGroup& waitGroup) = 0;

        // Set the current frame of the system.
        virtual void SetFrame(Frame& frame) = 0;

        // Called at the beginning of every frame execution before Fork.
        // Use this to request reads and writes to different component types.
        virtual void RequestSync(SyncObject& syncObject) = 0;

        // Begin execution of this system's update proceedure.
        // Gauranteed to be called from the main thread.
        // You should offload work into marl tasks as opposed
        // to performing operations in the function definition.
        virtual void Fork(Frame& frame,
            SyncObject& syncObject,
            const Time& time) = 0;

        // Ends the execution of this system's update proceedure.
        // Gauranteed to be called from the main thread.
        // Should block until the proceedures invoked from Fork are finished
        virtual void Join(Frame& frame) = 0;

        // Waits on this system to join
        virtual void Wait() = 0;

        virtual ~ISystem() = default;
    };

    class InterfaceCollection {
    private:
        std::unordered_map<entt::meta_type, void*, TypeHash> mInterfaces;
    
    public:
        template <typename T>
        inline void Add(T* t) {
            mInterfaces.emplace(entt::resolve<T>(), t);
        }

        template <typename T>
        inline T* Query() {
            auto it = mInterfaces.find(entt::resolve<T>());
            if (it == mInterfaces.end()) {
                return nullptr;
            } else {
                return reinterpret_cast<T*>(it->second);
            }
        }

        InterfaceCollection() = default;
        inline InterfaceCollection(ISystem* system) {
            system->RegisterInterfaces(*this);
        }
    };

    struct WaitHandle {
        marl::WaitGroup* mBefore = nullptr;
        marl::WaitGroup* mAfter = nullptr;
        marl::mutex* mWriteMutex = nullptr;
        bool bFinished = true;

        WaitHandle() = default;

        inline WaitHandle(marl::WaitGroup& after) : 
            mAfter(&after), bFinished(false) {
        }

        inline WaitHandle(marl::WaitGroup& before, marl::WaitGroup& after, marl::mutex& mutex) :
            mBefore(&before), mAfter(&after), mWriteMutex(&mutex) {
        }

        WaitHandle(const WaitHandle&) = delete;
        WaitHandle(WaitHandle&& other) = default;

        WaitHandle& operator=(const WaitHandle&) = delete;
        WaitHandle& operator=(WaitHandle&& other) = default;

        inline void Release() {
            if (!bFinished) {
                mAfter->done();
                bFinished = true;
            }
        }

        inline bool IsRead() const {
            return mWriteMutex == nullptr;
        }

        inline bool IsWrite() const {
            return mWriteMutex != nullptr;
        }
    };

    class SyncObject {
    private:
        std::unordered_map<entt::meta_type, 
            marl::WaitGroup, TypeHash> mReadWaits;
        std::unordered_map<entt::meta_type, 
            std::unique_ptr<marl::mutex>, TypeHash> mWriteMutexes;
        std::unordered_map<entt::meta_type,
            marl::WaitGroup, TypeHash> mWriteWaits;

    public:
        SyncObject() = default;
        SyncObject(SyncObject&&) = default;
        SyncObject& operator=(SyncObject&&) = default;
        SyncObject(const SyncObject&) = delete;
        SyncObject& operator=(const SyncObject&) = delete;

        marl::WaitGroup* ReadWaitGroup(const entt::meta_type& type, bool bCreateIfNotFound = false) {
            auto it = mReadWaits.find(type);

            if (it == mReadWaits.end()) {
                if (bCreateIfNotFound) {
                    auto waitGroup = marl::WaitGroup();
                    it = mReadWaits.emplace_hint(it, 
                        std::pair<entt::meta_type, marl::WaitGroup>(type, std::move(waitGroup)));
                } else {
                    return nullptr;
                }
            }

            return &it->second;
        }

        marl::WaitGroup* WriteWaitGroup(const entt::meta_type& type, bool bCreateIfNotFound = false) {
            auto it = mWriteWaits.find(type);

            if (it == mWriteWaits.end()) {
                if (bCreateIfNotFound) {
                    auto waitGroup = marl::WaitGroup();
                    it = mWriteWaits.emplace_hint(it, 
                        std::pair<entt::meta_type, marl::WaitGroup>(type, std::move(waitGroup)));
                } else {
                    return nullptr;
                }
            }

            return &it->second;
        }

        marl::mutex* WriteMutex(const entt::meta_type& type, bool bCreateIfNotFound = false) {
            auto it = mWriteMutexes.find(type);

            if (it == mWriteMutexes.end()) {
                if (bCreateIfNotFound) {
                    std::pair<entt::meta_type, std::unique_ptr<marl::mutex>> mutexPair(type, 
                        std::make_unique<marl::mutex>());
                    return mWriteMutexes.emplace_hint(it, std::move(mutexPair))->second.get();
                } else {
                    throw nullptr;
                }
            }

            return it->second.get();
        }

        template <typename T>
        void WaitUntilFinished() {
            auto group = WriteWaitGroup(entt::resolve<T>(), false);
            if (group)
                group->wait();
        }

        template <typename T>
        WaitHandle ReadHandle() {
            auto group = ReadWaitGroup(entt::resolve<T>(), true);
            group->add();
            return WaitHandle(*group);
        }

        template <typename T>
        WaitHandle WriteHandle() {
            auto readgroup = ReadWaitGroup(entt::resolve<T>(), true);
            auto writegroup = WriteWaitGroup(entt::resolve<T>(), true);
            auto mutex = WriteMutex(entt::resolve<T>(), true);
            writegroup->add();
            return WaitHandle(*readgroup, *writegroup, *mutex);
        }
    };

    class SystemCollection {
    private:
        std::vector<std::unique_ptr<ISystem>> mSystems;
        SyncObject mSyncObject;
        Frame* mFrame;

        InterfaceCollection mInterfaces;
    
    public:
        SystemCollection();
        ~SystemCollection();

        template <typename T>
        inline void AddInterface(T* t) {
            mInterfaces.Add<T>(t);
        }

        template <typename T>
        inline T* QueryInterface() {
            return mInterfaces.Query<T>();
        }

        template <typename SystemT, typename ... Args>
        ISystem* Add(Args&&... args) {
            auto ptr = std::make_unique<SystemT>(std::forward<Args>(args)...);
            ptr->RegisterInterfaces(mInterfaces);
            return mSystems.emplace_back(std::move(ptr)).get();
        }

        inline ISystem* Add(std::unique_ptr<ISystem>&& system) {
            system->RegisterInterfaces(mInterfaces);
            return mSystems.emplace_back(std::move(system)).get();
        }

        void Fork(const Time& time);
        void Join();

        void Startup();
        void Shutdown();
        void LoadResources();
        void SetFrame(Frame& frame);
    };

	void PrintWarning(const std::string& str);

    template <typename ... Types>
    class UpdaterReads;

    struct ReadLock {
        WaitHandle* mReadWait = nullptr;

        ReadLock(const ReadLock&) = delete;
        ReadLock& operator=(const ReadLock&) = delete;

        ReadLock(ReadLock&& lock) = default;
        ReadLock& operator=(ReadLock&& lock) = default;

        ReadLock(WaitHandle& handle) : mReadWait(&handle) {
            assert(handle.IsRead());
            if (handle.bFinished) {
                throw std::runtime_error("Handle has already been consumed!");
            }
        }

        ~ReadLock() {
            if (mReadWait) {
                mReadWait->Release();
            }
        }
    };

    struct WriteLock : private marl::lock {
        WaitHandle* mWriteWait = nullptr;

        WriteLock(const WriteLock&) = delete;
        WriteLock& operator=(const WriteLock&) = delete;

        WriteLock(WriteLock&& lock) = default;
        WriteLock& operator=(WriteLock&& lock) = default;

        inline WriteLock(WaitHandle& handle) : 
            marl::lock(*handle.mWriteMutex),
            mWriteWait(&handle) {
            mWriteWait->mBefore->wait();
            if (handle.bFinished) {
                throw std::runtime_error("Handle has already been consumed!");
            }
        }

        ~WriteLock() {
            if (mWriteWait) {
                mWriteWait->Release();
            }
        }
    };

    template <typename Type1, typename ... Types>
    class UpdaterReads<Type1, Types...> {
    private:
        WaitHandle mHandle;
        UpdaterReads<Types...> mRemaining;
    
    public:
        void RequestSync(SyncObject& obj) {
            mHandle = obj.ReadHandle<Type1>();
            mRemaining.RequestSync(obj);
        }

        void ReleaseHandles() {
            mHandle.Release();
            mRemaining.ReleaseHandles();
        }

        template <typename T>
        ReadLock Read() {
            if constexpr (std::is_same_v<T, Type1>)
                return ReadLock(mHandle);
            else 
                return mRemaining.template Read<T>();
        }
    };

    template <>
    class UpdaterReads<> {
    public:
        void RequestSync(SyncObject& obj) {
        }

        void ReleaseHandles() {
        }

        template <typename T>
        ReadLock Read() {
            throw std::runtime_error("UpdaterReads does not have requested type!");
        }
    };

    template <typename ... Types>
    class UpdaterWrites;

    template <typename Type1, typename ... Types>
    class UpdaterWrites<Type1, Types...> {
    private:
        WaitHandle mHandle;
        UpdaterWrites<Types...> mRemaining;

    public:
        void RequestSync(SyncObject& obj) {
            mHandle = obj.WriteHandle<Type1>();
            RequestSync(obj);
        }

        void ReleaseHandles() {
            mHandle.Release();
            mRemaining.ReleaseHandles();
        }

        template <typename T>
        WriteLock Write() {
            if constexpr (std::is_same_v<T, Type1>)
                return WriteLock(mHandle);
            else 
                return mRemaining.template Write<T>();
        }
    };

    template <>
    class UpdaterWrites<> {
    public:
        void RequestSync(SyncObject& obj) {
        }

        void ReleaseHandles() {
        }

        template <typename T>
        WriteLock Write() {
            throw std::runtime_error("UpdaterWrites does not have requested type!");
        }
    };

    template <typename Reads, typename Writes>
    using updater_system_t = void(*)(Frame& frame, Reads& reads, Writes& writes, const Time& time); 

    template <
        typename Reads,
        typename Writes,
        updater_system_t<Reads, Writes> UpdaterFunc>
    class Updater : public ISystem {
    private:
        marl::Event mFinishedEvent;
        Reads mReads;
        Writes mWrites;

    public:
        void Startup(marl::WaitGroup& waitGroup) override { }
        void RegisterInterfaces(InterfaceCollection& interfaces) override { }
        void Shutdown() override { }
        void LoadResources(marl::WaitGroup& waitGroup) override { }
        void SetFrame(Frame& frame) override { }
        
        void Join(Frame& frame) override { 
            Wait();
        }

        void RequestSync(SyncObject& syncObject) override {
            mReads.RequestSync(syncObject);
            mWrites.RequestSync(syncObject);
        }

        void Fork(Frame& frame, 
            SyncObject& syncObject,
            const Time& time) override {

            mFinishedEvent.clear();
            marl::schedule([&frame, 
                &syncObject, 
                time, 
                finishedEvent = mFinishedEvent,
                &reads = mReads,
                &writes = mWrites]() {
                defer(reads.ReleaseHandles());
                defer(writes.ReleaseHandles());
                defer(finishedEvent.signal());
                (*UpdaterFunc)(frame, reads, writes, time);
            }); 
        }

        void Wait() override {
            mFinishedEvent.wait();
        }
    };
}