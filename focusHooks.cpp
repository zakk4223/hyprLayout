#include "focusHooks.hpp"
#include <hyprland/src/Compositor.hpp>
#include <ranges>

CWindow *hkvectorToWindowIdeal(void *thisptr, const Vector2D& pos) {
    const auto         PMONITOR          = g_pCompositor->getMonitorFromVector(pos);
    static auto* const PRESIZEONBORDER   = &g_pConfigManager->getConfigValuePtr("general:resize_on_border")->intValue;
    static auto* const PBORDERSIZE       = &g_pConfigManager->getConfigValuePtr("general:border_size")->intValue;
    static auto* const PBORDERGRABEXTEND = &g_pConfigManager->getConfigValuePtr("general:extend_border_grab_area")->intValue;
    const auto         BORDER_GRAB_AREA  = *PRESIZEONBORDER ? *PBORDERSIZE + *PBORDERGRABEXTEND : 0;

    // special workspace
    if (PMONITOR->specialWorkspaceID) {
        for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
            const auto BB  = w->getWindowInputBox();
            wlr_box    box = {BB.x - BORDER_GRAB_AREA, BB.y - BORDER_GRAB_AREA, BB.width + 2 * BORDER_GRAB_AREA, BB.height + 2 * BORDER_GRAB_AREA};
            if (w->m_bIsFloating && w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && w->m_bIsMapped && wlr_box_contains_point(&box, pos.x, pos.y) && !w->isHidden() &&
                !w->m_bX11ShouldntFocus && !w->m_bNoFocus)
                return w.get();
        }

        for (auto& w : g_pCompositor->m_vWindows) {
            wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
            if (!w->m_bIsFloating && w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && w->m_bIsMapped && wlr_box_contains_point(&box, pos.x, pos.y) && !w->isHidden() &&
                !w->m_bX11ShouldntFocus && !w->m_bNoFocus)
                return w.get();
        }
    }

    // pinned windows on top of floating regardless
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        const auto BB  = w->getWindowInputBox();
        wlr_box    box = {BB.x - BORDER_GRAB_AREA, BB.y - BORDER_GRAB_AREA, BB.width + 2 * BORDER_GRAB_AREA, BB.height + 2 * BORDER_GRAB_AREA};
        if (w->m_bIsFloating && w->m_bIsMapped && !w->isHidden() && !w->m_bX11ShouldntFocus && w->m_bPinned && !w->m_bNoFocus) {
            if (wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y))
                return w.get();

            if (!w->m_bIsX11) {
                if (w->hasPopupAt(pos))
                    return w.get();
            }
        }
    }

    // first loop over floating cuz they're above, m_lWindows should be sorted bottom->top, for tiled it doesn't matter.
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        const auto BB  = w->getWindowInputBox();
        wlr_box    box = {BB.x - BORDER_GRAB_AREA, BB.y - BORDER_GRAB_AREA, BB.width + 2 * BORDER_GRAB_AREA, BB.height + 2 * BORDER_GRAB_AREA};
        if (w->m_bIsFloating && w->m_bIsMapped && g_pCompositor->isWorkspaceVisible(w->m_iWorkspaceID) && !w->isHidden() && !w->m_bPinned && !w->m_bNoFocus) {
            // OR windows should add focus to parent
            if (w->m_bX11ShouldntFocus && w->m_iX11Type != 2)
                continue;

            if (wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y)) {

                if (w->m_bIsX11 && w->m_iX11Type == 2 && !wlr_xwayland_or_surface_wants_focus(w->m_uSurface.xwayland)) {
                    // Override Redirect
                    return g_pCompositor->m_pLastWindow; // we kinda trick everything here.
                                                         // TODO: this is wrong, we should focus the parent, but idk how to get it considering it's nullptr in most cases.
                }

                return w.get();
            }

            if (!w->m_bIsX11) {
                if (w->hasPopupAt(pos))
                    return w.get();
            }
        }
    }

    // for windows, we need to check their extensions too, first.
    for (auto& w : g_pCompositor->m_vWindows) {
        if (!w->m_bIsX11 && !w->m_bIsFloating && w->m_bIsMapped && w->m_iWorkspaceID == PMONITOR->activeWorkspace && !w->isHidden() && !w->m_bX11ShouldntFocus && !w->m_bNoFocus) {
            if ((w)->hasPopupAt(pos))
                return w.get();
        }
    }
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
        if (!w->m_bIsFloating && w->m_bIsMapped && wlr_box_contains_point(&box, pos.x, pos.y) && w->m_iWorkspaceID == PMONITOR->activeWorkspace && !w->isHidden() &&
            !w->m_bX11ShouldntFocus && !w->m_bNoFocus) {
            return w.get();
        }
    }

    return nullptr;

}



