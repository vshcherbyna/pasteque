#
#  pastèque - a UCI chess playing engine derived from GreKo 2018.01
#
#  Copyright (C) 2018 Volodymyr Shcherbyna <volodymyr@shcherbyna.com>
#
#  pastèque is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  pastèque is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with pastèque.  If not, see <http://www.gnu.org/licenses/>.
#

#!/usr/bin/python3

import os
import platform

if platform.system() == 'Windows':
    if os.system('..\\unit.exe') != 0:
        raise Exception("test failed:", "unit")
else:
    if os.system('../unit') != 0:
        raise Exception("test failed:", "unit")
