#pragma once

#include <entt/entt.hpp>

#include <queue>
#include <atomic>
#include <functional>
#include <condition_variable>

#include <okami/Hashers.hpp>

namespace okami::core_revamp {

    typedef entt::entity entity_t;
    constexpr auto null_entity = entt::null;

    struct ResourceTag {
        std::filesystem::path mPath;
    };

    template <typename T>
    struct AtomWrapper
    {
        std::atomic<T> _a;

        AtomWrapper()
            :_a()
        {}

        AtomWrapper(const std::atomic<T> &a)
            :_a(a.load())
        {}

        AtomWrapper(const AtomWrapper &other)
            :_a(other._a.load())
        {}

        AtomWrapper &operator=(const AtomWrapper &other)
        {
            _a.store(other._a.load());
        }
    };

    template <typename T>
    struct LoadParams;

    class EntityData {
    private:
        entity_t mGlobalEntity;
        std::atomic<bool> bIsInUse = false;
        entt::registry mRegistry;
        std::unordered_map<
            std::filesystem::path, 
            entity_t,
            okami::core::PathHash> mResourceTable;

    public:
        EntityData();

        class Access {
        private:
            EntityData* mData;

            inline Access(EntityData* data) :
                mData(data) {
            }

        public:
            inline entity_t GlobalEntity() const {
                return mData->mGlobalEntity;
            }
            inline entt::registry& Registry() {
                return mData->mRegistry;
            }
        };

        template <typename T>
        T& Get(entt::entity e);
        template <typename T>
        T* TryGet(entt::entity e);
        template <typename T>
        entt::entity Load(
            const std::filesystem::path& path, 
            const LoadParams<T>& params);
        void Delete(entt::entity e);
    };

    template <typename T>
    class Read {
    private:
        entt::registry* mRegistry;

    public:
        T& Get(entt::entity e);
        T* TryGet(entt::entity e);

        using view_t = decltype(Read<T>().mRegistry->view<T>());
    };

    class IEventProducer {
    public:
        virtual entt::meta_type GetEventType() const = 0; 

        virtual ~IEventProducer() = default;
    };

    class IEventConsumer {
        virtual entt::meta_type GetEventType() const = 0;

        virtual ~IEventConsumer() = default;
    };

    class IEventBuffer {
    public:
        virtual entt::meta_type GetType() const = 0;

        virtual ~IEventBuffer() = default;
    };

    template <typename T>
    struct EventBuffer :
        public IEventBuffer {
        std::vector<T> mEvents;

        entt::meta_type GetType() const override {
            return entt::resolve<T>();
        }
    };

    template <typename T>
    struct AddComponent {
        entity_t mEntity;
        T mData;
    };

    template <typename T>
    struct UpdateComponent {
        entity_t mEntity;
        T mData;
    };

    template <typename T>
    struct RemoveComponent {
        entity_t mEntity;
    };

    template <typename T>
    struct DataFlow {
        class Access {
        private:
            EntityData::Access mAccessToken;

        public:
            Access() = default;

            inline Access(EntityData::Access token) :
                mAccessToken(token) {
            }

            inline SetAccess(EntityData::Access access) {
                mAccessToken = access;
            }

            inline T& GetGlobal() const {
                return mAccessToken.Registry().get<T>(
                    mAccessToken.GlobalEntity());
            }

            inline T* TryGetGlobal() const {
                return mAccessToken.Registry().try_get<T>(
                    mAccessToken.GlobalEntity());
            }

            inline T& Get(entity_t e) const {
                return mRegistry->get<T>(e);
            }

            inline T* TryGet(entity_t e) const {
                return mRegistry->try_get<T>(e);
            }
        };

        class View {
        private:
            entt::view<T> mView;

        public:
            View() = default;

            inline View(EntityData::Access access) :
                mView(access.mData->mRegistry.view<T>()) {
            }

            inline View(entt::view<T>&& view) :
                mView(std::move(view)) {
            }

            inline SetAccess(EntityData::Access access) {
                mView = access.mData->mRegistry.view<T>();
            }

            auto begin() {
                return mView.begin();
            }

            auto end() {
                return mView.end();
            }

            inline T& Get(entity_t e) const {
                return mView.get<T>(e);
            }
        };

