#!/usr/bin/env python3

import argparse
import difflib
import pathlib
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from enum import Enum
from typing import Iterator, Protocol

QUOTED_OR_WHITESPACE = re.compile(r"(?:\".*?\"|\S)+")


SOURCES_COMMENT = re.compile(
    r"^(?P<indent>[ \t]*)#![ \t]*SOURCES([ \t](?P<recurse>RECURSE))?([ \t](?P<sort>SORT(ED)?))?[ \t]*(?P<globs>.*)$"
)
SOURCE_ADD_COMMENT = re.compile(r"(?P<indent>[ \t]*)#![ \t]*(?P<globs>.*)$")
SET_START = re.compile(
    r"^(?P<indent>[ \t]*)[Ss][Ee][Tt]\((?P<var_name>[A-Za-z0-9_-]+)[ \t]*(?P<sources>.*)$"
)
SET_CMD = re.compile(
    r"^(?P<indent>[ \t]*)[Ss][Ee][Tt]\((?P<var_name>[A-Za-z0-9_-]+)[ \t]*(?P<sources>.*)\)$"
)
GROUP_COMMENT = re.compile(
    r"^(?P<indent>[ \t]*)#![ \t]*[Gg][Rr][Oo][Uu][Pp]([ \t](?P<recurse>RECURSE))?([ \t](?P<sort>SORT(ED)?))?[ \t]*(?P<globs>.*)$"
)
CMD_END_LINE = re.compile(r"^(?P<indent>[ \t]*)(?P<sources>.*)\)[ \t]*$")
COMMENT_LINE = re.compile(r"^(?P<indent>[ \t]*)#(?P<comment>.*)$")
ADD_SUBDIRECTORY_CMD = re.compile(
    r"^[Aa][Dd][Dd]_[Ss][Uu][Bb][Dd][Ii][Rr][Ee][Cc][Tt][Oo][Rr][Yy]\((?P<path>.*)( EXCLUDE_FROM_ALL)?\)$"
)


def split_quoted(s: str):
    return QUOTED_OR_WHITESPACE.findall(s)


def debug(msg: str, ctx: "Context"):
    if ctx.options.verbose > 0:
        pre = f"DEBUG ({ctx.result.cmake_file}):"
        if sys.stdout.isatty:
            pre = f"{AsciiColorFg.Magenta}{Ascii.Bold}DEBUG{Ascii.Reset}({AsciiColorFg.Yellow}{ctx.result.cmake_file}{Ascii.Reset}):"
        print(f"{pre} {msg}")


class Stringable(Protocol):
    def __str__(self) -> str: ...


class TermCode:
    code: str = ""
    _ESC = "\x1b"

    def __init__(self, code: Stringable):
        self.code = f"{TermCode._ESC}[{code}m"

    def __str__(self) -> str:
        return self.code


class StrDirectEnum(Enum):

    def __str__(self) -> str:
        return str(self.value)


class AsciiColorFg(StrDirectEnum):
    Black = TermCode(30)
    Red = TermCode(31)
    Green = TermCode(32)
    Yellow = TermCode(33)
    Blue = TermCode(34)
    Magenta = TermCode(35)
    Cyan = TermCode(36)
    White = TermCode(37)


class AsciiColorBg(StrDirectEnum):
    Black = TermCode(40)
    Red = TermCode(41)
    Green = TermCode(42)
    Yellow = TermCode(43)
    Blue = TermCode(44)
    Magenta = TermCode(45)
    Cyan = TermCode(46)
    White = TermCode(47)


class Ascii(StrDirectEnum):
    Reset = TermCode(0)
    Bold = TermCode(1)
    Faint = TermCode(2)
    Italic = TermCode(3)
    Underline = TermCode(4)
    NormalColor = TermCode(22)
    NormalFont = TermCode(23)


