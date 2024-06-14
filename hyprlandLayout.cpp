#include "hyprlandLayout.hpp"
#include "HyprlandLayoutProtocolManager.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/decorations/CHyprGroupBarDecoration.hpp>
#include <numeric>
#include <ranges>
#include <cmath>




CHyprlandLayout::CHyprlandLayout(const char *r_namespace, wl_resource *resource) {

  m_pWLResource = resource;
	m_sHyprlandLayoutNamespace = r_namespace;

}

SHyprlandLayoutNodeData* CHyprlandLayout::getNodeFromWindowID(std::string const &windowID) {
    for (auto& nd : m_lHyprlandLayoutNodesData) {
        if (nd.windowID ==  windowID)
            return &nd;
    }

    return nullptr;
}

SHyprlandLayoutNodeData* CHyprlandLayout::getNodeFromWindow(PHLWINDOW pWindow) {
    for (auto& nd : m_lHyprlandLayoutNodesData) {
        if (nd.pWindow == pWindow)
            return &nd;
    }

    return nullptr;
}

int CHyprlandLayout::getNodesOnWorkspace(const int& ws) {
    int no = 0;
    for (auto& n : m_lHyprlandLayoutNodesData) {
        if (n.workspaceID == ws)
            no++;
    }

    return no;
}


int CHyprlandLayout::getMastersOnWorkspace(const int& ws) {
    int no = 0;
    for (auto& n : m_lHyprlandLayoutNodesData) {
        if (n.workspaceID == ws && n.isMaster)
            no++;
    }
    return no;
}

SHyprlandLayoutNodeData* CHyprlandLayout::getMasterNodeOnWorkspace(const int& ws) {
    for (auto& n : m_lHyprlandLayoutNodesData) {
        if (n.isMaster && n.workspaceID == ws)
            return &n;
    }

    return nullptr;
}

SHyprlandLayoutWorkspaceData* CHyprlandLayout::getLayoutWorkspaceData(const int& ws) {
    for (auto& n : m_lHyprlandLayoutWorkspacesData) {
        if (n.workspaceID == ws)
            return &n;
    }

    //create on the fly if it doesn't exist yet
    const auto PWORKSPACEDATA   = &m_lHyprlandLayoutWorkspacesData.emplace_back();
    PWORKSPACEDATA->workspaceID = ws;
    return PWORKSPACEDATA;
}

void CHyprlandLayout::workspaceDestroyed(const int& ws) {

    for (auto& n : m_lHyprlandLayoutWorkspacesData) {
        if (n.workspaceID == ws)
            n.layoutConfigData = "";
    }

}

std::string CHyprlandLayout::getLayoutName() {
    return m_sHyprlandLayoutNamespace; 
}

void CHyprlandLayout::moveWindowTo(PHLWINDOW pWindow, const std::string& dir, bool silent) {
    if (!isDirection(dir))
        return;

    const auto PWINDOW2 = g_pCompositor->getWindowInDirection(pWindow, dir[0]);
		if (pWindow->m_pWorkspace != PWINDOW2->m_pWorkspace) {
 		  // if different monitors, send to monitor
		  onWindowRemovedTiling(pWindow);
		  pWindow->moveToWorkspace(PWINDOW2->m_pWorkspace);
		  if (!silent) {
		    const auto pMonitor = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);
			  g_pCompositor->setActiveMonitor(pMonitor);
		  }
		  onWindowCreatedTiling(pWindow);
    } else {
        // if same monitor, switch windows
        switchWindows(pWindow, PWINDOW2);
		    if (silent)
		      g_pCompositor->focusWindow(PWINDOW2);
		}
}

