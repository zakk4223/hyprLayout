#include "HyprlandLayoutProtocolManager.hpp"
#include <hyprland/src/Compositor.hpp>
#include "hyprland-layout-v1-protocol.h"





static void handleDestroy(wl_client *client, wl_resource *resource) {
	wl_resource_destroy(resource);
}

static void bindManagerInt(wl_client *client, void *data, uint32_t version, uint32_t id) {
	g_pHyprlandLayoutProtocolManager->bindManager(client, data, version, id);
}

static void handleDisplayDestroy(struct wl_listener *listener, void *data) {
	g_pHyprlandLayoutProtocolManager->displayDestroy();
}


void handleGetLayout(wl_client *client, wl_resource *resource, uint32_t id, const char *r_namespace) {
	g_pHyprlandLayoutProtocolManager->getLayout(client, resource, id, r_namespace);
}

void handlePushWindowDimensions(wl_client *client, wl_resource *resource, const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t serial) {

	g_pHyprlandLayoutProtocolManager->pushWindowDimensions(client, resource, window_id, window_index, x, y, width, height, serial);
}


void handleCommit(wl_client *client, wl_resource *resource, const char *layout_name, const char *config_data, uint32_t serial) {

	g_pHyprlandLayoutProtocolManager->commit(client, resource, layout_name, config_data, serial);
}



static const struct hyprland_layout_manager_v1_interface hyprlandLayoutManagerImpl = {
	.destroy = handleDestroy,
	.get_layout = handleGetLayout,

};

static const struct hyprland_layout_v1_interface hyprlandLayoutImpl = {
	.destroy = handleDestroy, 
	.push_window_dimensions = handlePushWindowDimensions, 
	.commit = handleCommit,

};

CHyprlandLayout *layoutFromResource(wl_resource *resource) {
	ASSERT(wl_resource_instance_of(resource, &hyprland_layout_v1_interface, &hyprlandLayoutImpl));
	return (CHyprlandLayout *)wl_resource_get_user_data(resource);
}

static void handleLayoutDestroy(wl_resource *resource) {
	const auto LAYOUT = layoutFromResource(resource);
	if (LAYOUT) {
		wl_resource_set_user_data(resource, nullptr);
  	g_pHyprlandLayoutProtocolManager->removeLayout(LAYOUT);
  }
}

CHyprlandLayoutProtocolManager::CHyprlandLayoutProtocolManager() {

	m_pGlobal = wl_global_create(g_pCompositor->m_sWLDisplay, &hyprland_layout_manager_v1_interface, 1, this, bindManagerInt);
	if (!m_pGlobal) {
		Debug::log(LOG, "HyprlandLayout could not start!");
		return;
	}

	m_liDisplayDestroy.notify = handleDisplayDestroy;
	wl_display_add_destroy_listener(g_pCompositor->m_sWLDisplay, &m_liDisplayDestroy);
	Debug::log(LOG, "HyprlandLayout started successfully!");
}


void CHyprlandLayoutProtocolManager::workspaceDestroyed(int workspace) {

    for (auto& layout : m_vHyprlandLayouts) { 
        layout.get()->workspaceDestroyed(workspace);
    }
}


void CHyprlandLayoutProtocolManager::pushWindowDimensions(wl_client *client, wl_resource *resource, const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t serial) {
	const auto LAYOUT = layoutFromResource(resource);
	if (LAYOUT) {
		LAYOUT->hyprlandLayoutWindowDimensions(window_id, window_index, x, y, width, height, serial);
	}
}

void CHyprlandLayoutProtocolManager::commit(wl_client *client, wl_resource *resource, const char *layout_name, const char *config_data, uint32_t serial) {
	const auto LAYOUT = layoutFromResource(resource);
	if (LAYOUT) {
		LAYOUT->hyprlandLayoutCommit(layout_name, config_data, serial);
	}
}

void CHyprlandLayoutProtocolManager::bindManager(wl_client *client, void *data, uint32_t version, uint32_t id) {
	const auto RESOURCE = wl_resource_create(client, &hyprland_layout_manager_v1_interface, version, id);
	wl_resource_set_implementation(RESOURCE, &hyprlandLayoutManagerImpl, this, nullptr);
	Debug::log(LOG, "HyprlandLayoutManager bound successfully!");
}

void CHyprlandLayoutProtocolManager::getLayout(wl_client *client, wl_resource *resource, uint32_t id, const char *r_namespace) {

	const auto LAYOUT = std::find_if(m_vHyprlandLayouts.begin(), m_vHyprlandLayouts.end(), [&](const auto &layout) { return layout->m_sHyprlandLayoutNamespace == r_namespace; });

	wl_resource *newResource = wl_resource_create(client, &hyprland_layout_v1_interface, wl_resource_get_version(resource), id);
	if (LAYOUT == m_vHyprlandLayouts.end()) { //Didn't find it
		m_vHyprlandLayouts.emplace_back(std::make_unique<CHyprlandLayout>(r_namespace, newResource));
		wl_resource_set_implementation(newResource, &hyprlandLayoutImpl, m_vHyprlandLayouts.back().get(), &handleLayoutDestroy);
    		HyprlandAPI::addLayout(PHANDLE, m_vHyprlandLayouts.back()->m_sHyprlandLayoutNamespace, m_vHyprlandLayouts.back().get());
	} else {
  //TODO: namespace collision event
		wl_resource_set_implementation(newResource, &hyprlandLayoutImpl, (*LAYOUT).get(), &handleLayoutDestroy);
    hyprland_layout_v1_send_name_in_use(newResource);
	}
}

void CHyprlandLayoutProtocolManager::removeLayout(CHyprlandLayout *rmLayout) {
	HyprlandAPI::removeLayout(PHANDLE, rmLayout);
	std::erase_if(m_vHyprlandLayouts, [&](const auto& l) { return l.get() == rmLayout; });
}

void CHyprlandLayoutProtocolManager::displayDestroy() {
	wl_global_destroy(m_pGlobal);
}


void CHyprlandLayoutProtocolManager::sendLayoutDemandResize(wl_resource *resource, const char *window_id, int32_t dx, int32_t dy, int32_t dwidth, int32_t dheight, uint32_t serial) {

  hyprland_layout_v1_send_layout_demand_resize(resource, window_id, dx, dy, dwidth, dheight, serial);
}

void CHyprlandLayoutProtocolManager::sendLayoutDemand(wl_resource *resource, uint32_t usable_width, uint32_t usable_height, int32_t workspace,uint32_t window_count, uint32_t serial) {
		hyprland_layout_v1_send_layout_demand(resource, usable_width, usable_height, workspace, window_count, serial);
}

void CHyprlandLayoutProtocolManager::sendLayoutDemandConfig(wl_resource *resource, const char *config_data, uint32_t serial) {
  hyprland_layout_v1_send_layout_demand_config(resource, config_data, serial);
}

void CHyprlandLayoutProtocolManager::sendUserCommand(wl_resource *resource, const char *command, uint32_t serial) {
	hyprland_layout_v1_send_user_command(resource, command, serial);
}

void CHyprlandLayoutProtocolManager::sendLayoutDemandCommit(wl_resource *resource, uint32_t serial)
{
  hyprland_layout_v1_send_layout_demand_commit(resource, serial);
}

void CHyprlandLayoutProtocolManager::sendWindowInfo(wl_resource *resource, const char *window_id, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t is_master, uint32_t is_active, uint32_t user_modified, const char *tags, uint32_t serial)
{
  hyprland_layout_v1_send_window_info(resource, window_id, x, y, width, height, is_master, is_active, user_modified, tags, serial);
}
