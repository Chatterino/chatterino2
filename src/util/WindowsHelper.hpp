#pragma once

#ifdef USEWINSDK

#    include <boost/optional.hpp>
#    include <Windows.h>

namespace chatterino {

typedef enum ASSOCIATION_QUERY_TYPE {
    AQT_PROTOCOL,
    AQT_FILE_EXTENSION
} ASSOCIATION_QUERY_TYPE;

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

QString getAssociatedCommand(ASSOCIATION_QUERY_TYPE queryType, LPCWSTR query);

}  // namespace chatterino

#endif
