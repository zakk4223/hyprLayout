#define WLR_USE_UNSTABLE
#include <hyprland/src/Compositor.hpp>

CWindow *hkvectorToWindowIdeal(void *thisptr, const Vector2D& pos);
CWindow *hkwindowFromCursor(void *thisptr);
CWindow* hkvectorToWindowTiled(void *thisptr, const Vector2D& pos);
CWindow* hkvectorToWindow(void *thisptr, const Vector2D& pos);


inline CFunctionHook *g_pvectorToWindowIdealHook;
inline CFunctionHook *g_pwindowFromCursorHook;
inline CFunctionHook *g_pvectorToWindowTiledHook;
inline CFunctionHook *g_pvectorToWindowHook;



