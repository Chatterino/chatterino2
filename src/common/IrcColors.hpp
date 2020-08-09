#pragma once

#include <QColor>
#include <QMap>

namespace chatterino {

// Colors taken from https://modern.ircdocs.horse/formatting.html
static QMap<int, QColor> IRC_COLORS = {
    {0, QColor("white")},      {1, QColor("black")},
    {2, QColor("blue")},       {3, QColor("green")},
    {4, QColor("red")},        {5, QColor("brown")},
    {6, QColor("purple")},     {7, QColor("orange")},
    {8, QColor("yellow")},     {9, QColor("lightgreen")},
    {10, QColor("cyan")},      {11, QColor("lightcyan")},
    {12, QColor("lightblue")}, {13, QColor("pink")},
    {14, QColor("gray")},      {15, QColor("lightgray")},
    {16, QColor("#470000")},   {17, QColor("#472100")},
    {18, QColor("#474700")},   {19, QColor("#324700")},
    {20, QColor("#004700")},   {21, QColor("#00472c")},
    {22, QColor("#004747")},   {23, QColor("#002747")},
    {24, QColor("#000047")},   {25, QColor("#2e0047")},
    {26, QColor("#470047")},   {27, QColor("#47002a")},
    {28, QColor("#740000")},   {29, QColor("#743a00")},
    {30, QColor("#747400")},   {31, QColor("#517400")},
    {32, QColor("#007400")},   {33, QColor("#007449")},
    {34, QColor("#007474")},   {35, QColor("#004074")},
    {36, QColor("#000074")},   {37, QColor("#4b0074")},
    {38, QColor("#740074")},   {39, QColor("#740045")},
    {40, QColor("#b50000")},   {41, QColor("#b56300")},
    {42, QColor("#b5b500")},   {43, QColor("#7db500")},
    {44, QColor("#00b500")},   {45, QColor("#00b571")},
    {46, QColor("#00b5b5")},   {47, QColor("#0063b5")},
    {48, QColor("#0000b5")},   {49, QColor("#7500b5")},
    {50, QColor("#b500b5")},   {51, QColor("#b5006b")},
    {52, QColor("#ff0000")},   {53, QColor("#ff8c00")},
    {54, QColor("#ffff00")},   {55, QColor("#b2ff00")},
    {56, QColor("#00ff00")},   {57, QColor("#00ffa0")},
    {58, QColor("#00ffff")},   {59, QColor("#008cff")},
    {60, QColor("#0000ff")},   {61, QColor("#a500ff")},
    {62, QColor("#ff00ff")},   {63, QColor("#ff0098")},
    {64, QColor("#ff5959")},   {65, QColor("#ffb459")},
    {66, QColor("#ffff71")},   {67, QColor("#cfff60")},
    {68, QColor("#6fff6f")},   {69, QColor("#65ffc9")},
    {70, QColor("#6dffff")},   {71, QColor("#59b4ff")},
    {72, QColor("#5959ff")},   {73, QColor("#c459ff")},
    {74, QColor("#ff66ff")},   {75, QColor("#ff59bc")},
    {76, QColor("#ff9c9c")},   {77, QColor("#ffd39c")},
    {78, QColor("#ffff9c")},   {79, QColor("#e2ff9c")},
    {80, QColor("#9cff9c")},   {81, QColor("#9cffdb")},
    {82, QColor("#9cffff")},   {83, QColor("#9cd3ff")},
    {84, QColor("#9c9cff")},   {85, QColor("#dc9cff")},
    {86, QColor("#ff9cff")},   {87, QColor("#ff94d3")},
    {88, QColor("#000000")},   {89, QColor("#131313")},
    {90, QColor("#282828")},   {91, QColor("#363636")},
    {92, QColor("#4d4d4d")},   {93, QColor("#656565")},
    {94, QColor("#818181")},   {95, QColor("#9f9f9f")},
    {96, QColor("#bcbcbc")},   {97, QColor("#e2e2e2")},
    {98, QColor("#ffffff")},
};

}  // namespace chatterino
