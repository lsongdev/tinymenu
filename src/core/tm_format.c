#include "core/tm_format.h"

char* tm_format_int(char* buf, size_t len, int32_t val) {
    snprintf(buf, len, "%" PRId32, val);
    return buf;
}

char* tm_format_fixed(char* buf, size_t len, int32_t val, tm_decimals decimals) {
    switch (decimals) {
    case TM_DECIMALS_1:
        snprintf(buf, len, "%.1f", val / 10.0f);
        break;
    case TM_DECIMALS_2:
        snprintf(buf, len, "%.2f", val / 100.0f);
        break;
    case TM_DECIMALS_3:
        snprintf(buf, len, "%.3f", val / 1000.0f);
        break;
    default:
        tm_format_int(buf, len, val);
        break;
    }
    return buf;
}

char* tm_format_general(char* buf, size_t len, int32_t val, tm_decimals decimals) {
    switch (decimals) {
    case TM_DECIMALS_1:
        snprintf(buf, len, "%g", val / 10.0f);
        break;
    case TM_DECIMALS_2:
        snprintf(buf, len, "%g", val / 100.0f);
        break;
    case TM_DECIMALS_3:
        snprintf(buf, len, "%g", val / 1000.0f);
        break;
    default:
        tm_format_int(buf, len, val);
        break;
    }
    return buf;
}