@dataclass
class FileGroup:
    name: str = ""
    recurse: bool = False
    sort: bool = False
    globs: list[str] = field(default_factory=list)
    sources: set[str] = field(default_factory=set)
    found_sources: set[str] = field(default_factory=set)
    comment_lines: list[str] = field(default_factory=list)
    lines: list[str] = field(default_factory=list)

    def append_line(self, line: str):
        self.lines.append(line)

    def append_comment(self, line: str):
        self.comment_lines.append(line)

    def __iter__(self):
        for line in self.comment_lines:
            yield line
        for line in (sorted(self.lines) if self.sort else self.lines):
            yield line

    def finalize(self, indent: str):
        missing: set[str] = set()
        for source in self.sources:
            if source not in self.found_sources:
                missing.add(source)
        for source in sorted(missing):
            self.append_line(f"{indent}    {source}")


@dataclass
class SourceSet:
    var: str
    recurse: bool = False
    sort: bool = False
    globs: set[str] = field(default_factory=set)
    sources: set[str] = field(default_factory=set)
    found_sources: set[str] = field(default_factory=set)
    present: set[str] = field(default_factory=set)
    groups: list[FileGroup] = field(default_factory=list)


@dataclass
class PathProcessingResult:
    cmake_file: pathlib.Path
    diff: Iterator[str] | None = None
    new_paths: list[pathlib.Path] = field(default_factory=list)
    included: dict[pathlib.Path, str] = field(default_factory=dict)
    missing: dict[pathlib.Path, str] = field(default_factory=dict)
    not_included: set[pathlib.Path] = field(default_factory=set)


@dataclass
class PathProcessingOptions:
    path: pathlib.Path
    root: pathlib.Path
    report_not_included: bool
    report_missing: bool
    remove_missing: bool
    ignore_missing: list[str]
    source_globs: list[str] = field(default_factory=list)
    verbose: int = 0
    color: bool = False


class Section:
    def append(
        self,
        line: str,
        line_no: int,
        ctx: "Context",
    ) -> "None | ProcessState": ...

    def append_header(self, line: str): ...

    def finalize(self, ctx: "Context"): ...

    def __str__(self) -> str: ...


@dataclass
class VerbatumSection(Section):
    header: list[str] = field(default_factory=list)
    footer: list[str] = field(default_factory=list)
    lines: list[str] = field(default_factory=list)

    def append(self, line: str, line_no: int, ctx: "Context"):
        self.lines.append(line)

    def append_header(self, line: str):
        self.header.append(line)

    def append_footer(self, line: str):
        self.header.append(line)

    def finalize(self, ctx: "Context"):
        pass

    def __str__(self) -> str:
        return "".join([*self.header, *self.lines])


