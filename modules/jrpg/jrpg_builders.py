"""Build-time helpers for modules/jrpg."""

import hashlib
import os

import methods

# Text files are stored with LF line endings whatever the checkout does, so hashes match on every machine.
TEXT_SUFFIXES = {".gd", ".tscn", ".tres", ".cfg", ".md", ".txt", ".json", ".gdshader", ".csv", ".svg"}
ADDON_FORMAT = 1


ADDON_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "addon_template")


def addon_relative_path(path):
    # Relative to this module's addon_template, whatever folder names the checkout path holds.
    return os.path.relpath(os.path.abspath(path), ADDON_DIR).replace("\\", "/")


def make_addon_files(target, source, env):
    """Embed modules/jrpg/addon_template/** (the managed addon, D-004) as compressed buffers with SHA-256 hashes."""
    files = []
    for node in sorted(source, key=lambda n: addon_relative_path(str(n))):
        path = str(node)
        data = methods.get_buffer(path)
        if os.path.splitext(path)[1].lower() in TEXT_SUFFIXES:
            data = data.replace(b"\r\n", b"\n")
        files.append((addon_relative_path(path), data))

    content_hash = hashlib.sha256()
    for relative, data in files:
        content_hash.update(relative.encode("utf-8") + b"\0" + hashlib.sha256(data).digest())
    version = f"{ADDON_FORMAT}-{content_hash.hexdigest()[:12]}"

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(
            """\
#include "core/typedefs.h"

struct JRPGAddonFile {
	const char *path;
	const char *sha256;
	int size;
	int compressed_size;
	const unsigned char *data;
};

"""
        )
        file.write(f'inline constexpr const char *JRPG_ADDON_VERSION = "{version}";\n\n')
        for index, (relative, data) in enumerate(files):
            compressed = methods.compress_buffer(data)
            file.write(f"inline constexpr const unsigned char _jrpg_addon_file_{index}[] = {{\n")
            file.write(methods.format_buffer(compressed, 1))
            file.write("\n};\n\n")
        file.write("inline constexpr JRPGAddonFile JRPG_ADDON_FILES[] = {\n")
        for index, (relative, data) in enumerate(files):
            compressed_size = len(methods.compress_buffer(data))
            digest = hashlib.sha256(data).hexdigest()
            file.write(f'\t{{ "{relative}", "{digest}", {len(data)}, {compressed_size}, _jrpg_addon_file_{index} }},\n')
        file.write("};\n\n")
        file.write(f"inline constexpr int JRPG_ADDON_FILE_COUNT = {len(files)};\n")


# The built-in window skin (D-049): file stem in talewright/assets/generated/system -> name in the generated header.
DEFAULT_SKIN = {
    "window_back": "BACK",
    "window_border": "BORDER",
    "window_cursor": "CURSOR",
    "window_arrows": "ARROWS",
    "icons": "ICONS",
}


def make_default_skin(target, source, env):
    """Embed the generated window skin PNGs (as they are: already compressed) for JRPGWindow's built-in skin."""
    with methods.generated_wrapper(str(target[0])) as file:
        file.write('#include "core/typedefs.h"\n\n')
        for node in source:
            path = str(node)
            name = DEFAULT_SKIN[os.path.splitext(os.path.basename(path))[0]]
            data = methods.get_buffer(path)
            file.write(f"inline constexpr int JRPG_SKIN_{name}_SIZE = {len(data)};\n")
            file.write(f"inline constexpr const unsigned char JRPG_SKIN_{name}[] = {{\n")
            file.write(methods.format_buffer(data, 1))
            file.write("\n};\n\n")
