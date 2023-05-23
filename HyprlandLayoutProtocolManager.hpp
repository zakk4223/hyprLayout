#pragma once
#define WLR_USE_UNSTABLE
#include "globals.hpp"
#include <src/defines.hpp>
#include "hyprlandLayout.hpp"



class CHyprlandLayout;

class CHyprlandLayoutProtocolManager {
	public:
		CHyprlandLayoutProtocolManager();
		void bindManager(wl_client *client, void *data, uint32_t version, uint32_t id);
		void displayDestroy();
		void getLayout(wl_client *client, wl_resource *resource, uint32_t id, const char *r_namespace);
		void pushWindowDimensions(wl_client *client, wl_resource *resource, const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t reject, uint32_t serial);
		void commit(wl_client *client, wl_resource *resource, const char *layout_name, const char *config_data, uint32_t serial);
    void sendWindowInfo(wl_resource *resource, const char *window_id, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t is_master, uint32_t is_active, uint32_t user_modified, uint32_t serial);
		void sendLayoutDemand(wl_resource *resource, uint32_t usable_width, uint32_t usable_height, int32_t workspace, uint32_t window_count, uint32_t serial);
		void sendUserCommand(wl_resource *resource, const char *command, uint32_t serial);
    void sendLayoutDemandConfig(wl_resource *resource, const char *config_data, uint32_t serial);
    void sendLayoutDemandCommit(wl_resource *resource, uint32_t serial);
		void removeLayout(CHyprlandLayout *rmLayout); 
    void workspaceDestroyed(int workspace);


	private:
		wl_global *m_pGlobal = nullptr;
		wl_listener m_liDisplayDestroy;
		wl_resource *m_pClientResource;
		std::vector<std::unique_ptr<CHyprlandLayout>> m_vHyprlandLayouts;

};

    inline std::unique_ptr<CHyprlandLayoutProtocolManager> g_pHyprlandLayoutProtocolManager;