        class Write {
        private:
            EntityData::Access mAccessToken;
            EventBuffer<AddComponent<T>>* mAddBuffer;
            EventBuffer<UpdateComponent<T>>* mUpdateBuffer;
            EventBuffer<RemoveComponent<T>>* mRemoveBuffer;

        public:
            inline Write(
                EventBuffer<AddComponent<T>>* addBuffer,
                EventBuffer<UpdateComponent<T>>* updateBuffer,
                EventBuffer<RemoveComponent<T>>* removeBuffer) :
                    mAddBuffer(addBuffer),
                    mUpdateBuffer(updateBuffer),
                    mRemoveBuffer(removeBuffer) {
            }

            inline Write(
                EntityData::Access token,
                EventBuffer<AddComponent<T>>* addBuffer,
                EventBuffer<UpdateComponent<T>>* updateBuffer,
                EventBuffer<RemoveComponent<T>>* removeBuffer) :
                    mAccessToken(token),
                    mAddBuffer(addBuffer),
                    mUpdateBuffer(updateBuffer),
                    mRemoveBuffer(removeBuffer) {
            }

            inline SetAccess(EntityData::Access access) {
                mAccessToken = access;
            }

            inline Add(entity_t e, const T& value) {
                mAddBuffer->mEvents.push_back(AddComponent<T>{e, value});
            }

            inline Add(entity_t e, T&& value) {
                mAddBuffer->mEvents.push_back(AddComponent<T>{e, std::move(value)});
            }

            inline AddGlobal(const T& value) {
                mAddBuffer->mEvents.push_back(
                    AddComponent<T>{mAccessToken.GlobalEntity(), value});
            }

            inline AddGlobal(T&& value) {
                mAddBuffer->mEvents.push_back(
                    AddComponent<T>{mAccessToken.GlobalEntity(), std::move(value)});
            }

            inline Update(entity_t e, const T& value) {
                mUpdateBuffer->mEvents.push_back(
                    UpdateComponent<T>{e, value});
            }

            inline Update(entity_t e, const T&& value) {
                mUpdateBuffer->mEvents.push_back(
                    UpdateComponent<T>{e, std::move(value)});
            }

            inline UpdateGlobal(const T& value) {
                mUpdateBuffer->mEvents.push_back(
                    UpdateComponent<T>{mAccessToken.GlobalEntity(), value});
            }

            inline UpdateGlobal(T&& value) {
                mUpdateBuffer->mEvents.push_back(
                    UpdateComponent<T>{mAccessToken.GlobalEntity(), std::move(value)});
            }

            inline Remove(entity_t e) {
                mRemoveBuffer->mEvents.push_back(RemoveComponent<T>{e});
            }

            inline RemoveGlobal() {
                mRemoveBuffer->mEvents.push_back(
                    RemoveComponent<T>{mAccessToken.GlobalEntity()});
            }
        };
    };

    template <typename T>
    class Observer {
    private:
        EventBuffer<T>* mBuffer;

    public:
        inline Observer(EventBuffer<T>* buffer) :
            mBuffer(buffer) {
        }

        inline SetAccess(EntityData::Access access) {
        }

        auto cbegin() {
            return mBuffer->mEvents.cbegin();
        }

        auto cend() {
            return mBuffer->mEvents.cend();
        }
    };

    template <typename T>
    class Trigger {
    private:
        EventBuffer<T>* mBuffer;

    public:
        inline Raise(EventBuffer<T>* buffer) :
            mBuffer(buffer) {
        }

        void Enqueue(const T& value) {
            mBuffer->mEvents.push_back(value);
        }

        void Enqueue(T&& value) {
            mBuffer->mEvents.push_back(std::move(value));
        }
    };

    typedef int stage_id;
    typedef int pipeline_node_id;
    typedef int staging_buffer_id;

    struct PipelineNode {
        stage_id mStageId = -1;
        std::function<void(EntityData::Access)> mFunc;
        std::string mDebugString;
        std::vector<entt::meta_type> mInEventsTypes;
        std::vector<entt::meta_type> mOutEventsTypes;
        std::vector<pipeline_node_id> mIn;
        std::vector<pipeline_node_id> mOut;
        AtomWrapper<int> mInputsRemaining;
    };

    template <typename T>
    struct InEventTypes;

    template <typename T>
    struct OutEventTypes;

