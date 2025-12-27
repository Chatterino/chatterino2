# SPDX-FileCopyrightText: 2018 fourtf <tf.four@gmail.com>
#
# SPDX-License-Identifier: CC0-1.0

find . -not -path "*.git*" -exec fsutil.exe file setCaseSensitiveInfo {} disable \;
