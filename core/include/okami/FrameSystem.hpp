#pragma once

#include <okami/Frame.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceInterface.hpp>

namespace okami::core {

	class FrameSystem final : public ISystem, public IResourceManager<Frame> {
	private:
		ResourceManager<Frame> mManager;
        marl::Event mUpdateFinished;

	public:
        void OnDestroy(Frame* frame);

        FrameSystem(ResourceInterface& resourceInterface); 

        void RegisterInterfaces(InterfaceCollection& interfaces) override;
	 	void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(marl::WaitGroup& waitGroup) override;
        void SetFrame(Frame& frame) override;
        void RequestSync(SyncObject& syncObject) override;

        void Fork(Frame& frame,
            SyncObject& syncObject,
            const Time& time) override;
        void Join(Frame& frame) override;
        void Wait() override;

		Handle<Frame> Load(
            const std::filesystem::path& path, 
            const LoadParams<Frame>& params, 
            resource_id_t newResId) override;

        Handle<Frame> Add(Frame&& obj, 
			resource_id_t newResId) override;
		Handle<Frame> Add(Frame&& obj, 
			const std::filesystem::path& path,
			resource_id_t newResId) override;
	};
}