    template <typename T>
    using ComponentEventList = std::tuple<
        AddComponent<T>,
        UpdateComponent<T>,
        RemoveComponent<T>>;

    template <typename T>
    struct InEventTypes<typename DataFlow<T>::Access> {
        using Result = ComponentEventList<T>;
    };

    template <typename T>
    struct InEventTypes<typename DataFlow<T>::View> {
        using Result = ComponentEventList<T>;
    };

    template <typename T>
    struct InEventTypes<typename DataFlow<T>::Write> {
        using Result = std::tuple<>;
    };

    template <typename T>
    struct InEventTypes<Observer<T>> {
        using Result = std::tuple<T>;
    };

    template <typename T>
    struct InEventTypes<Trigger<T>> {
        using Result = std::tuple<>;
    };

    template <typename T>
    struct OutEventTypes<typename DataFlow<T>::Access> {
        using Result = std::tuple<>;
    };

    template <typename T>
    struct OutEventTypes<typename DataFlow<T>::View> {
        using Result = std::tuple<>;
    };

    template <typename T>
    struct OutEventTypes<typename DataFlow<T>::Write> {
        using Result = ComponentEventList<T>;
    };

    template <typename T>
    struct OutEventTypes<Observer<T>> {
        using Result = std::tuple<>;
    };

    template <typename T>
    struct OutEventTypes<Trigger<T>> {
        using Result = std::tuple<T>;
    };

    template <typename T>
    T BindEventBuffersToArgument(const std::vector<IEventBuffer*> buffers) {
        static_assert("Invalid argument!");
    }

    template <typename T>
    typename DataFlow<T>::Access BindEventBuffersToArgument<DataFlow<T>::Access>(
        const std::vector<IEventBuffer*> buffers) {
        if (buffers.size() != 0) {
            throw std::runtime_error("Incorrect number of event buffers!");
        }

        return typename DataFlow<T>::Access();
    }

    template <typename T>
    typename DataFlow<T>::View BindEventBuffersToArgument<DataFlow<T>::View>(
        const std::vector<IEventBuffer*> buffers) {
        if (buffers.size() != 0) {
            throw std::runtime_error("Incorrect number of event buffers!");
        }

        return typename DataFlow<T>::View();
    }

    template <typename T>
    typename DataFlow<T>::Write BindEventBuffersToArgument<DataFlow<T>::Write>(
        const std::vector<IEventBuffer*> buffers) {
        if (buffers.size() != 3) {
            throw std::runtime_error("Incorrect number of event buffers!");
        }

        auto addBuffer = 
            dynamic_cast<EventBuffer<AddComponent<T>>*>(buffers[0]);
        auto updateBuffer = 
            dynamic_cast<EventBuffer<UpdateComponent<T>>*>(buffers[1]);
        auto removeBuffer = 
            dynamic_cast<EventBuffer<RemoveComponent<T>>*>(buffers[2]);

        return typename DataFlow<T>::Write(
            addBuffer, updateBuffer, removeBuffer);
    }

    template <typename T>
    Observer<T> BindEventBuffersToArgument<Observer<T>>(
        const std::vector<IEventBuffer*> buffers) {
        if (buffers.size() != 1) {
            throw std::runtime_error("Incorrect number of event buffers!");
        }

        auto buffer = dynamic_cast<EventBuffer<T>*>(buffers[0]);
        return Observer<T>(buffer);
    }

    template <typename T>
    Trigger<T> BindEventBuffersToArgument<Trigger<T>>(
        const std::vector<IEventBuffer*> buffers) {
        if (buffers.size() != 1) {
            throw std::runtime_error("Incorrect number of event buffers!");
        }

        auto buffer = dynamic_cast<EventBuffer<T>*>(buffers[0]);
        return Trigger<T>(buffer);
    }

    struct EventBufferFactory {
        std::function<std::unique_ptr<IEventBuffer>()> mFactory;
        entt::meta_type mBufferType;

        std::unique_ptr<IEventBuffer> operator()() const {
            return mFactory();
        }
    };

    template <typename T>
    struct TypesToBufferFactories;

    template <typename... Ts>
    struct TypesToBufferFactories<std::tuple<Ts...>> {
        std::vector<EventBufferFactory> operator()() const {
            return std::vector<EventBufferFactory>() {
                EventBufferFactory{
                    []() {
                        return std::make_unique<EventBuffer<Ts>>()
                    },
                    entt::resolve<Ts>
                }...
            }
        }
    };