void CHyprlandLayout::onWindowCreatedTiling(PHLWINDOW pWindow, eDirection direction) {
    m_vRemovedWindowVector = Vector2D(0.f, 0.f);
    if (pWindow->m_bIsFloating) {
        return;
    }


    //static auto* const PNEWTOP = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:river:layout:new_on_top")->intValue;

    const auto         PMONITOR = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);

    //const auto         PNODE = *PNEWTOP ? &m_lMasterNodesData.emplace_front() : &m_lMasterNodesData.emplace_back();
    const auto PNODE = &m_lHyprlandLayoutNodesData.emplace_front();

    PNODE->workspaceID = pWindow->workspaceID();
    PNODE->pWindow     = pWindow;
    char windowSTR[20];
    snprintf(windowSTR, sizeof(windowSTR), "%p", pWindow.get());
    PNODE->windowID = windowSTR;



    //static auto* const PNEWISMASTER = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:river:layout:new_is_master")->intValue;

    const auto         WINDOWSONWORKSPACE = getNodesOnWorkspace(PNODE->workspaceID);

    auto               OPENINGON = getNodeFromWindow(g_pCompositor->m_pLastWindow.lock());

		const auto MOUSECOORDS = g_pInputManager->getMouseCoordsInternal();


		if (g_pInputManager->m_bWasDraggingWindow && OPENINGON) {
			if (OPENINGON->pWindow->checkInputOnDecos(INPUT_TYPE_DRAG_END, MOUSECOORDS, pWindow))
				return;
		}

    if (OPENINGON && OPENINGON != PNODE && OPENINGON->pWindow->m_sGroupData.pNextWindow // target is group
        && pWindow->canBeGroupedInto(OPENINGON->pWindow.lock())) {

        if (!pWindow->m_sGroupData.pNextWindow)
            pWindow->m_dWindowDecorations.emplace_back(std::make_unique<CHyprGroupBarDecoration>(pWindow));

        m_lHyprlandLayoutNodesData.remove(*PNODE);

        static const auto* USECURRPOS = (Hyprlang::INT * const*)g_pConfigManager->getConfigValuePtr("group:insert_after_current");
        (*USECURRPOS ? OPENINGON->pWindow : OPENINGON->pWindow->getGroupTail())->insertWindowToGroup(pWindow);

        OPENINGON->pWindow->setGroupCurrent(pWindow);
        pWindow->applyGroupRules();
        pWindow->updateWindowDecos();
        recalculateWindow(pWindow);

        return;
    }

    if (WINDOWSONWORKSPACE == 1) {
        // first, check if it isn't too big.

        if (const auto MAXSIZE = g_pXWaylandManager->getMaxSizeForWindow(pWindow); MAXSIZE.x < PMONITOR->vecSize.x || MAXSIZE.y < PMONITOR->vecSize.y) {
            // we can't continue. make it floating.
            pWindow->m_bIsFloating = true;
            m_lHyprlandLayoutNodesData.remove(*PNODE);
            g_pLayoutManager->getCurrentLayout()->onWindowCreatedFloating(pWindow);
            return;
        }
    } else {
        // first, check if it isn't too big.
        if (const auto MAXSIZE = g_pXWaylandManager->getMaxSizeForWindow(pWindow);
            MAXSIZE.x < PMONITOR->vecSize.x || MAXSIZE.y < PMONITOR->vecSize.y * (1.f / (WINDOWSONWORKSPACE - 1))) {
            // we can't continue. make it floating.
            pWindow->m_bIsFloating = true;
            m_lHyprlandLayoutNodesData.remove(*PNODE);
            g_pLayoutManager->getCurrentLayout()->onWindowCreatedFloating(pWindow);
            return;
        }
    }

    // recalc
    //TODO: handle new is master style config. Also handle add to master etc
    for (auto& nd : m_lHyprlandLayoutNodesData) {
      if (nd.isMaster && nd.workspaceID == PNODE->workspaceID) {
        nd.isMaster = false;
      }
    }

    PNODE->isMaster = true;
    recalculateMonitor(pWindow->m_iMonitorID);
}

void CHyprlandLayout::onWindowRemovedTiling(PHLWINDOW pWindow) {
    const auto PNODE = getNodeFromWindow(pWindow);

    if (!PNODE)
        return;

    const auto WORKSPACEID = PNODE->workspaceID;
    const auto MASTERSLEFT = getMastersOnWorkspace(WORKSPACEID);

    m_vRemovedWindowVector = Vector2D(0.f, 0.f);
		pWindow->updateSpecialRenderData();

    if (PNODE->isMaster && MASTERSLEFT <= 1) {
      for (auto& nd : m_lHyprlandLayoutNodesData) {
        if (!nd.isMaster && nd.workspaceID == WORKSPACEID) {
          nd.isMaster = true;
          break;
        }
      }
    }
    if (pWindow->m_bIsFullscreen)
        g_pCompositor->setWindowFullscreen(pWindow, false, FULLSCREEN_FULL);

    m_lHyprlandLayoutNodesData.remove(*PNODE);

    m_vRemovedWindowVector = pWindow->m_vRealPosition.goal() + pWindow->m_vRealSize.goal() / 2.f;



    recalculateMonitor(pWindow->m_iMonitorID);
}

void CHyprlandLayout::recalculateMonitor(const int &monid) {
  recalculateMonitor(monid, true);
}


void CHyprlandLayout::recalculateMonitor(const int &monid, bool commit) {
    const auto PMONITOR   = g_pCompositor->getMonitorFromID(monid);
    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(PMONITOR->activeWorkspace);

    if (!PWORKSPACE)
        return;

    m_iLastLayoutSerial++;

    g_pHyprRenderer->damageMonitor(PMONITOR);

    if (PMONITOR->activeSpecialWorkspace) {
        calculateWorkspace(PMONITOR->activeSpecialWorkspace);
    }

    if (PWORKSPACE->m_bHasFullscreenWindow) {
        if (PWORKSPACE->m_efFullscreenMode == FULLSCREEN_FULL)
            return;

        // massive hack from the fullscreen func
        const auto      PFULLWINDOW = g_pCompositor->getFullscreenWindowOnWorkspace(PWORKSPACE->m_iID);

        SHyprlandLayoutNodeData fakeNode;
        fakeNode.pWindow         = PFULLWINDOW;
        fakeNode.position        = PMONITOR->vecPosition + PMONITOR->vecReservedTopLeft;
        fakeNode.size            = PMONITOR->vecSize - PMONITOR->vecReservedTopLeft - PMONITOR->vecReservedBottomRight;
        fakeNode.workspaceID     = PWORKSPACE->m_iID;
        PFULLWINDOW->m_vPosition = fakeNode.position;
        PFULLWINDOW->m_vSize     = fakeNode.size;

        applyNodeDataToWindow(&fakeNode);

        return;
    }

    // calc the WS
    calculateWorkspace(PWORKSPACE);
    if (commit) {
      g_pHyprlandLayoutProtocolManager->sendLayoutDemandCommit(m_pWLResource, m_iLastLayoutSerial); 
    }
}

