import argparse
import json
import os
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser(description="Ninja Bazel builder.")

parser.add_argument("--ninja-file", type=str, help="The ninja file in use", default="build.ninja")
parser.add_argument("--verbose", action="store_true", help="Turn on verbose mode")
parser.add_argument(
    "--integration-debug",
    action="store_true",
    help="Turn on extra debug output about the ninja-bazel integration",
)

args = parser.parse_args()

# This corresponds to BAZEL_INTEGRATION_DEBUG=1 from SCons command line
if args.integration_debug:

    def print_debug(msg):
        print("[BAZEL_INTEGRATION_DEBUG] " + msg)
else:

    def print_debug(msg):
        pass


# our ninja python module intercepts the command lines and
# prints out the targets everytime ninja is executed
ninja_command_line_targets = []
try:
    ninja_last_cmd_file = ".ninja_last_command_line_targets.txt"
    with open(ninja_last_cmd_file) as f:
        ninja_command_line_targets = [target.strip() for target in f.readlines() if target.strip()]
except OSError as exc:
    print(
        f"Failed to open {ninja_last_cmd_file}, this is expected to be generated on ninja execution by the mongo-ninja-python module."
    )
    raise exc


# Our ninja generation process generates all the build info related to
# the specific ninja file
ninja_build_info = dict()
try:
    bazel_info_file = ".bazel_info_for_ninja.txt"
    with open(bazel_info_file) as f:
        ninja_build_info = json.load(f)
except OSError as exc:
    print(
        f"Failed to open {bazel_info_file}, this is expected to be generated by scons during ninja generation."
    )
    raise exc

# ninja will automatically create directories for any outputs, but in this case
# bazel will be creating a symlink for the bazel-out dir to its cache. We don't want
# ninja to interfere so delete the dir if it was not a link (made by bazel)
if sys.platform == "win32":
    if os.path.exists("bazel-out"):
        try:
            os.readlink("bazel-out")
        except OSError:
            shutil.rmtree("bazel-out")

else:
    if not os.path.islink("bazel-out"):
        shutil.rmtree("bazel-out")

# now we are ready to build all bazel buildable files
targets_to_build = ["//src/..."]
if args.verbose:
    extra_args = []
else:
    extra_args = ["--output_filter=DONT_MATCH_ANYTHING"]

bazel_env = os.environ.copy()
if ninja_build_info.get("USE_NATIVE_TOOLCHAIN"):
    bazel_env["CC"] = ninja_build_info.get("CC")
    bazel_env["CXX"] = ninja_build_info.get("CXX")
    bazel_env["USE_NATIVE_TOOLCHAIN"] = "1"
sys.stderr.write(
    f"Running bazel command:\n{' '.join(ninja_build_info['bazel_cmd'] + extra_args + targets_to_build)}\n"
)
bazel_proc = subprocess.run(
    ninja_build_info["bazel_cmd"] + extra_args + targets_to_build, env=bazel_env
)
if bazel_proc.returncode != 0:
    print("Command that failed:")
    print(" ".join(ninja_build_info["bazel_cmd"] + extra_args + targets_to_build))
    sys.exit(1)
if (
    "compiledb" in ninja_command_line_targets
    or "compile_commands.json" in ninja_command_line_targets
):
    bazel_proc = subprocess.run(ninja_build_info["compiledb_cmd"], env=bazel_env)
    if bazel_proc.returncode != 0:
        print("Command that failed:")
        print(" ".join(ninja_build_info["compiledb_cmd"]))
        sys.exit(1)
