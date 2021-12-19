#pragma once

#include <okami/Frame.hpp>
#include <okami/ResourceManager.hpp>
#include <okami/ResourceInterface.hpp>

namespace okami::core {

	class FrameSystem final : public ISystem, public IResourceManager<Frame> {
	private:
		ResourceManager<Frame> mManager;

	public:
        void OnDestroy(Frame* frame);

        FrameSystem(ResourceInterface& resourceInterface); 

        void RegisterInterfaces(InterfaceCollection& interfaces) override;
	 	void Startup(marl::WaitGroup& waitGroup) override;
        void Shutdown() override;
        void LoadResources(Frame* frame, 
            marl::WaitGroup& waitGroup) override;
        void RequestSync(SyncObject& syncObject) override;

        void BeginExecute(Frame* frame, 
            marl::WaitGroup& renderGroup, 
            marl::WaitGroup& updateGroup,
            SyncObject& syncObject,
            const Time& time) override;
        void EndExecute(Frame* frame) override;

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