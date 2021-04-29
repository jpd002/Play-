import sys
import os
import shutil
import glob

def fix_id(lib_name, is_fw):
    if(lib_name.count("HOMEBREW_PREFIX") == 0):
        return ""

    if(is_fw):
        lib_name = "/".join(lib_name.split(" (c")[0].strip().split("/")[4:])
        new_lib_path = f"@rpath/{lib_name}"
        return f"-add_rpath @loader_path/../../../ -id {new_lib_path}"
    else:
        lib_name = lib_name.split(" (c")[0].strip().split("/")[-1]
        return f"-add_rpath @loader_path/../../lib -id {lib_name}"


def fix_framework(data):
    data = [lib_name.strip().split(" (c")[0].strip() for lib_name in data if lib_name.count("HOMEBREW_CELLAR")]
    l = []
    for lib_name in data:
        new_lib_name = "/".join(lib_name.split("/")[4:])
        new_lib_path = f"@rpath/{new_lib_name}"
        l.append(f"-change {lib_name} {new_lib_path}")
    return " ".join(l).strip()

def fix_library(fw_path, is_fw):
    stream = os.popen(f"otool -L {fw_path}")
    output = stream.read()
    data = output.split('\n')[1:]

    change_name = fix_framework(data)
    change_id = fix_id(data[0], is_fw)
    args = f" {change_name} {change_id} {fw_path}"
    if(change_id or change_name):
        os.system(f"install_name_tool {args}")
        print(f"install_name_tool {args}")

def process_framework():
    framework_list = glob.glob(os.path.join(qt_base, "lib_orig", "Qt*"))
    for orig_fw_path in framework_list:
        other_fw_path = orig_fw_path.replace("lib_orig", "lib").replace(qt_base, qt_base_2)
        if(os.path.isdir(other_fw_path)):
            fw_name = orig_fw_path.split("/")[-1].split(".")[0]
            if(os.path.isfile(os.path.join(orig_fw_path, "Versions", "5", fw_name))):
                new_path = shutil.copytree(orig_fw_path, orig_fw_path.replace("lib_orig", "lib"), symlinks=True)
                os.remove(os.path.join(new_path, "Versions", "5", fw_name))
                print(f"Processing {fw_name}.framework")
                fix_library(f"{orig_fw_path}/Versions/5/{fw_name}", True)
                fix_library(f"{other_fw_path}/Versions/5/{fw_name}", True)
                os.system(f"lipo -create -output {new_path}/Versions/5/{fw_name} {orig_fw_path}/Versions/5/{fw_name} {other_fw_path}/Versions/5/{fw_name}")
            else:
                src = os.path.join(qt_base, "lib_orig", f"{fw_name}.framework")
                dst = os.path.join(qt_base, "lib", f"{fw_name}.framework")
                if(os.path.isdir(src)):
                    print(f"Processing {fw_name}.framework (Header Only)")
                    shutil.copytree(src, dst)

def process_static_libs():
    static_lib_list = glob.glob(os.path.join(qt_base, "lib_orig", "lib*.a"))
    for orig_lib_path in static_lib_list:
        other_lib_path = orig_lib_path.replace("lib_orig", "lib").replace(qt_base, qt_base_2)
        if(os.path.isfile(other_lib_path)):
            new_lib_path = orig_lib_path.replace("lib_orig", "lib")
            lib_name = orig_lib_path.split("/")[-1]
            print(f"Processing {lib_name}")
            prl_src = orig_lib_path[:-1] + "prl"
            prl_dst = prl_src.replace("lib_orig", "lib")
            shutil.copy(prl_src, prl_dst)
            os.system(f"lipo -create -output {new_lib_path} {orig_lib_path} {other_lib_path}")

def process_plugins():
    plugin_list = glob.glob(os.path.join(qt_base, "plugins_orig", "*", "*.dylib"))
    for orig_plugin_path in plugin_list:
        other_plugin_path = orig_plugin_path.replace("plugins_orig", "plugins").replace(qt_base, qt_base_2)
        if(os.path.isfile(other_plugin_path)):
            plugin_name = orig_plugin_path.split("/")[-1]
            new_plugin_path = orig_plugin_path.replace("plugins_orig", "plugins")
            os.makedirs(os.path.dirname(new_plugin_path), exist_ok=True)
            print(f"Processing {plugin_name}")
            fix_library(orig_plugin_path, False)
            fix_library(other_plugin_path, False)
            os.system(f"lipo -create -output {new_plugin_path} {orig_plugin_path} {other_plugin_path}")

def prepare_folders():
    if(not os.path.exists(lib_orig)):
        shutil.move(os.path.join(qt_base, "lib"), lib_orig)
    if(os.path.exists(os.path.join(qt_base, "lib"))):
        shutil.rmtree(os.path.join(qt_base, "lib"))

    if(not os.path.exists(plugins_orig)):
        shutil.move(os.path.join(qt_base, "plugins"), plugins_orig)
    if(os.path.exists(os.path.join(qt_base, "plugins"))):
        shutil.rmtree(os.path.join(qt_base, "plugins"))

    os.makedirs(os.path.join(qt_base, "lib"))
    os.makedirs(os.path.join(qt_base, "plugins"))

    for name in ["cmake", "metatypes", "pkgconfig"]:
        shutil.copytree( os.path.join(lib_orig, name), os.path.join(qt_base, "lib", name))

if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 2:
        print("Usage: {} QT_BASE QT_2ND_BASE\n\n".format(sys.argv[0]))
        print("e.g: {} ./qt_intel/5.15.2/ ./qt_arm64/5.12.2/".format(sys.argv[0]))
        exit(0)

    qt_base = os.path.abspath(args[0])
    qt_base_2 = os.path.abspath(args[1])

    lib_orig = os.path.join(qt_base, "lib_orig")
    plugins_orig = os.path.join(qt_base, "plugins_orig")


    prepare_folders()
    process_framework()
    process_static_libs()
    process_plugins()
