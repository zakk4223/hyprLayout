#define WLR_USE_UNSTABLE

#include "globals.hpp"

#include "hyprlandLayout.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <src/helpers/Workspace.hpp>

#include <unistd.h>
#include <thread>

// Methods

void onWorkspaceDestroy(void *self, std::any data) {
  auto *const WORKSPACE = std::any_cast<CWorkspace*>(data);
  g_pHyprlandLayoutProtocolManager->workspaceDestroyed(WORKSPACE->m_iID);
}
// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    g_pHyprlandLayoutProtocolManager = std::make_unique<CHyprlandLayoutProtocolManager>();
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "destroyWorkspace", [&](void *self, std::any data) { onWorkspaceDestroy(self, data);});
    HyprlandAPI::reloadConfig();

    return {"Hyprland Layout", "Use external layout generators", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}
