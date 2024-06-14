#define WLR_USE_UNSTABLE

#include "globals.hpp"

#include "hyprlandLayout.hpp"
#include "focusHooks.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

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
    static auto DWCB = HyprlandAPI::registerCallbackDynamic(PHANDLE, "destroyWorkspace", [&](void *self, SCallbackInfo &, std::any data) { onWorkspaceDestroy(self, data);});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:extlayout:layout:special_scale_factor", Hyprlang::FLOAT{0.8f});

	
    static const auto WINDOWUNIFIEDMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "vectorToWindowUnified");
    g_pvectorToWindowUnifiedHook = HyprlandAPI::createFunctionHook(PHANDLE, WINDOWUNIFIEDMETHODS[0].address, (void*)&hkvectorToWindowUnified);
    g_pvectorToWindowUnifiedHook->hook();

    HyprlandAPI::reloadConfig();

    return {"Hyprland Layout", "Use external layout generators", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}
