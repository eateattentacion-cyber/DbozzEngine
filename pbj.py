#!/usr/bin/env python3
#single file for build system(scons inspired https://scons.org/)
#############################################################################
# pbj.py                                                                    #
#############################################################################
#                         This file is part of:                             #
#                           DABOZZ ENGINE                                   #
#############################################################################
# Copyright (c) 2026-present DabozzEngine contributors.                     #
#                                                                           #
# Permission is hereby granted, free of charge, to any person obtaining     #
# a copy of this software and associated documentation files (the           #
# "Software"), to deal in the Software without restriction, including       #
# without limitation the rights to use, copy, modify, merge, publish,       #
# distribute, sublicense, and/or sell copies of the Software, and to        #
# permit persons to whom the Software is furnished to do so, subject to     #
# the following conditions:                                                 #
#                                                                           #
# The above copyright notice and this permission notice shall be included   #
# in all copies or substantial portions of the Software.                    #
#                                                                           #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   #
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                #
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.    #
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY      #
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,      #
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE         #
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                    #
# dabozzengine is made on our child company Junkygames                      #
#  https://www.youtube.com/@JunkyGames-g8t                                  #
#############################################################################

"""
PB&J Build System - A  build system for DabozzEngine.

Usage:
    python pbj.py build [--target debug|release] [-j N] [--verbose]
    python pbj.py clean
    python pbj.py rebuild [--target debug|release] [-j N] [--verbose]
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
from multiprocessing import Pool, cpu_count
from pathlib import Path


class Environment:
    """
    @brief Build environment configuration.

    Holds all compiler settings, source files, include paths, libraries,
    and flags needed to compile and link the project.
    """

    def __init__(self):
        self.project_name = ""
        self.compiler = "g++"
        self.linker = "g++"
        self.output = "a.exe"
        self.obj_dir = "obj"
        self.bin_dir = "bin"

        self._sources = []
        self._includes = []
        self._cflags = []
        self._defines = []
        self._ldflags = []
        self._lib_dirs = []
        self._libs = []
        self._extensions = [".cpp", ".c"]

        self._debug_cflags = ["-g", "-O0", "-DDEBUG"]
        self._release_cflags = ["-O2", "-DNDEBUG"]

        self.moc_path = ""
        self.rcc_path = ""
        self._qrc_files = []
        self._moc_dirs = []
        self._deploy_files = []
        self._deploy_dirs = []

    def add_sources(self, directory, extensions=None):
        """
        @brief Recursively scan a directory for source files.
        @param directory Source directory path relative to project root.
        @param extensions List of file extensions to include (e.g. [".cpp", ".c"]).
        """
        if extensions:
            self._extensions = extensions
        root = Path(directory)
        for ext in self._extensions:
            for f in root.rglob(f"*{ext}"):
                self._sources.append(str(f))

    def add_source_files(self, files):
        """
        @brief Add explicit source file paths.
        @param files List of source file paths.
        """
        self._sources.extend(files)

    def add_includes(self, paths):
        """
        @brief Add include directories.
        @param paths List of include directory paths.
        """
        self._includes.extend(paths)

    def add_cflags(self, flags):
        """
        @brief Add compiler flags.
        @param flags List of compiler flags.
        """
        self._cflags.extend(flags)

    def add_defines(self, defines):
        """
        @brief Add preprocessor definitions.
        @param defines List of definition names (without -D prefix).
        """
        self._defines.extend(defines)

    def add_ldflags(self, flags):
        """
        @brief Add linker flags.
        @param flags List of linker flags.
        """
        self._ldflags.extend(flags)

    def add_lib_dirs(self, paths):
        """
        @brief Add library search directories.
        @param paths List of library directory paths.
        """
        self._lib_dirs.extend(paths)

    def add_libs(self, libs):
        """
        @brief Add libraries to link against.
        @param libs List of library names (without -l prefix).
        """
        self._libs.extend(libs)

    def set_debug_flags(self, flags):
        """
        @brief Override default debug-mode compiler flags.
        @param flags List of compiler flags for debug builds.
        """
        self._debug_cflags = flags

    def set_release_flags(self, flags):
        """
        @brief Override default release-mode compiler flags.
        @param flags List of compiler flags for release builds.
        """
        self._release_cflags = flags

    def add_qrc(self, qrc_files):
        """
        @brief Add Qt resource files (.qrc) to be compiled with rcc.
        @param qrc_files List of .qrc file paths.
        """
        if isinstance(qrc_files, str):
            qrc_files = [qrc_files]
        self._qrc_files.extend(qrc_files)

    def add_moc_dirs(self, dirs):
        """
        @brief Add directories to scan for Q_OBJECT headers (moc).
        @param dirs List of directories containing project headers to moc.
        """
        if isinstance(dirs, str):
            dirs = [dirs]
        self._moc_dirs.extend(dirs)

    def deploy(self, src, dst=""):
        """
        @brief Register a file to copy into bin_dir after build.
        @param src Source file path.
        @param dst Destination path relative to bin_dir (default: same filename).
        """
        self._deploy_files.append((src, dst))

    def deploy_dir(self, src, dst):
        """
        @brief Register a directory to copy into bin_dir after build.
        @param src Source directory path.
        @param dst Destination path relative to bin_dir.
        """
        self._deploy_dirs.append((src, dst))


class DependencyCache:
    """
    @brief Tracks file hashes and header dependencies for incremental builds.

    Stores MD5 hashes of source and header files in a JSON cache file.
    On subsequent builds, only files whose hash (or whose headers' hashes)
    changed will be recompiled.
    """

    def __init__(self, cache_path):
        self._cache_path = cache_path
        self._data = {"files": {}, "deps": {}}
        self._load()

    def _load(self):
        if os.path.exists(self._cache_path):
            try:
                with open(self._cache_path, "r") as f:
                    self._data = json.load(f)
            except (json.JSONDecodeError, IOError):
                self._data = {"files": {}, "deps": {}}

    def save(self):
        with open(self._cache_path, "w") as f:
            json.dump(self._data, f, indent=2)

    def get_file_hash(self, filepath):
        """@brief Compute MD5 hash of a file's contents."""
        h = hashlib.md5()
        try:
            with open(filepath, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    h.update(chunk)
        except IOError:
            return None
        return h.hexdigest()

    def needs_rebuild(self, source, include_dirs, obj_path=None):
        """
        @brief Check if a source file needs recompilation.
        @param source Path to the source file.
        @param include_dirs List of include directories for header resolution.
        @param obj_path Path to the expected object file.
        @return True if the file or any of its dependencies changed.
        """
        if obj_path and not os.path.exists(obj_path):
            return True

        current_hash = self.get_file_hash(source)
        if current_hash is None:
            return True

        cached_hash = self._data["files"].get(source)
        if cached_hash != current_hash:
            return True

        deps = self._resolve_deps(source, include_dirs)
        cached_deps = self._data["deps"].get(source, {})
        for dep in deps:
            dep_hash = self.get_file_hash(dep)
            if dep_hash is None or cached_deps.get(dep) != dep_hash:
                return True

        return False

    def update(self, source, include_dirs):
        """
        @brief Update the cache entry for a source file after compilation.
        @param source Path to the source file.
        @param include_dirs List of include directories for header resolution.
        """
        self._data["files"][source] = self.get_file_hash(source)
        deps = self._resolve_deps(source, include_dirs)
        dep_hashes = {}
        for dep in deps:
            h = self.get_file_hash(dep)
            if h:
                dep_hashes[dep] = h
        self._data["deps"][source] = dep_hashes

    def _resolve_deps(self, source, include_dirs, visited=None):
        """
        @brief Recursively resolve #include dependencies.
        @param source Path to the source file.
        @param include_dirs List of include directories.
        @param visited Set of already-visited files to avoid cycles.
        @return Set of header file paths that this source depends on.
        """
        if visited is None:
            visited = set()

        if source in visited:
            return set()
        visited.add(source)

        deps = set()
        include_re = re.compile(r'#\s*include\s*"([^"]+)"')

        try:
            with open(source, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    m = include_re.search(line)
                    if m:
                        inc = m.group(1)
                        resolved = self._find_header(inc, source, include_dirs)
                        if resolved and resolved not in visited:
                            deps.add(resolved)
                            deps.update(self._resolve_deps(resolved, include_dirs, visited))
        except IOError:
            pass

        return deps

    def _find_header(self, include_name, source_file, include_dirs):
        """
        @brief Locate a header file on disk.
        @param include_name The include path as written in the #include directive.
        @param source_file The source file containing the #include.
        @param include_dirs List of include search directories.
        @return Absolute path to the header, or None if not found.
        """
        source_dir = os.path.dirname(os.path.abspath(source_file))
        candidate = os.path.join(source_dir, include_name)
        if os.path.isfile(candidate):
            return os.path.normpath(candidate)

        for inc_dir in include_dirs:
            candidate = os.path.join(inc_dir, include_name)
            if os.path.isfile(candidate):
                return os.path.normpath(candidate)

        return None


def _compile_task(args):
    """
    @brief Worker function for parallel compilation.
    @param args Tuple of (source, obj_path, command, verbose).
    @return Tuple of (source, success, output).
    """
    source, obj_path, cmd, verbose = args
    os.makedirs(os.path.dirname(obj_path), exist_ok=True)
    if verbose:
        print(f"  [CC] {source}")
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if result.returncode != 0:
            return (source, False, result.stderr)
        return (source, True, "")
    except subprocess.TimeoutExpired:
        return (source, False, "compilation timed out")
    except Exception as e:
        return (source, False, str(e))


class Builder:
    """
    @brief Orchestrates the build process.

    Reads the Environment configuration, manages the dependency cache,
    dispatches compilation tasks (optionally in parallel), and links
    the final executable.
    """

    def __init__(self, env, target="release", jobs=1, verbose=False):
        self._env = env
        self._target = target
        self._jobs = jobs
        self._verbose = verbose
        self._cache = DependencyCache(".pbj_cache.json")

    def _run_moc(self):
        """
        @brief Scan headers for Q_OBJECT and run Qt moc to generate moc_*.cpp files.
        @return List of generated moc source file paths.
        """
        if not self._env.moc_path:
            return []

        moc_dir = os.path.join(self._env.obj_dir, "moc")
        os.makedirs(moc_dir, exist_ok=True)

        q_object_re = re.compile(r'^\s*Q_OBJECT\s*$', re.MULTILINE)
        moc_sources = []

        moc_dirs = self._env._moc_dirs if self._env._moc_dirs else []
        if not moc_dirs:
            return []

        headers = []
        for inc_dir in moc_dirs:
            if os.path.isdir(inc_dir):
                for root, _, files in os.walk(inc_dir):
                    for f in files:
                        if f.endswith(".h") or f.endswith(".hpp"):
                            headers.append(os.path.join(root, f))

        for header in headers:
            try:
                with open(header, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
            except IOError:
                continue

            if not q_object_re.search(content):
                continue

            base = os.path.splitext(os.path.basename(header))[0]
            moc_out = os.path.join(moc_dir, f"moc_{base}.cpp")

            header_hash = self._cache.get_file_hash(header)
            cache_key = f"moc:{header}"
            cached = self._cache._data["files"].get(cache_key)

            if cached == header_hash and os.path.exists(moc_out):
                moc_sources.append(moc_out)
                continue

            cmd = [self._env.moc_path]
            for inc in self._env._includes:
                cmd.extend(["-I", inc])
            for d in self._env._defines:
                cmd.append(f"-D{d}")
            cmd.extend([header, "-o", moc_out])

            if self._verbose:
                print(f"  [MOC] {header}")

            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"  [MOC FAIL] {header}")
                print(result.stderr)
                continue

            self._cache._data["files"][cache_key] = header_hash
            moc_sources.append(moc_out)

        return moc_sources

    def _run_rcc(self):
        """
        @brief Compile Qt resource files (.qrc) with rcc.
        @return List of generated rcc source file paths.
        """
        if not self._env.rcc_path or not self._env._qrc_files:
            return []

        rcc_dir = os.path.join(self._env.obj_dir, "rcc")
        os.makedirs(rcc_dir, exist_ok=True)

        rcc_sources = []
        for qrc in self._env._qrc_files:
            base = os.path.splitext(os.path.basename(qrc))[0]
            rcc_out = os.path.join(rcc_dir, f"qrc_{base}.cpp")

            qrc_hash = self._cache.get_file_hash(qrc)
            cache_key = f"rcc:{qrc}"
            cached = self._cache._data["files"].get(cache_key)

            if cached == qrc_hash and os.path.exists(rcc_out):
                rcc_sources.append(rcc_out)
                continue

            cmd = [self._env.rcc_path, qrc, "-o", rcc_out]

            if self._verbose:
                print(f"  [RCC] {qrc}")

            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"  [RCC FAIL] {qrc}")
                print(result.stderr)
                continue

            self._cache._data["files"][cache_key] = qrc_hash
            rcc_sources.append(rcc_out)

        return rcc_sources

    def build(self):
        """@brief Run the full build: compile changed sources, then link."""
        start = time.time()
        print(f"=== PB&J Build System ===")
        print(f"Project: {self._env.project_name}")
        print(f"Target:  {self._target}")
        print(f"Jobs:    {self._jobs}")
        print()

        os.makedirs(self._env.obj_dir, exist_ok=True)
        os.makedirs(self._env.bin_dir, exist_ok=True)

        moc_sources = self._run_moc()
        rcc_sources = self._run_rcc()

        all_sources = list(self._env._sources) + moc_sources + rcc_sources

        tasks = []
        up_to_date = 0
        all_objs = []

        for source in all_sources:
            source = os.path.normpath(source)
            obj_path = self._source_to_obj(source)
            all_objs.append(obj_path)

            if self._cache.needs_rebuild(source, self._env._includes, obj_path):
                cmd = self._compile_cmd(source, obj_path)
                tasks.append((source, obj_path, cmd, self._verbose))
            else:
                up_to_date += 1

        if not tasks and up_to_date > 0:
            print(f"All {up_to_date} files up to date.")
            if not self._link_needed(all_objs):
                self._deploy()
                elapsed = time.time() - start
                print(f"\nBuild complete ({elapsed:.2f}s)")
                return True

        if tasks:
            print(f"Compiling {len(tasks)} file(s) ({up_to_date} up to date)...")
            if self._jobs > 1 and len(tasks) > 1:
                with Pool(processes=self._jobs) as pool:
                    results = pool.map(_compile_task, tasks)
            else:
                results = [_compile_task(t) for t in tasks]

            failed = False
            for source, success, output in results:
                if not success:
                    print(f"\n  [FAIL] {source}")
                    print(output)
                    failed = True
                else:
                    self._cache.update(source, self._env._includes)

            self._cache.save()

            if failed:
                print("\nBuild FAILED.")
                return False

        print(f"\nLinking {self._env.output}...")
        if not self._link(all_objs):
            print("Link FAILED.")
            return False

        self._deploy()

        elapsed = time.time() - start
        print(f"\nBuild complete ({elapsed:.2f}s)")
        return True

    def clean(self):
        """@brief Remove all build artifacts."""
        print("Cleaning...")
        for d in [self._env.obj_dir]:
            if os.path.exists(d):
                shutil.rmtree(d)
                print(f"  Removed {d}/")

        output_path = os.path.join(self._env.bin_dir, self._env.output)
        if os.path.exists(output_path):
            os.remove(output_path)
            print(f"  Removed {output_path}")

        if os.path.exists(".pbj_cache.json"):
            os.remove(".pbj_cache.json")
            print("  Removed .pbj_cache.json")

        print("Clean complete.")

    def _source_to_obj(self, source):
        """@brief Map a source file path to its object file path."""
        rel = os.path.normpath(source)
        obj_name = rel.replace(os.sep, "_").replace("/", "_") + ".o"
        return os.path.join(self._env.obj_dir, obj_name)

    def _compile_cmd(self, source, obj_path):
        """@brief Build the compiler command line for a single source file."""
        cmd = [self._env.compiler]

        if self._target == "debug":
            cmd.extend(self._env._debug_cflags)
        else:
            cmd.extend(self._env._release_cflags)

        cmd.extend(self._env._cflags)

        for d in self._env._defines:
            cmd.append(f"-D{d}")

        for inc in self._env._includes:
            cmd.extend(["-I", inc])

        cmd.extend(["-c", source, "-o", obj_path])
        return cmd

    def _link_needed(self, obj_files):
        """@brief Check if the output binary exists and is newer than all objects."""
        output_path = os.path.join(self._env.bin_dir, self._env.output)
        if not os.path.exists(output_path):
            return True
        out_mtime = os.path.getmtime(output_path)
        for obj in obj_files:
            if os.path.exists(obj) and os.path.getmtime(obj) > out_mtime:
                return True
        return False

    def _link(self, obj_files):
        """@brief Link all object files into the final executable."""
        output_path = os.path.join(self._env.bin_dir, self._env.output)
        os.makedirs(self._env.bin_dir, exist_ok=True)

        cmd = [self._env.linker]

        lflags = []
        archives = []
        for f in self._env._ldflags:
            if f.endswith(".a") or f.endswith(".dll"):
                archives.append(f)
            else:
                lflags.append(f)

        cmd.extend(lflags)

        for obj in obj_files:
            cmd.append(obj)

        for lib_dir in self._env._lib_dirs:
            cmd.append(f"-L{lib_dir}")

        for lib in self._env._libs:
            cmd.append(f"-l{lib}")

        cmd.extend(archives)

        cmd.extend(["-o", output_path])

        if self._verbose:
            print(f"  [LD] {' '.join(cmd)}")

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(result.stderr)
            return False
        return True

    def _deploy(self):
        """@brief Copy deploy files and directories into the bin directory."""
        if not self._env._deploy_files and not self._env._deploy_dirs:
            return

        print("Deploying dependencies...")
        bin_dir = self._env.bin_dir

        for src, dst in self._env._deploy_files:
            if not dst:
                dst = os.path.basename(src)
            dest_path = os.path.join(bin_dir, dst)
            os.makedirs(os.path.dirname(dest_path) if os.path.dirname(dst) else bin_dir, exist_ok=True)
            if os.path.exists(dest_path):
                src_mtime = os.path.getmtime(src)
                dst_mtime = os.path.getmtime(dest_path)
                if src_mtime <= dst_mtime:
                    continue
            shutil.copy2(src, dest_path)
            if self._verbose:
                print(f"  [DEPLOY] {src} -> {dest_path}")

        for src, dst in self._env._deploy_dirs:
            dest_path = os.path.join(bin_dir, dst)
            if os.path.exists(dest_path):
                shutil.rmtree(dest_path)
            shutil.copytree(src, dest_path)
            if self._verbose:
                print(f"  [DEPLOY] {src}/ -> {dest_path}/")


def _load_pbjfile():
    """
    @brief Load and execute pbjfile.py to get the build Environment.
    @return The Environment object configured by pbjfile.py.
    """
    pbjfile = os.path.join(os.getcwd(), "pbjfile.py")
    if not os.path.exists(pbjfile):
        print("Error: pbjfile.py not found in current directory.")
        sys.exit(1)

    env = Environment()
    globs = {"__builtins__": __builtins__, "Environment": Environment, "env": env}
    with open(pbjfile, "r") as f:
        exec(f.read(), globs)

    if "env" in globs:
        env = globs["env"]

    return env


def main():
    parser = argparse.ArgumentParser(
        prog="pbj",
        description="PB&J Build System for DabozzEngine"
    )
    parser.add_argument("command", choices=["build", "clean", "rebuild"],
                        help="Build command to execute")
    parser.add_argument("--target", choices=["debug", "release"], default="release",
                        help="Build target (default: release)")
    parser.add_argument("-j", "--jobs", type=int, default=cpu_count(),
                        help=f"Number of parallel jobs (default: {cpu_count()})")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Verbose output")

    args = parser.parse_args()
    env = _load_pbjfile()
    builder = Builder(env, target=args.target, jobs=args.jobs, verbose=args.verbose)

    if args.command == "clean":
        builder.clean()
    elif args.command == "build":
        if not builder.build():
            sys.exit(1)
    elif args.command == "rebuild":
        builder.clean()
        if not builder.build():
            sys.exit(1)


if __name__ == "__main__":
  main()