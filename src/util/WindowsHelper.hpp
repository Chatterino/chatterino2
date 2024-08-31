#pragma once

#ifdef USEWINSDK

#    include <QString>
#    include <Windows.h>

#    include <optional>

namespace chatterino {

enum class AssociationQueryType { Protocol, FileExtension };

std::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

QString getAssociatedExecutable(AssociationQueryType queryType, LPCWSTR query);

}  // namespace chatterino

#endif
