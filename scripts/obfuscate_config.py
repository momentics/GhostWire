#!/usr/bin/env python3

import sys
import os
import secrets

KEY_LENGTH = 1


def obfuscate(input_path: str, output_path: str) -> None:
    with open(input_path, "rb") as f:
        data = f.read()

    if not data:
        print("ERROR: config.json is empty!", file=sys.stderr)
        sys.exit(1)

    key = secrets.token_bytes(KEY_LENGTH)
    obfuscated = bytes(data[i] ^ key[i % KEY_LENGTH] for i in range(len(data)))

    lines = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "",
        f"static const unsigned char g_config_key[{KEY_LENGTH}] = {{",
    ]

    for i in range(0, KEY_LENGTH, 16):
        chunk = key[i : i + 16]
        hex_values = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_values},")
    lines.append("};")
    lines.append("")
    lines.append(f"static const unsigned char g_config_data[{len(data)}] = {{")

    for i in range(0, len(obfuscated), 16):
        chunk = obfuscated[i : i + 16]
        hex_values = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_values},")

    lines += [
        "};",
        "",
        f"static const size_t g_config_size = {len(data)};",
        "",
        f"static const size_t g_config_key_len = {KEY_LENGTH};",
        "",
    ]

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"OK: {input_path} -> {output_path} ({len(data)} bytes, key={key.hex().upper()})")


def main():
    if len(sys.argv) != 3:
        print("Usage: python obfuscate_config.py <input.json> <output.h>", file=sys.stderr)
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    if not os.path.isfile(input_path):
        print(f"ERROR: file not found: {input_path}", file=sys.stderr)
        sys.exit(1)

    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.isdir(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    obfuscate(input_path, output_path)


if __name__ == "__main__":
    main()
