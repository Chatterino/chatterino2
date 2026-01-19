// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

class QApplication;

namespace chatterino {

class Args;
class Paths;
class Settings;
class Updates;

void runGui(QApplication &a, const Paths &paths, Settings &settings,
            const Args &args, Updates &updates);

}  // namespace chatterino
