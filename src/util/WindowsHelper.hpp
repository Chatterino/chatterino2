#pragma once

#ifdef USEWINSDK

#    include <boost/optional.hpp>
#    include <QString>
#    include <Windows.h>

namespace chatterino {

enum class AssociationQueryType { Protocol, FileExtension };

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

QString getAssociatedCommand(AssociationQueryType queryType, LPCWSTR query);

}  // namespace chatterino

#endif