void CHyprlandLayout::calculateWorkspace(PHLWORKSPACE pWorkspace) {

    if (!pWorkspace)
        return;

    const auto         PWORKSPACEDATA = getLayoutWorkspaceData(pWorkspace);
    const auto         PMONITOR = g_pCompositor->getMonitorFromID(pWorkspace->m_iMonitorID);
    const uint32_t num_nodes = getNodesOnWorkspace(pWorkspace->m_iID);



     g_pHyprlandLayoutProtocolManager->sendLayoutDemand(m_pWLResource, PMONITOR->vecSize.x - PMONITOR->vecReservedBottomRight.x - PMONITOR->vecReservedTopLeft.x, PMONITOR->vecSize.y - PMONITOR->vecReservedBottomRight.y - PMONITOR->vecReservedTopLeft.y, pWorkspace->m_iID,num_nodes, m_iLastLayoutSerial);
    if (PWORKSPACEDATA) {
      g_pHyprlandLayoutProtocolManager->sendLayoutDemandConfig(m_pWLResource, PWORKSPACEDATA->layoutConfigData.c_str(), m_iLastLayoutSerial); 
    }
    //record serial so we know if we're done

    for (auto& nd : m_lHyprlandLayoutNodesData) {
	    if (nd.workspaceID != pWorkspace->m_iID)
		    continue;
	    nd.requestSerial = m_iLastLayoutSerial;
	    nd.requestDone = false;
			Debug::log(LOG, "Sending window info ID: {} X: {} Y: {} W: {} H: {}, MASTER: {} ACTIVE: {}, MOD: {}, SERIAL: {}", nd.windowID.c_str(), nd.position.x, nd.position.y, nd.size.x, nd.size.y, nd.isMaster, g_pCompositor->isWindowActive(nd.pWindow.lock()), nd.userModified, m_iLastLayoutSerial);
			const auto tags = nd.pWindow.lock()->m_tags.getTags();
			std::string tagStr = std::accumulate(tags.begin(), tags.end(), std::string(), [](const std::string& a, const std::string& b) { return a.empty() ? b : a + ", " + b; });
      g_pHyprlandLayoutProtocolManager->sendWindowInfo(m_pWLResource, nd.windowID.c_str(), (int32_t)nd.position.x, (int32_t)nd.position.y, (int32_t)nd.size.x, (int32_t)nd.size.y, nd.isMaster, g_pCompositor->isWindowActive(nd.pWindow.lock()), nd.userModified, tagStr.c_str(), m_iLastLayoutSerial);
    }

    //Layout commit is done externally (in either calculateWorkspace or layoutMessage)
}

void CHyprlandLayout::hyprlandLayoutWindowDimensions(const char *window_id, uint32_t window_index, int32_t x, int32_t y, uint32_t width, uint32_t height,uint32_t serial)
{
  //TODO: handle reject-float

  std::string windowSTR = window_id;
  auto nd = getNodeFromWindowID(windowSTR);
  if (!nd) {
    return;
  }

		const auto PMONITOR = g_pCompositor->getMonitorFromID(nd->pWindow->m_iMonitorID);
		nd->position = PMONITOR->vecPosition  + PMONITOR->vecReservedTopLeft + Vector2D(x+0.0f, y+0.0f);
		nd->size = Vector2D(width+0.0f, height+0.0f);
		nd->requestDone = true;
    nd->requestIdx = window_index;
}

void CHyprlandLayout::hyprlandLayoutCommit(const char *layout_name, const char *config_data, uint32_t serial) {

  SHyprlandLayoutWorkspaceData* WSDATA = nullptr;
  //First sort the node data by the (possibly) new indices given to us from the layout client.


  m_lHyprlandLayoutNodesData.sort([](const SHyprlandLayoutNodeData &a, const SHyprlandLayoutNodeData &b) {return a.requestIdx < b.requestIdx;});

	for (auto &nd : m_lHyprlandLayoutNodesData) {
		if (nd.requestSerial != serial)
			continue;
		if (!nd.requestDone)
			continue;

		nd.requestDone = false;
		nd.requestSerial = 0;
    nd.requestIdx = 0;
    nd.userModified = 0;
		applyNodeDataToWindow(&nd);
    if (WSDATA == nullptr) {
      WSDATA = getLayoutWorkspaceData(nd.workspaceID);
      WSDATA->layoutConfigData = config_data;
    }
	}
        
	if (m_vRemovedWindowVector != Vector2D(0.f, 0.f)) {
		const auto FOCUSCANDIDATE = g_pCompositor->vectorToWindowUnified(m_vRemovedWindowVector, FULL_EXTENTS | ALLOW_FLOATING);
		if (FOCUSCANDIDATE) {
			g_pCompositor->focusWindow(FOCUSCANDIDATE);
		}

		m_vRemovedWindowVector = Vector2D(0.f, 0.f);
	}
}

