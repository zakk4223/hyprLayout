#include "focusHooks.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprland/src/xwayland/XWayland.hpp>
#include <ranges>



PHLWINDOW hkvectorToWindowUnified(void *thisptr, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow) {
	  CCompositor *self = (CCompositor *)thisptr;


    const auto  PMONITOR          = self->getMonitorFromVector(pos);
    static auto PRESIZEONBORDER   = CConfigValue<Hyprlang::INT>("general:resize_on_border");
    static auto PBORDERSIZE       = CConfigValue<Hyprlang::INT>("general:border_size");
    static auto PBORDERGRABEXTEND = CConfigValue<Hyprlang::INT>("general:extend_border_grab_area");
    static auto PSPECIALFALLTHRU  = CConfigValue<Hyprlang::INT>("input:special_fallthrough");
    const auto  BORDER_GRAB_AREA  = *PRESIZEONBORDER ? *PBORDERSIZE + *PBORDERGRABEXTEND : 0;

    // pinned windows on top of floating regardless
    if (properties & ALLOW_FLOATING) {
        for (auto& w : self->m_vWindows | std::views::reverse) {
            const auto BB  = w->getWindowBoxUnified(properties);
            CBox       box = BB.copy().expand(w->m_iX11Type == 2 ? BORDER_GRAB_AREA : 0);
            if (w->m_bIsFloating && w->m_bIsMapped && !w->isHidden() && !w->m_bX11ShouldntFocus && w->m_bPinned && !w->m_sAdditionalConfigData.noFocus && w != pIgnoreWindow) {
                if (box.containsPoint(g_pPointerManager->position()))
                    return w;

                if (!w->m_bIsX11) {
                    if (w->hasPopupAt(pos))
                        return w;
                }
            }
        }
    }

    auto windowForWorkspace = [&](bool special) -> PHLWINDOW {
        auto floating = [&](bool aboveFullscreen) -> PHLWINDOW {
            for (auto& w : self->m_vWindows | std::views::reverse) {

                if (special && !w->onSpecialWorkspace()) // because special floating may creep up into regular
                    continue;

                const auto BB             = w->getWindowBoxUnified(properties);
                const auto PWINDOWMONITOR = self->getMonitorFromID(w->m_iMonitorID);

                // to avoid focusing windows behind special workspaces from other monitors
                if (!*PSPECIALFALLTHRU && PWINDOWMONITOR && PWINDOWMONITOR->activeSpecialWorkspace && w->m_pWorkspace != PWINDOWMONITOR->activeSpecialWorkspace &&
                    BB.x >= PWINDOWMONITOR->vecPosition.x && BB.y >= PWINDOWMONITOR->vecPosition.y &&
                    BB.x + BB.width <= PWINDOWMONITOR->vecPosition.x + PWINDOWMONITOR->vecSize.x && BB.y + BB.height <= PWINDOWMONITOR->vecPosition.y + PWINDOWMONITOR->vecSize.y)
                    continue;

                CBox box = BB.copy().expand(w->m_iX11Type == 2 ? BORDER_GRAB_AREA : 0);
                if (w->m_bIsFloating && w->m_bIsMapped && self->isWorkspaceVisible(w->m_pWorkspace) && !w->isHidden() && !w->m_bPinned && !w->m_sAdditionalConfigData.noFocus &&
                    w != pIgnoreWindow && (!aboveFullscreen || w->m_bCreatedOverFullscreen)) {
                    // OR windows should add focus to parent
                    if (w->m_bX11ShouldntFocus && w->m_iX11Type != 2)
                        continue;

                    if (box.containsPoint(g_pPointerManager->position())) {

                        if (w->m_bIsX11 && w->m_iX11Type == 2 && !w->m_pXWaylandSurface->wantsFocus()) {
                            // Override Redirect
                            return g_pCompositor->m_pLastWindow.lock(); // we kinda trick everything here.
                                // TODO: this is wrong, we should focus the parent, but idk how to get it considering it's nullptr in most cases.
                        }

                        return w;
                    }

                    if (!w->m_bIsX11) {
                        if (w->hasPopupAt(pos))
                            return w;
                    }
                }
            }

            return nullptr;
        };

        if (properties & ALLOW_FLOATING) {
            // first loop over floating cuz they're above, m_lWindows should be sorted bottom->top, for tiled it doesn't matter.
            auto found = floating(true);
            if (found)
                return found;
        }

        if (properties & FLOATING_ONLY)
            return floating(false);

        const int64_t WORKSPACEID = special ? PMONITOR->activeSpecialWorkspaceID() : PMONITOR->activeWorkspaceID();
        const auto    PWORKSPACE  = self->getWorkspaceByID(WORKSPACEID);

        if (PWORKSPACE->m_bHasFullscreenWindow)
            return self->getFullscreenWindowOnWorkspace(PWORKSPACE->m_iID);

        auto found = floating(false);
        if (found)
            return found;

        // for windows, we need to check their extensions too, first.
        for (auto& w : self->m_vWindows) {
            if (special != w->onSpecialWorkspace())
                continue;

            if (!w->m_bIsX11 && !w->m_bIsFloating && w->m_bIsMapped && w->workspaceID() == WORKSPACEID && !w->isHidden() && !w->m_bX11ShouldntFocus &&
                !w->m_sAdditionalConfigData.noFocus && w != pIgnoreWindow) {
                if (w->hasPopupAt(pos))
                    return w;
            }
        }

        for (auto& w : self->m_vWindows | std::views::reverse) {
            if (special != w->onSpecialWorkspace())
                continue;

            CBox box = (properties & USE_PROP_TILED) ? w->getWindowBoxUnified(properties) : CBox{w->m_vPosition, w->m_vSize};
            if (!w->m_bIsFloating && w->m_bIsMapped && box.containsPoint(pos) && w->workspaceID() == WORKSPACEID && !w->isHidden() && !w->m_bX11ShouldntFocus &&
                !w->m_sAdditionalConfigData.noFocus && w != pIgnoreWindow)
                return w;
        }

        return nullptr;
    };

    // special workspace
    if (PMONITOR->activeSpecialWorkspace && !*PSPECIALFALLTHRU)
        return windowForWorkspace(true);

    if (PMONITOR->activeSpecialWorkspace) {
        const auto PWINDOW = windowForWorkspace(true);

        if (PWINDOW)
            return PWINDOW;
    }

    return windowForWorkspace(false);
}