@dataclass
class SourceSetSection(Section):
    path: pathlib.Path
    remove_missing: bool
    found_sources: set[str] = field(default_factory=set)
    updating_var: str = ""
    sources: set[str] = field(default_factory=set)
    recurse: bool = False
    sort_output: bool = False
    globs: list[str] = field(default_factory=list)
    file_group: FileGroup = field(init=False)
    groups: list[FileGroup] = field(init=False)
    indent: str = ""
    header: list[str] = field(default_factory=list)
    footer: list[str] = field(default_factory=list)

    def __post_init__(self):
        self.file_group = FileGroup(found_sources=self.found_sources)
        self.groups = [self.file_group]

    def finalize(self, ctx: "Context"):
        for group in self.groups:
            group.finalize(self.indent)
        ctx.found_sources.update(
            ((source, self.updating_var) for source in self.found_sources)
        )

    def __str__(self) -> str:
        return "".join(
            [
                *self.header,
                f"{self.indent}set({self.updating_var}\n",
                *(line for group in self.groups for line in group),
                f"{self.indent})\n",
                *self.footer,
            ]
        )

    def append_header(self, line: str):
        self.header.append(line)

    def append_footer(self, line: str):
        self.header.append(line)

    def append(self, line: str, line_no: int, ctx: "Context"):
        line_sources: list[str] = []
        next_state: None | ProcessState = None
        if match := COMMENT_LINE.match(line):
            file_group = FileGroup(
                name=match.group("comment"), recurse=self.recurse, sort=self.sort_output
            )
            file_group.append_comment(line)
            self.file_group = file_group
            self.groups.append(file_group)
        elif match := CMD_END_LINE.match(line):
            debug(
                f"line {line_no}: found Set ending paren for section `{self.updating_var}`",
                ctx,
            )
            line_sources = split_quoted(match.group("sources"))
            next_state = DefautState()
            ctx.append_section(VerbatumSection())
        elif not self.file_group.lines and (match := GROUP_COMMENT.match(line)):
            debug(f"line {line_no}: found GROUP command for file group `{line}`", ctx)
            if match.group("recurse") is not None and len(match.group("recurse")) > 0:
                self.file_group.recurse = True
            if match.group("sort") is not None and len(match.group("sort")) > 0:
                self.file_group.sort = True
            self.file_group.globs = split_quoted(match.group("globs"))
            self.file_group.sources = find_globs(
                self.file_group.globs, self.file_group.recurse, self.path
            )
            self.file_group.append_comment(line)
        elif not self.file_group.lines and (match := SOURCE_ADD_COMMENT.match(line)):
            debug(
                f"line {line_no}: found globs for file group `{match.group("globs")}`",
                ctx,
            )
            self.file_group.sources.update(
                find_globs(
                    split_quoted(match.group("globs")),
                    self.file_group.recurse,
                    self.path,
                )
            )
            self.file_group.append_comment(line)
        elif not line.strip():  # blank line -> make a unnamed group
            file_group = FileGroup(name="", recurse=self.recurse, sort=self.sort_output)
            file_group.append_comment(line)
            self.file_group = file_group
            self.groups.append(file_group)
        else:
            line_sources = split_quoted(line)

        for l in line_sources:
            if not l.strip():
                continue
            source = line.strip()

            if "$" in l:
                self.file_group.append_line(f"{self.indent}    {l}\n")
                continue
            if self.remove_missing:
                if not self.path.joinpath(source).exists():
                    continue
            self.file_group.append_line(f"{self.indent}    {l}\n")
            self.file_group.found_sources.add(source)
            self.found_sources.add(source)

        return next_state


class ProcessState:
    def process(
        self, ctx: "Context", line: str, lin_no: int
    ) -> "ProcessState": ...  # pyright: ignore[reportUnusedParameter]


def find_globs(globs: list[str], recurse: bool, path: pathlib.Path):
    glob_sources: list[pathlib.Path] = []
    for glob in globs:
        if not recurse:
            glob_sources.extend(path.glob(glob))
        else:
            glob_sources.extend(path.rglob(glob))
    return {
        str(pathlib.PurePosixPath(source.relative_to(path))) for source in glob_sources
    }


@dataclass
class Context:
    options: PathProcessingOptions
    result: PathProcessingResult = field(init=False)
    content: list[str] = field(default_factory=list)

    section: Section = field(init=False)
    sections: list[Section] = field(default_factory=list)

    found_sources: dict[str, str] = field(default_factory=dict)

    state: ProcessState = field(init=False)

    def __post_init__(self):
        cmake_file = self.options.path.joinpath("CMakeLists.txt")
        self.result = PathProcessingResult(cmake_file)
        self.state = DefautState()

    def append_section(self, section: Section):
        self.sections.append(section)
        self.section = section

    def process(self):
        if not self.result.cmake_file.exists():
            return self.result

        self.section = VerbatumSection()
        self.sections = [self.section]

        with self.result.cmake_file.open() as f:
            for lin_no, line in enumerate(f):
                self.content.append(line)
                next = self.state.process(self, line, lin_no + 1)
                if id(next) != id(self.state):
                    debug(f"state change: {next!r}", self)
                self.state = next

        for section in self.sections:
            section.finalize(self)

        content = "".join(self.content)
        output = "".join((str(section) for section in self.sections))

        if output != content:
            rel_path = self.result.cmake_file.relative_to(self.options.root)
            self.result.diff = difflib.unified_diff(
                a=content.splitlines(keepends=True),
                b=output.splitlines(keepends=True),
                fromfile=f"a/{rel_path}",
                tofile=f"b/{rel_path}",
            )

        for source, var in self.found_sources.items():
            path = self.options.path.joinpath(source)
            self.result.included[path] = var
            if not path.exists():
                self.result.missing[path] = var

        source_files = set(
            (
                path
                for glob in self.options.source_globs
                for path in self.options.path.rglob(glob)
                if all(
                    (part not in ["target", "debug", "build"] for part in path.parts)
                )
            )
        )

        for file in source_files:
            if file not in self.result.included:
                self.result.not_included.add(file)

        return self.result


