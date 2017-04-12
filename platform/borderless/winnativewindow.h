#ifndef WINNATIVEWINDOW_H
#define WINNATIVEWINDOW_H

#include "Windows.h"
#include "Windowsx.h"

#include <QWidget>

class WinNativeWindow
{
public:
    WinNativeWindow(const int x, const int y, const int width, const int height);
    ~WinNativeWindow();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // These six functions exist to restrict native window resizing to whatever
    // you want your app minimum/maximum size to be
    void setMinimumSize(const int width, const int height);
    int getMinimumHeight();
    int getMinimumWidth();

    void setMaximumSize(const int width, const int height);
    int getMaximumHeight();
    int getMaximumWidth();
    void setGeometry(const int x, const int y, const int width, const int height);

    HWND hWnd;

    static HWND childWindow;
    static QWidget *childWidget;

private:
    struct sizeType {
        sizeType()
            : required(false)
            , width(0)
            , height(0)
        {
        }
        bool required;
        int width;
        int height;
    };

    sizeType minimumSize;
    sizeType maximumSize;

    DWORD aero_borderless = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                            WS_THICKFRAME | WS_CLIPCHILDREN;
};

#endif  // WINNATIVEWINDOW_H