    template <typename T>
    using InBufferFactories = TypesToBufferFactories<
        typename InTypes<T>::Result>;

    template <typename T>
    using OutBufferFactories = TypesToBufferFactories<
        typename OutTypes<T>::Result>;

    class Pipeline {
    private:
        std::unordered_multimap<entt::meta_type,
            std::unique_ptr<IEventBuffer>,
            okami::core::TypeHash> mEventStagingBuffers;

        std::unordered_map<entt::meta_type,
            std::unique_ptr<IEventBuffer>,
            okami::core::TypeHash> mEventBuffers;

        std::vector<PipelineNode> mNodes;

        using event_buffer_it = 
            std::vector<std::unique_ptr<IEventBuffer>>::iterator;

        struct IdTypeHash {
            size_t operator()(const std::pair<stage_id, entt::meta_type>& a) {
                return std::hash<int>{}(a.first) ^ okami::core::TypeHash{}(a.second);
            }
        };

        std::unordered_map<
            std::pair<stage_id, entt::meta_type>,
            int,
            IdTypeHash> mStagingBuffersPerStage;

        bool bBuilt = false;

    public:
        template <typename... paramsT>
        void AddSystem(
            stage_id stage, 
            std::function<void(paramsT... params)> system,
            const std::string& debugString = "") {
            
            std::vector<entt::meta_type> ins;
            std::vector<entt::meta_type> outs;

            // Bind the correct event buffers to the future arguments of the system
            std::tuple<paramsT...> arguments = 
                std::make_tuple<paramsT...>(
                    [&mStagingBuffersPerStage,
                    &mEventBuffers,
                    &mEventStagingBuffers,
                    &ins, 
                    &outs]{
                        std::vector<EventBufferFactory> inFactories 
                            = InBufferFactories<paramsT>{}();
                        std::vector<EventBufferFactory> outFactories
                            = OutBufferFactories<paramsT>{}();
                        std::vector<IEventBuffer*> buffers;

                        for (auto& factory : inFactories) {
                            ins.push_back(factory.mBufferType);

                            auto it = mEventBuffers.find(factory.mBufferType);

                            if (it == mEventBuffers.end()) {
                                it = mEventBuffers.emplace_hint(it, factory());
                            }

                            buffers.push_back(it->second.get());
                        }

                        for (auto& factory : outFactories) {
                            ins.push_back(factory.mBufferType);

                            auto stageTypePair = std::make_pair(stage, factory.mBufferType);

                            auto bufCountIt = mStagingBuffersPerStage.find(stageTypePair);
                        
                            if (bufCountIt == mStagingBuffersPerStage.end()) {
                                bufCountIt = mStagingBuffersPerStage.emplace_hint(bufCountIt, 0);
                            }

                            bufCountIt->second++;

                            auto stagingBuffer = 

                            if (bufCountIt->second > mEventStagingBuffers.)
                        }

                        return BindEventBuffersToArgument<paramsT>(buffers);
                    }()...
                );

            // Bind the arguments to the system
            std::function<void(EntityData::Access)> boundFunc = 
                [arguments, system](EntityData::Access access) {
                constexpr std::size_t n = sizeof...(paramsT);
                
                auto bindAccess = [access](paramsT&&... params) {
                    params.SetAccess(access)...;
                    return std::make_tuple<paramsT...>(
                        std::forward(params)...
                    );
                };

                auto argumentsCopy = arguments;
                auto boundArguments = std::apply(bindAccess, std::move(argumentsCopy));
                std::apply(system, boundArguments);
            };

            PipelineNode node;
            node.mDebugString = debugString;
            node.mFunc = std::move(boundFunc);
            node.mInEventsTypes = std::move(ins);
            node.mOutEventsTypes = std::move(outs);
            node.mStageId = stage;

            mNodes.emplace_back(std::move(node));
        }

        void Build() {

        }
    };

    class ThreadPool {
    public:
        struct Data {
            std::atomic<bool> bShutdown;
            std::condition_variable mConditionVariable;
        };
    private:
        Data mData;

    public:
        void Startup();
        void Run(const Pipeline& pipeline, EntityData& data);
        void Shutdown();
    };
}