DIFF_HDR_PAT = re.compile(r"^@@ -(\d+),?(\d+)? \+(\d+),?(\d+)? @@$")


def apply_diff_patch(source: str, diff: list[str], reverse=False) -> str:
    text = ""
    i = start = 0
    (pat_group, sign) = (1, "+") if not reverse else (3, "-")
    while i < len(diff) and diff[i].startswith(("---", "+++")):
        i += 1  # skip header lines
    while i < len(diff):
        match = DIFF_HDR_PAT.match(diff[i])
        if not match:
            raise Exception(f"Bad Diff: header missing or malformed (line: {i})")
        end = int(match.group(pat_group)) - 1 + (match.group(pat_group + 1) == "0")
        if start > end or end > len(source):
            raise Exception(f"Bad Diff: bad line number (line: {i})")
        text += "".join(source[start:end])
        start = end
        i += 1
        while i < len(diff) and diff[i][0] != "@":
            line = diff[i]
            i += 1
            if len(line) > 0:
                if line[0] == sign or line[0] == " ":
                    text += line[1:]
                start += line[0] != sign
    text += "".join(source[start:])
    return text


@dataclass
class DefautState(ProcessState):

    def process(self, ctx: "Context", line: str, line_no: int) -> ProcessState:
        next_state = self
        if match := SOURCES_COMMENT.match(line):
            debug(f"line {line_no}: found SOURCES section START comment", ctx)
            recurse = False
            sort = False
            if match.group("recurse") is not None and len(match.group("recurse")) > 0:
                recurse = True
            if match.group("sort") is not None and len(match.group("sort")) > 0:
                sort = True
            globs = split_quoted(match.group("globs"))
            next_state = ParseSorceSectionHeader(
                sort=sort,
                recurse=recurse,
                globs=globs,
                sources=find_globs(globs, recurse=recurse, path=ctx.options.path),
            )
        elif match := ADD_SUBDIRECTORY_CMD.match(line):
            debug(f"line {line_no}: found add_subdirectory command", ctx)
            ctx.result.new_paths.append(ctx.options.path.joinpath(match.group("path")))
        ctx.section.append(line, line_no, ctx)
        return next_state


