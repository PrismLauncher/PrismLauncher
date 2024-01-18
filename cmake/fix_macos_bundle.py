# -*- coding: utf-8 -*-
#
# Taken from https://github.com/pyinstaller/pyinstaller/wiki/Recipe-OSX-Code-Signing-Qt
#
# Given that the original file doesn't have a copyright header, here's the one from
# their COPYING file at https://github.com/pyinstaller/pyinstaller/blob/develop/COPYING.txt
#
#-----------------------------------------------------------------------------
# Copyright (c) 2005-2023, PyInstaller Development Team.
#
# Distributed under the terms of the GNU General Public License (version 2
# or later) with exception for distributing the bootloader.
#
# The full license is in the file COPYING.txt, distributed with this software.
#
# SPDX-License-Identifier: (GPL-2.0-or-later WITH Bootloader-exception)
#-----------------------------------------------------------------------------

import os
import shutil
import sys
from pathlib import Path
from typing import Generator, List, Optional

from macholib.MachO import MachO


def create_symlink(folder: Path) -> None:
    """Create the appropriate symlink in the MacOS folder
    pointing to the Resources folder.
    """
    sibbling = Path(str(folder).replace("MacOS", ""))

    # PyQt5/Qt/qml/QtQml/Models.2
    root = str(sibbling).partition("Contents")[2].lstrip("/")
    # ../../../../
    backward = "../" * (root.count("/") + 1)
    # ../../../../Resources/PyQt5/Qt/qml/QtQml/Models.2
    good_path = f"{backward}Resources/{root}"

    folder.symlink_to(good_path)


def fix_dll(dll: Path) -> None:
    """Fix the DLL lookup paths to use relative ones for Qt dependencies.
    Inspiration: PyInstaller/depend/dylib.py:mac_set_relative_dylib_deps()
    Currently one header is pointing to (we are in the Resources folder):
        @loader_path/../../../../QtCore (it is referencing to the old MacOS folder)
    It will be converted to:
        @loader_path/../../../../../../MacOS/QtCore
    """

    def match_func(pth: str) -> Optional[str]:
        """Callback function for MachO.rewriteLoadCommands() that is
        called on every lookup path setted in the DLL headers.
        By returning None for system libraries, it changes nothing.
        Else we return a relative path pointing to the good file
        in the MacOS folder.
        """
        basename = os.path.basename(pth)
        if not basename.startswith("Qt"):
            return None
        return f"@loader_path{good_path}/{basename}"

    # Resources/PyQt5/Qt/qml/QtQuick/Controls.2/Fusion
    root = str(dll.parent).partition("Contents")[2][1:]
    # /../../../../../../..
    backward = "/.." * (root.count("/") + 1)
    # /../../../../../../../MacOS
    good_path = f"{backward}/MacOS"

    # Rewrite Mach headers with corrected @loader_path
    dll = MachO(dll)
    dll.rewriteLoadCommands(match_func)
    with open(dll.filename, "rb+") as f:
        for header in dll.headers:
            f.seek(0)
            dll.write(f)
        f.seek(0, 2)
        f.flush()


def find_problematic_folders(folder: Path) -> Generator[Path, None, None]:
    """Recursively yields problematic folders (containing a dot in their name)."""
    for path in folder.iterdir():
        if not path.is_dir() or path.is_symlink():
            # Skip simlinks as they are allowed (even with a dot)
            continue
        if "." in path.name:
            yield path
        else:
            yield from find_problematic_folders(path)


def move_contents_to_resources(folder: Path) -> Generator[Path, None, None]:
    """Recursively move any non symlink file from a problematic folder
    to the sibbling one in Resources.
    """
    for path in folder.iterdir():
        if path.is_symlink():
            continue
        if path.name == "qml":
            yield from move_contents_to_resources(path)
        else:
            sibbling = Path(str(path).replace("MacOS", "Resources"))
            sibbling.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(path, sibbling)
            yield sibbling


def main(args: List[str]) -> int:
    """
    Fix the application to allow codesign (NXDRIVE-1301).
    Take one or more .app as arguments: "Nuxeo Drive.app".
    To overall process will:
        - move problematic folders from MacOS to Resources
        - fix the DLLs lookup paths
        - create the appropriate symbolic link
    """
    for app in args:
        name = os.path.basename(app)
        print(f">>> [{name}] Fixing Qt folder names")
        path = Path(app) / "Contents" / "MacOS"
        for folder in find_problematic_folders(path):
            for file in move_contents_to_resources(folder):
                try:
                    fix_dll(file)
                except (ValueError, IsADirectoryError):
                    continue
            shutil.rmtree(folder)
            create_symlink(folder)
            print(f" !! Fixed {folder}")
        print(f">>> [{name}] Application fixed.")


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
