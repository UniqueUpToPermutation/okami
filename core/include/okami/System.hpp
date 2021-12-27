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

    class SyncObject {
    private:
        std::unordered_map<entt::meta_type, 
            marl::WaitGroup, TypeHash> mReadWaits;
        std::unordered_map<entt::meta_type, 
            std::unique_ptr<marl::mutex>, TypeHash> mWriteMutexes;

    public:
        SyncObject() = default;
        SyncObject(SyncObject&&) = default;
        SyncObject& operator=(SyncObject&&) = default;
        SyncObject(const SyncObject&) = delete;
        SyncObject& operator=(const SyncObject&) = delete;

        marl::WaitGroup Read(const entt::meta_type& type, bool bCreateIfNotFound = false) {
            auto it = mReadWaits.find(type);

            if (it == mReadWaits.end()) {
                if (bCreateIfNotFound) {
                    auto waitGroup = marl::WaitGroup();
                    mReadWaits.emplace_hint(it, 
                        std::pair<entt::meta_type, marl::WaitGroup>(type, std::move(waitGroup)));
                    return waitGroup;
                } else {
                    throw std::runtime_error("Read wait group not available!");
                }
            }

            return it->second;
        }

        marl::mutex& Write(const entt::meta_type& type, bool bCreateIfNotFound = false) {
            auto it = mWriteMutexes.find(type);

            if (it == mWriteMutexes.end()) {
                if (bCreateIfNotFound) {
                    std::pair<entt::meta_type, std::unique_ptr<marl::mutex>> mutexPair(type, 
                        std::make_unique<marl::mutex>());
                    return *mWriteMutexes.emplace_hint(it, std::move(mutexPair))->second;
                } else {
                    throw std::runtime_error("Read wait group not available!");
                }
            }

            return *it->second;
        }

        template <typename T>
        inline marl::WaitGroup Read(bool bCreateIfNotFound = true) {
            return Read(entt::resolve<T>(), bCreateIfNotFound);
        }

        template <typename T>
        inline marl::mutex& Write(bool bCreateIfNotFound = false) {
            return Write(entt::resolve<T>(), bCreateIfNotFound);
        }

        template <typename T>
        inline void RequireWrite() {
            Write<T>(true);
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
    struct UpdaterReads;

    template <typename Type1, typename ... Types>
    struct UpdaterReads<Type1, Types...> {
        static void RequestSync(SyncObject& obj) {
            obj.Read<Type1>().add();
            UpdaterReads<Types...>::RequestSync(obj);
        }
    };

    template <>
    struct UpdaterReads<> {
        static void RequestSync(SyncObject& obj) {
        }
    };

    template <typename ... Types>
    struct UpdaterWrites;

    template <typename Type1, typename ... Types>
    struct UpdaterWrites<Type1, Types...> {
        static void RequestSync(SyncObject& obj) {
            obj.RequireWrite<Type1>();
            UpdaterWrites<Types...>::RequestSync(obj);
        }
    };

    template <>
    struct UpdaterWrites<> {
        static void RequestSync(SyncObject& obj) {
        }
    };

    typedef void(*updater_system_t)(
            Frame& frame,
            SyncObject& syncObject,
            const Time& time);

    template <updater_system_t UpdaterFunc,
        typename Reads,
        typename Writes>
    class Updater : public ISystem {
    private:
        marl::Event mFinishedEvent;

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
            Reads::RequestSync(syncObject);
            Writes::RequestSync(syncObject);
        }
        void Fork(Frame& frame, 
            SyncObject& syncObject,
            const Time& time) override {

            mFinishedEvent.clear();
            marl::schedule([&frame, &syncObject, 
                time, 
                finishedEvent = mFinishedEvent]() {
                defer(finishedEvent.signal());
                (*UpdaterFunc)(frame, syncObject, time);
            }); 
        }

        void Wait() override {
            mFinishedEvent.wait();
        }
    };
}