@dataclass
class ParseSorceSectionHeader(ProcessState):
    sort: bool = False
    recurse: bool = False
    globs: list[str] = field(default_factory=list)
    sources: set[str] = field(default_factory=set)

    def add_section(self, ctx: Context, match: re.Match[str]):
        updating_var = match.group("var_name")
        indent = match.group("indent")
        output_sources: list[str] = []
        found_sources: set[str] = set()
        line_sources = split_quoted(match.group("sources"))
        for line_source in line_sources:
            if line_source in self.sources and line_source not in found_sources:
                found_sources.add(line_source)
                output_sources.append(line_source)
        for source in self.sources:
            if source not in found_sources:
                output_sources.append(source)
        ctx.append_section(
            SourceSetSection(
                path=ctx.options.path,
                remove_missing=ctx.options.remove_missing,
                updating_var=updating_var,
                found_sources=found_sources,
                sort_output=self.sort,
                sources=self.sources,
                recurse=self.recurse,
                indent=indent,
            )
        )

    def process(self, ctx: "Context", line: str, line_no: int) -> ProcessState:
        next_state = self
        if match := SOURCE_ADD_COMMENT.match(line):
            debug(f"line {line_no}: found source globs {match.group("globs")}", ctx)
            self.sources.update(
                find_globs(
                    split_quoted(match.group("globs")), self.recurse, ctx.options.path
                )
            )
            ctx.section.append_header(line)
        elif match := SET_CMD.match(line):
            debug(
                f"line {line_no}: found set cmd for SOURCES section `{match.group("var_name")}`",
                ctx,
            )
            self.add_section(ctx, match)
            next_state = DefautState()
            ctx.append_section(VerbatumSection())
        elif match := SET_START.match(line):
            debug(
                f"line {line_no}:  found set cmd START for SOURCES section `{match.group("var_name")}`",
                ctx,
            )
            self.add_section(ctx, match)
            next_state = ParseSourceSection()
        elif match := COMMENT_LINE.match(line):
            ctx.section.append_header(line)
        else:
            debug(f"line {line_no}: Broken SROUCES section", ctx)
            next_state = DefautState()
            ctx.append_section(VerbatumSection())

        return next_state


@dataclass
class ParseSourceSection(ProcessState):
    def process(self, ctx: "Context", line: str, line_no: int) -> ProcessState:
        next_state = self
        next = ctx.section.append(line, line_no, ctx)
        if next is not None:
            debug(f"line {line_no}: switching states to {next!r}", ctx)
            next_state = next
        return next_state


ASCIIESC = re.compile("\x1b\\[[0-9;]*m")


def eprint(*args, **kwargs):
    if not sys.stderr.isatty:
        print(*(ASCIIESC.sub("", str(arg)) for arg in args), file=sys.stderr, **kwargs)
    else:
        print(*args, file=sys.stderr, **kwargs)