void CHyprlandLayout::applyNodeDataToWindow(SHyprlandLayoutNodeData* pNode) {
    CMonitor* PMONITOR = nullptr;

    if (g_pCompositor->isWorkspaceSpecial(pNode->workspaceID)) {
        for (auto& m : g_pCompositor->m_vMonitors) {
            if (m->activeSpecialWorkspaceID() == pNode->workspaceID) {
                PMONITOR = m.get();
                break;
            }
        }
    } else {
        PMONITOR = g_pCompositor->getMonitorFromID(g_pCompositor->getWorkspaceByID(pNode->workspaceID)->m_iMonitorID);
    }

    if (!PMONITOR) {
        Debug::log(ERR, "Orphaned Node {} (workspace ID: {})!!", static_cast<void *>(pNode), pNode->workspaceID);
        return;
    }

    // for gaps outer
    const bool DISPLAYLEFT   = STICKS(pNode->position.x, PMONITOR->vecPosition.x + PMONITOR->vecReservedTopLeft.x);
    const bool DISPLAYRIGHT  = STICKS(pNode->position.x + pNode->size.x, PMONITOR->vecPosition.x + PMONITOR->vecSize.x - PMONITOR->vecReservedBottomRight.x);
    const bool DISPLAYTOP    = STICKS(pNode->position.y, PMONITOR->vecPosition.y + PMONITOR->vecReservedTopLeft.y);
    const bool DISPLAYBOTTOM = STICKS(pNode->position.y + pNode->size.y, PMONITOR->vecPosition.y + PMONITOR->vecSize.y - PMONITOR->vecReservedBottomRight.y);

    static auto* const PGAPSINDATA     = (Hyprlang::CUSTOMTYPE* const*)g_pConfigManager->getConfigValuePtr("general:gaps_in");
    static auto* const PGAPSOUTDATA    = (Hyprlang::CUSTOMTYPE* const*)g_pConfigManager->getConfigValuePtr("general:gaps_out");
    auto* const        PGAPSIN         = (CCssGapData*)(*PGAPSINDATA)->getData();
    auto* const        PGAPSOUT        = (CCssGapData*)(*PGAPSOUTDATA)->getData();
		static auto* const PANIMATE = (Hyprlang::INT* const*)g_pConfigManager->getConfigValuePtr("misc:animate_manual_resizes");

    const auto PWINDOW = pNode->pWindow.lock();
		const auto WORKSPACERULE = g_pConfigManager->getWorkspaceRuleFor(g_pCompositor->getWorkspaceByID(PWINDOW->workspaceID()));

		PWINDOW->updateSpecialRenderData();

		auto gapsIn = WORKSPACERULE.gapsIn.value_or(*PGAPSIN);
		auto gapsOut = WORKSPACERULE.gapsOut.value_or(*PGAPSOUT);

    if (!validMapped(PWINDOW)) {
        Debug::log(ERR, "Node {} holding invalid window {}!!", pNode, PWINDOW);
        return;
    }

    PWINDOW->m_vSize     = pNode->size;
    PWINDOW->m_vPosition = pNode->position;

    auto calcPos  = PWINDOW->m_vPosition;
    auto calcSize = PWINDOW->m_vSize;

    const auto OFFSETTOPLEFT = Vector2D(DISPLAYLEFT ? gapsOut.left : gapsIn.left, DISPLAYTOP ? gapsOut.top : gapsIn.top);

    const auto OFFSETBOTTOMRIGHT = Vector2D(DISPLAYRIGHT ? gapsOut.right : gapsIn.right, DISPLAYBOTTOM ? gapsOut.bottom : gapsIn.bottom);

    calcPos  = calcPos + OFFSETTOPLEFT;
    calcSize = calcSize - OFFSETTOPLEFT - OFFSETBOTTOMRIGHT;

    const auto RESERVED = PWINDOW->getFullWindowReservedArea();
    calcPos             = calcPos + RESERVED.topLeft;
    calcSize            = calcSize - (RESERVED.topLeft + RESERVED.bottomRight);

    /*if (g_pCompositor->isWorkspaceSpecial(PWINDOW->m_iWorkspaceID)) {
        static auto* const PSCALEFACTOR = &g_pConfigManager->getConfigValuePtr("master:special_scale_factor")->floatValue;

        PWINDOW->m_vRealPosition = calcPos + (calcSize - calcSize * *PSCALEFACTOR) / 2.f;
        PWINDOW->m_vRealSize     = calcSize * *PSCALEFACTOR;

        g_pXWaylandManager->setWindowSize(PWINDOW, calcSize * *PSCALEFACTOR);
    } else {*/
				CBox wb = {calcPos, calcSize};
				wb.round();
        PWINDOW->m_vRealSize     = wb.size();
        PWINDOW->m_vRealPosition = wb.pos();;

        g_pXWaylandManager->setWindowSize(PWINDOW, wb.size());
    //}

    if (m_bForceWarps && !**PANIMATE) {
        g_pHyprRenderer->damageWindow(PWINDOW);

        PWINDOW->m_vRealPosition.warp();
        PWINDOW->m_vRealSize.warp();

        g_pHyprRenderer->damageWindow(PWINDOW);
    }

    PWINDOW->updateWindowDecos();
}

