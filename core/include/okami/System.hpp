#pragma once 

#include <entt/entt.hpp>
#include <okami/Frame.hpp>
#include <marl/waitgroup.h>

namespace okami::core {
    class SyncObject;

    class Frame;

    struct Time {
        double mTimeElapsed;
        double mTotalTime;
    };

    class InterfaceCollection {
    private:
        std::unordered_map<entt::id_type, void*> mInterfaces;
    
    public:
        template <typename T>
        inline void Add(T* t) {
            mInterfaces.emplace(entt::resolve<T>().id(), t);
        }

        template <typename T>
        inline T* Query() {
            auto it = mInterfaces.find(entt::resolve<T>().id());
            if (it == mInterfaces.end()) {
                return nullptr;
            } else {
                return reinterpret_cast<T*>(it->second);
            }
        }
    };

    class ISystem {
    public:
        // Initialize the system and load all required resources.
        virtual void Startup(marl::WaitGroup& waitGroup) = 0;

        // Register all interfaces to the specified interface collection
        virtual void RegisterInterfaces(InterfaceCollection& interfaces) = 0;
        
        // Destroy the system and free all used resources.
        virtual void Shutdown() = 0;

        // Load all underlying resources for the specified frame.
        virtual void LoadResources(Frame* frame, 
            marl::WaitGroup& waitGroup) = 0;

        // Begin execution of this system's update proceedure.
        // Gauranteed to be called from the main thread.
        // You should offload work into marl tasks as opposed
        // to performing operations in the function definition.
        virtual void BeginExecute(Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            SyncObject& syncObject,
            const Time& time) = 0;

        // Ends the execution of this system's update proceedure.
        // Gauranteed to be called from the main thread.
        virtual void EndExecute(Frame* frame) = 0;

        virtual ~ISystem() = default;
    };

    class SyncObject {
    private:
        std::unordered_map<entt::id_type, marl::WaitGroup> mReadWaits;
        std::unordered_map<entt::id_type, std::unique_ptr<marl::mutex>> mWriteMutexes;

    public:
        marl::WaitGroup Read(entt::id_type typeId, bool bCreateIfNotFound = false) {
            auto it = mReadWaits.find(typeId);

            if (it == mReadWaits.end()) {
                if (bCreateIfNotFound) {
                    auto waitGroup = marl::WaitGroup();
                    mReadWaits.emplace_hint(it, 
                        std::pair<const entt::id_type, marl::WaitGroup>(typeId, std::move(waitGroup)));
                    return waitGroup;
                } else {
                    throw std::runtime_error("Read wait group not available!");
                }
            }

            return it->second;
        }

        marl::mutex& Write(entt::id_type typeId, bool bCreateIfNotFound = false) {
            auto it = mWriteMutexes.find(typeId);

            if (it == mWriteMutexes.end()) {
                if (bCreateIfNotFound) {
                    std::pair<const entt::id_type, std::unique_ptr<marl::mutex>> mutexPair(typeId, 
                        std::make_unique<marl::mutex>());
                    return *mWriteMutexes.emplace_hint(it, std::move(mutexPair))->second;
                } else {
                    throw std::runtime_error("Read wait group not available!");
                }
            }

            return *it->second;
        }

        template <typename T>
        inline marl::WaitGroup Read(bool bCreateIfNotFound = false) {
            return Read(entt::resolve<T>().id(), bCreateIfNotFound);
        }

        template <typename T>
        inline marl::mutex& Write(bool bCreateIfNotFound = false) {
            return Write(entt::resolve<T>().id(), bCreateIfNotFound);
        }

        template <typename T>
        inline void RequireRead() {
            Read<T>(true);
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

        marl::WaitGroup mRenderGroup;
        marl::WaitGroup mUpdateGroup;
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
            return mSystems.emplace(std::make_unique<SystemT>(std::forward(args...))).first->get();
        }

        inline ISystem* Add(std::unique_ptr<ISystem>&& system) {
            return mSystems.emplace_back(std::move(system)).get();
        }

        inline void WaitOnRender() {
            mRenderGroup.wait();
        }

        void BeginExecute(Frame* frame, const Time& time);
        void EndExecute();
        void Startup();
        void Shutdown();
        void LoadResources(Frame* frame);
    };
}