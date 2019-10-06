import sys
import os
import shutil
import compileall

src_root = sys.argv[1]
target_prefix = os.path.join(sys.argv[2], "lib", "python{}.{}".format(sys.version_info.major, sys.version_info.minor),"site-packages")


if not os.path.isdir(src_root):
    sys.exit("No such directory: {}".format(src_root))

path_offset = len(os.path.dirname(src_root))
for path,_ ,fnames in os.walk(src_root):
    target_path = os.path.join(target_prefix, path[path_offset+1:])
    if not os.path.isdir(target_path):
        os.makedirs(target_path)

    for f in fnames:
        _, ext = os.path.splitext(f)
        if ext == ".pyc":
            continue

        src_file = os.path.join(path, f)
        target_file = os.path.join(target_path, f)
        shutil.copy(src_file, target_file)

target_root = os.path.join(target_prefix, os.path.basename(src_root))
compileall.compile_dir(target_root)