bool CHyprlandLayout::isWindowTiled(PHLWINDOW pWindow) {
    return getNodeFromWindow(pWindow) != nullptr;
}

void CHyprlandLayout::resizeActiveWindow(const Vector2D& pixResize, eRectCorner corner, PHLWINDOW pWindow) {

    const auto PWINDOW = pWindow ? pWindow : g_pCompositor->m_pLastWindow.lock();


    if (!validMapped(PWINDOW))
        return;

    const auto PNODE = getNodeFromWindow(PWINDOW);

    if (!PNODE) {
        PWINDOW->m_vRealSize = Vector2D(std::max((PWINDOW->m_vRealSize.goal() + pixResize).x, 20.0), std::max((PWINDOW->m_vRealSize.goal() + pixResize).y, 20.0));
        PWINDOW->updateWindowDecos();
        return;
    }

    Vector2D newSize = Vector2D(PNODE->size.x, PNODE->size.y);
    Vector2D newPos = Vector2D(PNODE->position.x, PNODE->position.y);

    //Thanks to hy3 for showing me how to get the 'drag side'
		if (g_pInputManager->currentlyDraggedWindow == PWINDOW) {
			auto mouse = g_pInputManager->getMouseCoordsInternal();
			auto mouse_offset = mouse - PWINDOW->m_vPosition;
      bool xleft = mouse_offset.x < PWINDOW->m_vSize.x / 2;
      bool ytop = mouse_offset.y > PWINDOW->m_vSize.y / 2;
      if (xleft) {
        newPos.x += pixResize.x;
        newSize.x -= pixResize.x;
      } else {
        newSize.x += pixResize.x;
      }

      if (ytop) {
        newPos.y += pixResize.y;
        newSize.y -= pixResize.y;
      } else {
        newSize.y += pixResize.y;
      }
    } else {
      newSize.x += pixResize.x;
      newSize.y += pixResize.y;
    }

    
      recalculateMonitor(pWindow->m_iMonitorID, false);
      g_pHyprlandLayoutProtocolManager->sendLayoutDemandResize(m_pWLResource, PNODE->windowID.c_str(), (int32_t)(newPos.x - PNODE->position.x), (int32_t)(newPos.y - PNODE->position.y), (int32_t)(newSize.x - PNODE->size.x), (int32_t)(newSize.y - PNODE->size.y), m_iLastLayoutSerial);
      g_pHyprlandLayoutProtocolManager->sendLayoutDemandCommit(m_pWLResource, m_iLastLayoutSerial); 
}


void CHyprlandLayout::fullscreenRequestForWindow(PHLWINDOW pWindow, eFullscreenMode fullscreenMode, bool on) {
    if (!validMapped(pWindow))
        return;

    if (on == pWindow->m_bIsFullscreen || g_pCompositor->isWorkspaceSpecial(pWindow->workspaceID()))
        return; // ignore

    const auto PMONITOR   = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);
    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(pWindow->workspaceID());

    if (PWORKSPACE->m_bHasFullscreenWindow && on) {
        // if the window wants to be fullscreen but there already is one,
        // ignore the request.
        return;
    }

    // otherwise, accept it.
    pWindow->m_bIsFullscreen           = on;
    PWORKSPACE->m_bHasFullscreenWindow = !PWORKSPACE->m_bHasFullscreenWindow;

		pWindow->updateDynamicRules();
		pWindow->updateWindowDecos();
    g_pEventManager->postEvent(SHyprIPCEvent{"fullscreen", std::to_string((int)on)});
    EMIT_HOOK_EVENT("fullscreen", pWindow);

    if (!pWindow->m_bIsFullscreen) {
        // if it got its fullscreen disabled, set back its node if it had one
        const auto PNODE = getNodeFromWindow(pWindow);
        if (PNODE)
            applyNodeDataToWindow(PNODE);
        else {
            // get back its' dimensions from position and size
            pWindow->m_vRealPosition = pWindow->m_vLastFloatingPosition;
            pWindow->m_vRealSize     = pWindow->m_vLastFloatingSize;

            pWindow->m_sSpecialRenderData.rounding = true;
            pWindow->m_sSpecialRenderData.border   = true;
            pWindow->m_sSpecialRenderData.decorate = true;
        }
    } else {
        // if it now got fullscreen, make it fullscreen

        PWORKSPACE->m_efFullscreenMode = fullscreenMode;

        // save position and size if floating
        if (pWindow->m_bIsFloating) {
            pWindow->m_vLastFloatingSize     = pWindow->m_vRealSize.goal();
            pWindow->m_vLastFloatingPosition = pWindow->m_vRealPosition.goal();
            pWindow->m_vPosition             = pWindow->m_vRealPosition.goal();
            pWindow->m_vSize                 = pWindow->m_vRealSize.goal();
        }

        // apply new pos and size being monitors' box
        if (fullscreenMode == FULLSCREEN_FULL) {
            pWindow->m_vRealPosition = PMONITOR->vecPosition;
            pWindow->m_vRealSize     = PMONITOR->vecSize;
        } else {
            // This is a massive hack.
            // We make a fake "only" node and apply
            // To keep consistent with the settings without C+P code

            SHyprlandLayoutNodeData fakeNode;
            fakeNode.pWindow     = pWindow;
            fakeNode.position    = PMONITOR->vecPosition + PMONITOR->vecReservedTopLeft;
            fakeNode.size        = PMONITOR->vecSize - PMONITOR->vecReservedTopLeft - PMONITOR->vecReservedBottomRight;
            fakeNode.workspaceID = pWindow->workspaceID();
            pWindow->m_vPosition = fakeNode.position;
            pWindow->m_vSize     = fakeNode.size;

            applyNodeDataToWindow(&fakeNode);
        }
    }

    g_pCompositor->updateWindowAnimatedDecorationValues(pWindow);

    g_pXWaylandManager->setWindowSize(pWindow, pWindow->m_vRealSize.goal());

    g_pCompositor->changeWindowZOrder(pWindow, true);

	recalculateMonitor(PMONITOR->ID);
}

