"""Build the VR bridge from this repository; never reads a game installation."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BUILDS = {
    "current": "src/current",
    "teardown-26320217": "src/compat/26320217",
    "teardown-21": "src/compat/legacy21",
}
DETOURS = ["detours", "modules", "disasm", "image", "creatwth",
           "disolx86", "disolx64", "disolia64", "disolarm", "disolarm64"]
INCLUDES = ["third_party/glm", "third_party/openvr/headers",
            "third_party/detours/src", "third_party/glew/include"]
LIBS = ["third_party/openvr/lib/win64/openvr_api.lib",
        "user32.lib", "gdi32.lib", "opengl32.lib", "advapi32.lib"]
COMMON = ["/nologo", "/std:c++20", "/EHsc", "/MD", "/O2", "/DGLEW_STATIC"]


def run(args, log):
    result = subprocess.run([str(a) for a in args], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, errors="replace")
    log = Path(log)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(result.stdout, encoding="utf-8")
    if result.returncode:
        print(result.stdout, flush=True)
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}")
    return result.stdout


def compile_one(task):
    source, output, flags = task
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    # Relative compiler inputs keep __FILE__ assertions free of personal paths.
    run(["cl.exe", *flags, "/c", source, "/Fo" + output],
        Path(output).with_suffix(".log"))
    print("Compiled " + source, flush=True)


def dependencies(jobs):
    includes = ["/I" + p for p in INCLUDES]
    flags = COMMON + ["/DWIN32_LEAN_AND_MEAN", "/DNOMINMAX", "/D_AMD64_",
                      "/D_CRT_SECURE_NO_WARNINGS"] + includes
    tasks = [(f"third_party/detours/src/{n}.cpp", f"build/obj/detours_{n}.obj", flags)
             for n in DETOURS]
    tasks.append(("third_party/glew/src/glew.c", "build/obj/glew.obj",
                  ["/nologo", "/TC", "/MD", "/O2", "/DGLEW_STATIC",
                   "/Ithird_party/glew/include"]))
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        list(pool.map(compile_one, tasks))
    return [t[1] for t in tasks]


def native(build, objects):
    source = BUILDS[build]
    output = Path("build/runtime") / build
    output.mkdir(parents=True, exist_ok=True)
    flags = COMMON + ["/I" + p for p in INCLUDES]
    compiled = []
    for name in ["renderer_bridge", "attach"]:
        obj = f"build/obj/{build}/{name}.obj"
        compile_one((f"{source}/{name}.cpp", obj, flags))
        compiled.append(obj)
    run(["link.exe", "/NOLOGO", "/DLL", "/MACHINE:X64", "/INCREMENTAL:NO",
         "/OUT:" + str(output / "SledgePort21.dll"), compiled[0], *objects, *LIBS],
        f"build/logs/{build}-dll.log")
    run(["link.exe", "/NOLOGO", "/MACHINE:X64", "/INCREMENTAL:NO",
         "/OUT:" + str(output / "SledgePortAttach.exe"), compiled[1], "user32.lib"],
        f"build/logs/{build}-attach.log")
    shutil.copyfile("third_party/openvr/bin/win64/openvr_api.dll", output / "openvr_api.dll")
    print("Built " + build, flush=True)


def tests(objects, opengl):
    programs = ["stereo_tests", "physics_tests", "pose_cache_tests", "assist_tests"]
    if opengl:
        programs.append("inline_hud_tests")
    includes = ["/Isrc/current"] + ["/I" + p for p in INCLUDES]
    Path("build/tests").mkdir(parents=True, exist_ok=True)
    for name in programs:
        obj = f"build/tests/{name}.obj"
        compile_one((f"tests/{name}.cpp", obj, COMMON + ["/DNOMINMAX"] + includes))
        libraries = [*objects, *LIBS] if name == "inline_hud_tests" else ["user32.lib"]
        run(["link.exe", "/NOLOGO", "/MACHINE:X64", "/INCREMENTAL:NO",
             f"/OUT:build/tests/{name}.exe", obj, *libraries], f"build/tests/{name}-link.log")
        shutil.copyfile("third_party/openvr/bin/win64/openvr_api.dll", "build/tests/openvr_api.dll")
        output = run([f"build/tests/{name}.exe"], f"build/tests/{name}-run.log")
        print(output.strip().splitlines()[-1], flush=True)
    for name in ["companion_tests", "hyve_tests", "tablet_asset_tests"]:
        output = run([sys.executable, f"tests/{name}.py"], f"build/tests/{name}.log")
        print(output.strip().splitlines()[-1], flush=True)
    output = run(["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
                  "-File", "tests/performance_tests.ps1"], "build/tests/performance_tests.log")
    print(output.strip().splitlines()[-1], flush=True)
    print("Offline tests passed. A fresh headset session is still required for runtime validation.")


def package():
    # The temporary directory is created under this repository's generated build folder.
    # Only files explicitly collected below enter the release, never logs or settings.
    Path("dist").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="release-", dir=ROOT / "build") as tmp:
        staging = Path(tmp) / "Teardown VR"
        files = {}
        for path in Path("package").rglob("*"):
            if path.is_file():
                files[path.relative_to("package").as_posix()] = path
        for path in Path("mods").rglob("*"):
            if path.is_file():
                files[path.as_posix()] = path
        for build in BUILDS:
            for name in ["SledgePort21.dll", "SledgePortAttach.exe", "openvr_api.dll"]:
                files[f"runtime/{build}/{name}"] = Path(f"build/runtime/{build}/{name}")
        entries = []
        for relative, path in sorted(files.items()):
            data = path.read_bytes()
            out = staging / relative
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(data)
            entries.append({"path": relative, "bytes": len(data),
                            "sha256": hashlib.sha256(data).hexdigest()})
        manifest = {"name": "Teardown VR", "version": "r7.34", "files": entries}
        (staging / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        archive = Path("dist/Teardown VR.zip")
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for path in sorted(staging.rglob("*")):
                if path.is_file():
                    z.write(path, "Teardown VR/" + path.relative_to(staging).as_posix())
        with zipfile.ZipFile(archive) as z:
            if z.testzip() is not None:
                raise RuntimeError("Release ZIP failed its integrity check")
        print(f"Created {archive}; {len(entries) + 1} files", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--all", action="store_true", help="Build all three supported game executables")
    parser.add_argument("--package", action="store_true", help="Build all runtimes and the portable installer ZIP")
    parser.add_argument("--test", action="store_true", help="Run offline native, Lua and graphics-preset tests")
    parser.add_argument("--test-opengl", action="store_true", help="Also run the live OpenGL HUD test (needs a GPU context)")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1))
    args = parser.parse_args()
    os.chdir(ROOT)
    if args.jobs < 1 or args.jobs > 32:
        parser.error("--jobs must be between 1 and 32")
    if not shutil.which("cl.exe") or not shutil.which("link.exe"):
        parser.error("Use Build.cmd or an x64 Native Tools Command Prompt for Visual Studio 2022.")
    if os.environ.get("VSCMD_ARG_TGT_ARCH", "").lower() not in ("", "x64"):
        parser.error("Use the x64 compiler environment.")
    objects = dependencies(args.jobs)
    for build in BUILDS if args.all or args.package else ["current"]:
        native(build, objects)
    if args.test or args.test_opengl:
        tests(objects, args.test_opengl)
    if args.package:
        package()


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError) as error:
        sys.exit(str(error))