CWindow *hkwindowFromCursor(void *thisptr) {
    const auto PMONITOR = g_pCompositor->getMonitorFromCursor();

    if (PMONITOR->specialWorkspaceID) {
        for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
            wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
            if (w->m_bIsFloating && w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && w->m_bIsMapped && wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y) &&
                !w->isHidden() && !w->m_bNoFocus)
                return w.get();
        }

        for (auto& w : g_pCompositor->m_vWindows) {
            wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
            if (w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y) && w->m_bIsMapped && !w->m_bNoFocus)
                return w.get();
        }
    }

    // pinned
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
        if (wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y) && w->m_bIsMapped && w->m_bIsFloating && w->m_bPinned && !w->m_bNoFocus)
            return w.get();
    }

    // first loop over floating cuz they're above, m_lWindows should be sorted bottom->top, for tiled it doesn't matter.
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
        if (wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y) && w->m_bIsMapped && w->m_bIsFloating && g_pCompositor->isWorkspaceVisible(w->m_iWorkspaceID) && !w->m_bPinned &&
            !w->m_bNoFocus)
            return w.get();
    }

    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
        if (wlr_box_contains_point(&box, g_pCompositor->m_sWLRCursor->x, g_pCompositor->m_sWLRCursor->y) && w->m_bIsMapped && w->m_iWorkspaceID == PMONITOR->activeWorkspace && !w->m_bNoFocus)
            return w.get();
    }

    return nullptr;
}



CWindow* hkvectorToWindow(void *thisptr, const Vector2D& pos) {
    const auto PMONITOR = g_pCompositor->getMonitorFromVector(pos);

    if (PMONITOR->specialWorkspaceID) {
        for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
            wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
            if (w->m_bIsFloating && w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && w->m_bIsMapped && wlr_box_contains_point(&box, pos.x, pos.y) && !w->isHidden() &&
                !w->m_bNoFocus)
                return w.get();
        }

        for (auto& w : g_pCompositor->m_vWindows) {
            wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
            if (w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && wlr_box_contains_point(&box, pos.x, pos.y) && w->m_bIsMapped && !w->m_bIsFloating && !w->isHidden() &&
                !w->m_bNoFocus)
                return w.get();
        }
    }

    // pinned
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
        if (wlr_box_contains_point(&box, pos.x, pos.y) && w->m_bIsMapped && w->m_bIsFloating && !w->isHidden() && w->m_bPinned && !w->m_bNoFocus)
            return w.get();
    }

    // first loop over floating cuz they're above, m_vWindows should be sorted bottom->top, for tiled it doesn't matter.
    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
        if (wlr_box_contains_point(&box, pos.x, pos.y) && w->m_bIsMapped && w->m_bIsFloating && g_pCompositor->isWorkspaceVisible(w->m_iWorkspaceID) && !w->isHidden() && !w->m_bPinned &&
            !w->m_bNoFocus)
            return w.get();
    }

    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vRealPosition.vec().x, w->m_vRealPosition.vec().y, w->m_vRealSize.vec().x, w->m_vRealSize.vec().y};
        if (wlr_box_contains_point(&box, pos.x, pos.y) && w->m_bIsMapped && !w->m_bIsFloating && PMONITOR->activeWorkspace == w->m_iWorkspaceID && !w->isHidden() && !w->m_bNoFocus)
            return w.get();
    }

    return nullptr;
}

CWindow* hkvectorToWindowTiled(void *thisptr, const Vector2D& pos) {
    const auto PMONITOR = g_pCompositor->getMonitorFromVector(pos);

    if (PMONITOR->specialWorkspaceID) {
        for (auto& w : g_pCompositor->m_vWindows) {
            wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
            if (w->m_iWorkspaceID == PMONITOR->specialWorkspaceID && wlr_box_contains_point(&box, pos.x, pos.y) && !w->m_bIsFloating && !w->isHidden() && !w->m_bNoFocus)
                return w.get();
        }
    }

    for (auto& w : g_pCompositor->m_vWindows | std::views::reverse) {
        wlr_box box = {w->m_vPosition.x, w->m_vPosition.y, w->m_vSize.x, w->m_vSize.y};
        if (w->m_bIsMapped && wlr_box_contains_point(&box, pos.x, pos.y) && w->m_iWorkspaceID == PMONITOR->activeWorkspace && !w->m_bIsFloating && !w->isHidden() && !w->m_bNoFocus)
            return w.get();
    }

    return nullptr;
}