void CHyprlandLayout::recalculateWindow(PHLWINDOW pWindow) {
    const auto PNODE = getNodeFromWindow(pWindow);

    if (!PNODE)
        return;

    recalculateMonitor(pWindow->m_iMonitorID);
}

SWindowRenderLayoutHints CHyprlandLayout::requestRenderHints(PHLWINDOW pWindow) {
    // window should be valid, insallah

    SWindowRenderLayoutHints hints;

    return hints; // master doesnt have any hints
}

void CHyprlandLayout::switchWindows(PHLWINDOW pWindow, PHLWINDOW pWindow2) {
    // windows should be valid, insallah

    const auto PNODE  = getNodeFromWindow(pWindow);
    const auto PNODE2 = getNodeFromWindow(pWindow2);

    if (!PNODE2 || !PNODE)
        return;

    const auto inheritFullscreen = prepareLoseFocus(pWindow);

    if (PNODE->workspaceID != PNODE2->workspaceID) {
        std::swap(pWindow2->m_iMonitorID, pWindow->m_iMonitorID);
        std::swap(pWindow2->m_pWorkspace, pWindow->m_pWorkspace);
    }

    // massive hack: just swap window pointers, lol
    PNODE->pWindow  = pWindow2;
    PNODE2->pWindow = pWindow;

    recalculateMonitor(pWindow->m_iMonitorID);
    if (PNODE2->workspaceID != PNODE->workspaceID)
        recalculateMonitor(pWindow2->m_iMonitorID);

    g_pHyprRenderer->damageWindow(pWindow);
    g_pHyprRenderer->damageWindow(pWindow2);

    prepareNewFocus(pWindow2, inheritFullscreen);
}

void CHyprlandLayout::alterSplitRatio(PHLWINDOW pWindow, float ratio, bool exact) {
    recalculateMonitor(pWindow->m_iMonitorID);
}

PHLWINDOW CHyprlandLayout::getNextWindow(PHLWINDOW pWindow, bool next) {
    if (!isWindowTiled(pWindow))
        return nullptr;
    const auto PNODE = getNodeFromWindow(pWindow);

    auto       nodes = m_lHyprlandLayoutNodesData;
    if (!next)
        std::reverse(nodes.begin(), nodes.end());

    const auto NODEIT = std::find(nodes.begin(), nodes.end(), *PNODE);

    const bool ISMASTER = PNODE->isMaster;

    auto CANDIDATE = std::find_if(NODEIT, nodes.end(), [&](const auto& other) { return other != *PNODE && ISMASTER == other.isMaster && other.workspaceID == PNODE->workspaceID; });
    if (CANDIDATE == nodes.end())
        CANDIDATE =
            std::find_if(nodes.begin(), nodes.end(), [&](const auto& other) { return other != *PNODE && ISMASTER != other.isMaster && other.workspaceID == PNODE->workspaceID; });

    return CANDIDATE == nodes.end() ? nullptr : CANDIDATE->pWindow.lock();
}


bool CHyprlandLayout::prepareLoseFocus(PHLWINDOW pWindow) {
    if (!pWindow)
        return false;

    //if the current window is fullscreen, make it normal again if we are about to lose focus
    if (pWindow->m_bIsFullscreen) {
        g_pCompositor->setWindowFullscreen(pWindow, false, FULLSCREEN_FULL);
        //static auto* const INHERIT = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:nstack:layout:inherit_fullscreen")->intValue;
        //return *INHERIT == 1;
    }

    return false;
}