def main():

    default_cmake_root = pathlib.Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(
        prog="UpdateCMakeSources",
        description="Parse comments above CMake source lists for globs to update the lists with files found."
        + "\tPrints unified diffs to stdout if a change should be made.",
    )

    parser.add_argument("--root", type=pathlib.Path, default=default_cmake_root)
    parser.add_argument(
        "--fix", "-f", action="store_true", help="modify the files in place"
    )
    parser.add_argument(
        "--ignore",
        "-i",
        action="append",
        nargs="*",
        type=pathlib.Path,
        default=[
            pathlib.Path("libraries"),
            pathlib.Path("program_info"),
            pathlib.Path("tests"),
        ],
        help="directory names not to traverse when finding/importing CMakeLists.txt",
    )
    parser.add_argument(
        "--report-missing",
        "-m",
        action="store_true",
        help="Report listed files not found by the update globs",
    )
    parser.add_argument(
        "--remove-missing",
        "-R",
        action="store_true",
        help="Remove listed files not found by the update globs",
    )
    parser.add_argument(
        "--ignore-not-included",
        "-I",
        action="append",
        nargs="*",
        type=str,
        default=["*main.cpp"],
        help="paths or globs to ignore if detected as not included",
    )
    parser.add_argument(
        "--report-not-included",
        "-N",
        action="store_true",
        help="Report source files in the processed paths nnt included in a source listing",
    )
    parser.add_argument(
        "--source-globs",
        help="glops used for finding paths not included in a source listing",
        action="append",
        nargs="*",
        type=str,
        default=["*.c", "*.cpp", "*.cc", "*.h", "*.hpp"],
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Print out put in ASCII color on the console",
    )
    parser.add_argument(
        "--check",
        "-c",
        action="store_true",
        help="Only perform checks, don't print diff, exits non-zero if any check fail",
    )
    parser.add_argument("--verbose", "-v", action="count", default=0)

    exit_code = 0

    args = parser.parse_args()
    root: pathlib.Path = args.root.resolve()
    ignore_paths: list[pathlib.Path] = [root.joinpath(p) for p in args.ignore]
    fix: bool = args.fix
    report_missing: bool = args.report_missing
    remove_missing: bool = args.report_missing
    ignore_not_included: list[str] = args.ignore_not_included
    report_not_included: bool = args.report_not_included
    source_globs = args.source_globs
    color: bool = not args.no_color
    check = args.check
    verbose: int = args.verbose

    def parents_ignored(path: pathlib.Path) -> bool:
        return any((parent in ignore_paths for parent in path.parents))

    paths_to_process = [root]

    results: list[PathProcessingResult] = []
    included: dict[pathlib.Path, list[tuple[pathlib.Path, str]]] = defaultdict(list)
    missing: dict[pathlib.Path, list[tuple[pathlib.Path, str]]] = defaultdict(list)
    not_included: set[pathlib.Path] = set()
    while paths_to_process:
        cur_path = paths_to_process.pop()
        if (
            cur_path not in ignore_paths
            and cur_path.parent not in ignore_paths
            and cur_path.parent.parent not in ignore_paths
        ):
            result = Context(
                PathProcessingOptions(
                    path=cur_path,
                    root=root,
                    report_missing=report_missing,
                    remove_missing=remove_missing,
                    ignore_missing=ignore_not_included,
                    report_not_included=report_not_included,
                    source_globs=source_globs,
                    verbose=verbose,
                    color=color,
                )
            ).process()

            for path, var in result.included.items():
                included[path].append((result.cmake_file, var))
            for path, var in result.missing.items():
                missing[path].append((result.cmake_file, var))
            for path in result.not_included:
                if not parents_ignored(path):
                    not_included.add(path)

            results.append(result)
            if len(result.new_paths) > 0:
                paths_to_process.extend(result.new_paths)

    not_included.difference_update(included.keys())

    if report_missing:
        if len(missing) > 0:
            exit_code = 1
        for path, places in sorted(missing.items()):
            eprint(
                f"{AsciiColorFg.Red}MISSING: {AsciiColorFg.Yellow}{path}{Ascii.Reset}"
            )
            for file, var in places:
                eprint(
                    f"    Sourced in: {AsciiColorFg.Cyan}{var}{Ascii.Reset} from {AsciiColorFg.Yellow}{file}{Ascii.Reset}"
                )

    if report_not_included:
        if len(not_included) > 0:
            exit_code = 1
        for path in sorted(not_included):
            path = path.relative_to(root)
            if any((path.match(pat) for pat in ignore_not_included)):
                continue
            eprint(
                f"{AsciiColorFg.Magenta}NOT INCLUDED:{Ascii.Reset} {AsciiColorFg.Yellow}{path}{Ascii.Reset} was not included in any marked source sections"
            )

    if not check:
        for result in results:
            if result.diff is not None:
                if fix:
                    new_content = apply_diff_patch(
                        result.cmake_file.read_text(), list(result.diff)
                    )
                    result.cmake_file.write_text(new_content)
                else:
                    print_diff(result.diff, color)
    else:
        if any((result.diff is not None for result in results)):
            exit_code = 1

    if not fix and not check:
        exit_code = 0
    sys.exit(exit_code)


def print_diff(diff: Iterator[str], color: bool):
    is_ascii = sys.stdout.isatty()
    for line in diff:
        if color and is_ascii:
            if line.startswith("+") and not line.startswith("+++"):
                sys.stdout.write(f"{AsciiColorFg.Green}{line}{Ascii.Reset}")
            elif line.startswith("-") and not line.startswith("---"):
                sys.stdout.write(f"{AsciiColorFg.Red}{line}{Ascii.Reset}")
            elif match := re.match(r"@@[0-9-+, ]*@@", line):
                start, end = line[0 : match.endpos], line[match.endpos :]
                sys.stdout.write(f"{AsciiColorFg.Blue}{start}{Ascii.Reset}{end}")
            else:
                sys.stdout.write(line)
        else:
            sys.stdout.write(line)


if __name__ == "__main__":
    main()
