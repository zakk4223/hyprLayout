#pragma once

#define WLR_USE_UNSTABLE

#include "globals.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <src/layout/IHyprLayout.hpp>
#include <vector>
#include <list>
#include <deque>
#include <any>

enum eFullscreenMode : uint8_t;

struct SHyprlandLayoutNodeData {

    uint32_t requestSerial;
    bool     requestDone;
    uint32_t requestIdx;
    bool     isMaster = false;
    bool     userModified = false;
    CWindow* pWindow = nullptr;
    std::string windowID;

    Vector2D position;
    Vector2D size;

    float    percSize = 1.f; // size multiplier for resizing children

    int      workspaceID = -1;

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
    virtual void                     onWindowCreatedTiling(CWindow*);
    virtual void                     onWindowRemovedTiling(CWindow*);
    virtual bool                     isWindowTiled(CWindow*);
    virtual void                     recalculateMonitor(const int&);
    virtual void                     recalculateMonitor(const int &monid, bool commit);
    virtual void                     recalculateWindow(CWindow*);
    virtual void                     resizeActiveWindow(const Vector2D&, CWindow* pWindow = nullptr);
    virtual void                     fullscreenRequestForWindow(CWindow*, eFullscreenMode, bool);
    virtual std::any                 layoutMessage(SLayoutMessageHeader, std::string);
    virtual SWindowRenderLayoutHints requestRenderHints(CWindow*);
    virtual void                     switchWindows(CWindow*, CWindow*);
    virtual void                     alterSplitRatio(CWindow*, float, bool);
    virtual std::string              getLayoutName();
    virtual void                     replaceWindowDataWith(CWindow* from, CWindow* to);

    virtual void                     onEnable();
    virtual void                     onDisable();
    void 			     hyprlandLayoutWindowDimensions(const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t reject, uint32_t serial);
    void			     hyprlandLayoutCommit(const char *layout_name, const char *config_data, uint32_t serial);
    void           workspaceDestroyed(const int& ws);


        



  private:
    std::list<SHyprlandLayoutNodeData>        m_lHyprlandLayoutNodesData;
    std::vector<SHyprlandLayoutWorkspaceData> m_lHyprlandLayoutWorkspacesData;
    wl_resource                     *m_pWLResource;
    Vector2D 			   m_vRemovedWindowVector;
    uint32_t         m_iLastLayoutSerial = 0;
    

    bool                              m_bForceWarps = false;

    int                               getNodesOnWorkspace(const int&);
    void                              applyNodeDataToWindow(SHyprlandLayoutNodeData*);
    void                              resetNodeSplits(const int&);
    SHyprlandLayoutNodeData*                  getNodeFromWindow(CWindow*);
    SHyprlandLayoutWorkspaceData*             getMasterWorkspaceData(const int&);
    SHyprlandLayoutNodeData*                  getNodeFromWindowID(std::string const &windowID);
    void                              calculateWorkspace(const int&);
    CWindow*                          getNextWindow(CWindow*, bool);
    int                               getMastersOnWorkspace(const int&);
     SHyprlandLayoutNodeData*         getMasterNodeOnWorkspace(const int& ws);
    bool                              prepareLoseFocus(CWindow*);
    void                              prepareNewFocus(CWindow*, bool inherit_fullscreen);

    friend struct SHyprlandLayoutNodeData;
    friend struct SHyprlandLayoutWorkspaceData;
};