void CHyprlandLayout::prepareNewFocus(PHLWINDOW pWindow, bool inheritFullscreen) {
    if (!pWindow)
        return;

	  if (inheritFullscreen)
	    g_pCompositor->setWindowFullscreen(pWindow, true, g_pCompositor->getWorkspaceByID(pWindow->workspaceID())->m_efFullscreenMode);
}


std::any CHyprlandLayout::layoutMessage(SLayoutMessageHeader header, std::string message) {
    auto switchToWindow = [&](PHLWINDOW PWINDOWTOCHANGETO) {
        if (!validMapped(PWINDOWTOCHANGETO))
            return;

        g_pCompositor->focusWindow(PWINDOWTOCHANGETO);
        g_pCompositor->warpCursorTo(PWINDOWTOCHANGETO->middle());
    };
    CVarList vars(message, 0, ' ');

    if (vars.size() < 1 || vars[0].empty()) {
        Debug::log(ERR, "layoutmsg called without params");
        return 0;
    }

    auto command = vars[0];

    bool sendCommand = false;

    const auto PWINDOW = header.pWindow;

    // swapwithmaster <master | child | auto>
    // first message argument can have the following values:
    // * master - keep the focus at the new master
    // * child - keep the focus at the new child
    // * auto (default) - swap the focus (keep the focus of the previously selected window)
    if (command == "swapwithmaster") {
      if (!PWINDOW)
        return 0;
      if (!isWindowTiled(PWINDOW))
        return 0;

      const auto PMASTER = getMasterNodeOnWorkspace(PWINDOW->workspaceID());
      const auto NEWCHILD = PMASTER->pWindow.lock();
      if (PMASTER->pWindow != PWINDOW) {
        const auto NEWMASTER = PWINDOW;
        const bool newFocusToChild = vars.size() >= 2 && vars[1] == "child";
        const bool inheritFullscreen = prepareLoseFocus(NEWMASTER);
        switchWindows(NEWMASTER, NEWCHILD);
        const auto NEWFOCUS = newFocusToChild ? NEWCHILD : NEWMASTER;
        switchToWindow(NEWFOCUS);
        prepareNewFocus(NEWFOCUS, inheritFullscreen);
        } else {
            for (auto& n : m_lHyprlandLayoutNodesData) {
                if (n.workspaceID == PMASTER->workspaceID && !n.isMaster) {
                    const auto NEWMASTER         = n.pWindow.lock();
                    const bool inheritFullscreen = prepareLoseFocus(NEWCHILD);
                    switchWindows(NEWMASTER, NEWCHILD);
                    const bool newFocusToMaster = vars.size() >= 2 && vars[1] == "master";
                    const auto NEWFOCUS         = newFocusToMaster ? NEWMASTER : NEWCHILD;
                    switchToWindow(NEWFOCUS);
                    prepareNewFocus(NEWFOCUS, inheritFullscreen);
                    break;
                }
            }
        }
    // focusmaster <master | auto>
    // first message argument can have the following values:
    // * master - keep the focus at the new master, even if it was focused before
    // * auto (default) - swap the focus with the first child, if the current focus was master, otherwise focus master
    } else if (command == "focusmaster") {
        if (!PWINDOW)
            return 0;

        const bool inheritFullscreen = prepareLoseFocus(PWINDOW);

        const auto PMASTER = getMasterNodeOnWorkspace(PWINDOW->workspaceID());

        if (!PMASTER)
            return 0;

        if (PMASTER->pWindow.lock() != PWINDOW) {
            switchToWindow(PMASTER->pWindow.lock());
            prepareNewFocus(PMASTER->pWindow.lock() , inheritFullscreen);
        } else if (vars.size() >= 2 && vars[1] == "master") {
            return 0;
        } else {
            // if master is focused keep master focused (don't do anything)
            for (auto& n : m_lHyprlandLayoutNodesData) {
                if (n.workspaceID == PMASTER->workspaceID && !n.isMaster) {
                    switchToWindow(n.pWindow.lock());
                    prepareNewFocus(n.pWindow.lock(), inheritFullscreen);
                    break;
                }
            }
        }
    } else if (command == "cyclenext") {
        if (!PWINDOW)
            return 0;

        const bool inheritFullscreen = prepareLoseFocus(PWINDOW);

        const auto PNEXTWINDOW = getNextWindow(PWINDOW, true);
        switchToWindow(PNEXTWINDOW);
        prepareNewFocus(PNEXTWINDOW, inheritFullscreen);

    } else if (command == "cycleprev") {
        if (!PWINDOW)
            return 0;

        const bool inheritFullscreen = prepareLoseFocus(PWINDOW);

        const auto PPREVWINDOW = getNextWindow(PWINDOW, false);
        switchToWindow(PPREVWINDOW);
        prepareNewFocus(PPREVWINDOW, inheritFullscreen);
    } else if (command == "swapnext") {
        if (!PWINDOW)
          return 0;

        if (!validMapped(PWINDOW))
            return 0;

        if (PWINDOW->m_bIsFloating) {
            g_pKeybindManager->m_mDispatchers["swapnext"]("");
        }

        const auto PWINDOWTOSWAPWITH = getNextWindow(PWINDOW, true);

        if (PWINDOWTOSWAPWITH) {
            prepareLoseFocus(PWINDOW);
            switchWindows(PWINDOW, PWINDOWTOSWAPWITH);
            g_pCompositor->focusWindow(PWINDOW);
        }
    } else if (command == "swapprev") {
        if (!PWINDOW)
          return 0;
        if (!validMapped(PWINDOW))
            return 0;

        if (PWINDOW->m_bIsFloating) {
            g_pKeybindManager->m_mDispatchers["swapnext"]("prev");
        }

        const auto PWINDOWTOSWAPWITH = getNextWindow(PWINDOW, false);

        if (PWINDOWTOSWAPWITH) {
            prepareLoseFocus(PWINDOW);
            switchWindows(PWINDOW, PWINDOWTOSWAPWITH);
            g_pCompositor->focusWindow(PWINDOW);
        }
    } else if (command == "addmaster") {
      if (!PWINDOW)
        return 0;
      if (!validMapped(PWINDOW))
        return 0;
      if (PWINDOW->m_bIsFloating)
        return 0;

      const auto PNODE = getNodeFromWindow(PWINDOW);
      prepareLoseFocus(PWINDOW);
      if (!PNODE || PNODE->isMaster) {
        for (auto& n : m_lHyprlandLayoutNodesData) {
          if (n.workspaceID == PWINDOW->workspaceID()) {
            n.isMaster = true;
            break;
          }
        }
      } else {
        PNODE->isMaster = true;
      }
    } else if (command == "removemaster") {
        if (!PWINDOW)
          return 0;
        if (!validMapped(PWINDOW))
            return 0;

        if (PWINDOW->m_bIsFloating)
            return 0;

        const auto PNODE = getNodeFromWindow(PWINDOW);

        prepareLoseFocus(PWINDOW);

        if (!PNODE || !PNODE->isMaster) {
            // first non-master node
            for (auto& nd : m_lHyprlandLayoutNodesData | std::views::reverse) {
                if (nd.workspaceID == header.pWindow->workspaceID() && nd.isMaster) {
                    nd.isMaster = false;
                    break;
                }
            }
        } else {
            PNODE->isMaster = false;
        }
    } else if (command == "resetconfig") {
      const auto WSID = header.pWindow->workspaceID();
      auto WSDATA = getLayoutWorkspaceData(WSID);
      if (WSDATA != nullptr) {
        WSDATA->layoutConfigData = "";
      }
      sendCommand = true;
    } else {
      sendCommand = true;
    } 

    recalculateMonitor(header.pWindow->m_iMonitorID, false);
    if (sendCommand) {
      g_pHyprlandLayoutProtocolManager->sendUserCommand(m_pWLResource, message.c_str(), m_iLastLayoutSerial);
    }
    g_pHyprlandLayoutProtocolManager->sendLayoutDemandCommit(m_pWLResource, m_iLastLayoutSerial); 


    return 0;
}

			
void CHyprlandLayout::moveCWindowToEnd(PHLWINDOW win) {
  if (win && !win->m_bIsFloating) { //leave floating windows alone
    for (auto it = g_pCompositor->m_vWindows.begin(); it != g_pCompositor->m_vWindows.end(); ++it) {
      if (it->get() == win.get()) {
        std::rotate(it, it + 1, g_pCompositor->m_vWindows.end());
        break;
      }
    }
  }

}
void CHyprlandLayout::onWindowFocusChange(PHLWINDOW pNewFocus) {
  //Hax. Anytime a tiled window gets focus, just move it to the rear of the compositor's window list.
  //Tiled windows are rendered such that the last one rendered it 'on top', and we want the last focused
  //window on a workspace to remain on top even when it loses focus.
  //This is for layouts that created overlapping windows (like monocole)
  //We can't just call moveWindowToTop() since that has other side effects (I think)
  //So just twiddle the (public) window list...

  IHyprLayout::onWindowFocusChange(pNewFocus);
  moveCWindowToEnd(pNewFocus);
}


void CHyprlandLayout::replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) {
    const auto PNODE = getNodeFromWindow(from);

    if (!PNODE)
        return;

    PNODE->pWindow = to;
    //TODO: window ID

    applyNodeDataToWindow(PNODE);
}

void CHyprlandLayout::onEnable() {
    for (auto& w : g_pCompositor->m_vWindows) {
        if (w->m_bIsFloating || !w->m_bIsMapped || w->isHidden())
            continue;

        onWindowCreatedTiling(w);
						/*
        if (g_pCompositor->m_pLastWindow == w.get()) {
          moveCWindowToEnd(w.get());
        }*/
    }
}

void CHyprlandLayout::onDisable() {
    m_lHyprlandLayoutNodesData.clear();

}

Vector2D CHyprlandLayout::predictSizeForNewWindowTiled() {
	//What the fuck is this shit. Seriously.
	return {};
}

