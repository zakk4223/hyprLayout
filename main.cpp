#define WLR_USE_UNSTABLE

#include "globals.hpp"

#include "hyprlandLayout.hpp"
#include "focusHooks.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <hyprland/src/helpers/Workspace.hpp>

#include <unistd.h>
#include <thread>

// Methods

HOOK_CALLBACK_FN *destroyWorkspaceCallback = nullptr;
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
    destroyWorkspaceCallback = HyprlandAPI::registerCallbackDynamic(PHANDLE, "destroyWorkspace", [&](void *self, std::any data) { onWorkspaceDestroy(self, data);});

    static const auto WINDOWIDEALMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "vectorToWindowIdeal");
    g_pvectorToWindowIdealHook = HyprlandAPI::createFunctionHook(PHANDLE, WINDOWIDEALMETHODS[0].address, (void*)&hkvectorToWindowIdeal);
    g_pvectorToWindowIdealHook->hook();

    static const auto WINDOWFROMCURSORMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "windowFromCursor");
    g_pwindowFromCursorHook = HyprlandAPI::createFunctionHook(PHANDLE, WINDOWFROMCURSORMETHODS[0].address, (void*)&hkwindowFromCursor);
    g_pwindowFromCursorHook->hook();

    static const auto VECTORTOWINDOWTILEDMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "vectorToWindowTiled");
    g_pvectorToWindowTiledHook = HyprlandAPI::createFunctionHook(PHANDLE, VECTORTOWINDOWTILEDMETHODS[0].address, (void*)&hkvectorToWindowTiled);
    g_pvectorToWindowTiledHook->hook();

    static const auto VECTORTOWINDOWMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "vectorToWindow");
    g_pvectorToWindowHook = HyprlandAPI::createFunctionHook(PHANDLE, VECTORTOWINDOWMETHODS[0].address, (void*)&hkvectorToWindow);
    g_pvectorToWindowHook->hook();

    HyprlandAPI::reloadConfig();

    return {"Hyprland Layout", "Use external layout generators", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
    if (destroyWorkspaceCallback) {
      HyprlandAPI::unregisterCallback(PHANDLE, destroyWorkspaceCallback);
    }
}
