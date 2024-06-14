#define WLR_USE_UNSTABLE
#include <hyprland/src/Compositor.hpp>

PHLWINDOW hkvectorToWindowUnified(void *thisptr, const Vector2D& pos, uint8_t properties, PHLWINDOW pIgnoreWindow);


inline CFunctionHook *g_pvectorToWindowUnifiedHook;



