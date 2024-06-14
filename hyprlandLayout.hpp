#pragma once

#define WLR_USE_UNSTABLE

#include "globals.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <hyprland/src/layout/IHyprLayout.hpp>
#include <vector>
#include <list>
#include <deque>
#include <any>

enum eFullscreenMode : int8_t;

struct SHyprlandLayoutNodeData {

    uint32_t requestSerial;
    bool     requestDone;
    uint32_t requestIdx;
    bool     isMaster = false;
    bool     userModified = false;
    PHLWINDOWREF pWindow; 
    std::string windowID;

    Vector2D position;
    Vector2D size;

    float    percSize = 1.f; // size multiplier for resizing children

    int      workspaceID = -1;
		bool 		ignoreFullscreenChecks = false;

    bool     operator==(const SHyprlandLayoutNodeData& rhs) const {
        return pWindow == rhs.pWindow;
    }
};

struct SHyprlandLayoutWorkspaceData {
    int                workspaceID = -1;
    std::string         layoutConfigData;
    bool               operator==(const SHyprlandLayoutWorkspaceData& rhs) const {
        return workspaceID == rhs.workspaceID;
    }
};


class CHyprlandLayout : public IHyprLayout {
  public:

    CHyprlandLayout(const char *r_namespace, wl_resource *resource);
    std::string			     m_sHyprlandLayoutNamespace = "";
    virtual void                     onWindowCreatedTiling(PHLWINDOW, eDirection direction = DIRECTION_DEFAULT);
    virtual void                     onWindowRemovedTiling(PHLWINDOW);
    virtual bool                     isWindowTiled(PHLWINDOW);
    virtual void                     recalculateMonitor(const int&);
    virtual void                     recalculateMonitor(const int &monid, bool commit);
    virtual void                     recalculateWindow(PHLWINDOW);
    virtual void                     resizeActiveWindow(const Vector2D&, eRectCorner corner, PHLWINDOW pWindow);
    virtual void                     fullscreenRequestForWindow(PHLWINDOW, eFullscreenMode, bool);
    virtual std::any                 layoutMessage(SLayoutMessageHeader, std::string);
    virtual SWindowRenderLayoutHints requestRenderHints(PHLWINDOW);
    virtual void                     switchWindows(PHLWINDOW, PHLWINDOW);
    virtual void                     alterSplitRatio(PHLWINDOW, float, bool);
    virtual std::string              getLayoutName();
    virtual void                     replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to);
    virtual Vector2D 								 predictSizeForNewWindowTiled();

    virtual void                     onEnable();
    virtual void                     onDisable();
    virtual void                     onWindowFocusChange(PHLWINDOW);
		virtual void										 moveWindowTo(PHLWINDOW, const std::string &dir, bool silent);
    void 			     hyprlandLayoutWindowDimensions(const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t serial);
    void			     hyprlandLayoutCommit(const char *layout_name, const char *config_data, uint32_t serial);
    void           workspaceDestroyed(const int& ws);


        



  private:
    std::list<SHyprlandLayoutNodeData>        m_lHyprlandLayoutNodesData;
    std::vector<SHyprlandLayoutWorkspaceData> m_lHyprlandLayoutWorkspacesData;
    wl_resource                     *m_pWLResource;
    Vector2D 			   m_vRemovedWindowVector;
    uint32_t         m_iLastLayoutSerial = 0;
    SHyprlandLayoutNodeData *m_pLastTiledWindowNodeData;
    

    bool                              m_bForceWarps = false;

    int                               getNodesOnWorkspace(const int&);
    void                              applyNodeDataToWindow(SHyprlandLayoutNodeData*);
    void                              resetNodeSplits(const int&);
    SHyprlandLayoutNodeData*                  getNodeFromWindow(PHLWINDOW);
    SHyprlandLayoutWorkspaceData*             getLayoutWorkspaceData(const int&);
    SHyprlandLayoutNodeData*                  getNodeFromWindowID(std::string const &windowID);
    void                              calculateWorkspace(PHLWORKSPACE);
    PHLWINDOW                          getNextWindow(PHLWINDOW, bool);
    int                               getMastersOnWorkspace(const int&);
     SHyprlandLayoutNodeData*         getMasterNodeOnWorkspace(const int& ws);
    bool                              prepareLoseFocus(PHLWINDOW);
    void                              prepareNewFocus(PHLWINDOW, bool inherit_fullscreen);
    void                              moveCWindowToEnd(PHLWINDOW win);

    friend struct SHyprlandLayoutNodeData;
    friend struct SHyprlandLayoutWorkspaceData;
};


template <typename CharT>
struct std::formatter<SHyprlandLayoutNodeData*, CharT> : std::formatter<CharT> {
    template <typename FormatContext>
    auto format(const SHyprlandLayoutNodeData* const& node, FormatContext& ctx) const {
        auto out = ctx.out();
        if (!node)
            return std::format_to(out, "[Node nullptr]");
        std::format_to(out, "[Node {:x}: workspace: {}, pos: {:j2}, size: {:j2}", (uintptr_t)node, node->workspaceID, node->position, node->size);
        if (!node->pWindow.expired())
            std::format_to(out, ", window: {:x}", node->pWindow.lock());
        return std::format_to(out, "]");
    }
};
