/* ==================== DebugConfig.h ==================== */
/* AmbiSense Hub debug flags */
#pragma once

/* =============== DEBUG =============== */
#define DEBUG_ENABLED  1   // Master toggle

#if DEBUG_ENABLED   // EDIT BELOW
    #define DEBUG_NETWORK  0
    #define DEBUG_GEO      0
#else   // DO NOT EDIT BELOW
    #define DEBUG_NETWORK  0
    #define DEBUG_GEO      0
#endif