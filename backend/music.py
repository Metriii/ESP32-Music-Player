#!/usr/bin/env python3
"""CLI para gerenciar as músicas da ESP32 Music Player (só biblioteca padrão do Python).

Uso (com o PC conectado na rede Wi-Fi da ESP32):
  python3 backend/music.py list
  python3 backend/music.py upload backend/musicas/exemplo.txt [--id exemplo] [--name "Nome"]
  python3 backend/music.py delete exemplo
  python3 backend/music.py play exemplo
  python3 backend/music.py stop
Use --host para outro IP (padrão 192.168.4.1).
"""
import argparse
import json
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

MIN_MS, MAX_MS, MAX_STEPS, MAX_BYTES, ID_MAX = 10, 10000, 512, 4096, 16


def validate(text):
    """Mesmas regras do firmware: 1ª linha opcional '# Nome', durações em ms, quantidade par."""
    body = text
    if text.startswith("#"):
        # Descarta a linha de nome (se o arquivo for só o nome, sobra um corpo vazio)
        body = text.split("\n", 1)[1] if "\n" in text else ""

    nums = re.split(r"[,\s;]+", body.strip())
    if nums == [""] or not all(n.isdigit() for n in nums):
        raise ValueError("o arquivo deve conter apenas números (ms) separados por vírgula/espaço/linha")

    vals = [int(n) for n in nums]
    if len(vals) % 2:
        raise ValueError("quantidade de valores deve ser par (som,pausa,som,pausa...)")
    if len(vals) > MAX_STEPS:
        raise ValueError(f"máximo de {MAX_STEPS} valores")
    if any(v < MIN_MS or v > MAX_MS for v in vals):
        raise ValueError(f"cada valor deve estar entre {MIN_MS} e {MAX_MS} ms")
    if len(text.encode()) > MAX_BYTES:
        raise ValueError(f"arquivo maior que {MAX_BYTES} bytes")


def make_id(path):
    return re.sub(r"[^a-z0-9_-]", "-", path.stem.lower())[:ID_MAX] or "musica"


def call(host, path, data=None):
    req = urllib.request.Request(
        f"http://{host}{path}",
        data=data.encode() if data is not None else None,
        method="POST" if data is not None else "GET",
        headers={"Content-Type": "text/plain"} if data is not None else {},
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as r:
            return json.load(r)
    except urllib.error.HTTPError as e:
        try:
            msg = json.load(e).get("error", e.reason)
        except Exception:
            msg = e.reason
        sys.exit(f"erro {e.code}: {msg}")
    except (urllib.error.URLError, TimeoutError) as e:
        sys.exit(f"não foi possível falar com a ESP32 em {host}: {e}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="192.168.4.1")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list")
    up = sub.add_parser("upload")
    up.add_argument("file", type=Path)
    up.add_argument("--id")
    up.add_argument("--name")
    for c in ("delete", "play"):
        sub.add_parser(c).add_argument("id")
    sub.add_parser("stop")
    a = ap.parse_args()

    if a.cmd == "list":
        for s in call(a.host, "/songs"):
            print(f"{s['id']:<16} {s['name']}")
    elif a.cmd == "upload":
        text = a.file.read_text(encoding="utf-8")
        try:
            validate(text)
        except ValueError as e:
            sys.exit(f"arquivo inválido: {e}")
        sid = a.id or make_id(a.file)
        q = f"/upload?id={urllib.parse.quote(sid)}"
        if a.name:
            q += f"&name={urllib.parse.quote(a.name)}"
        r = call(a.host, q, text)
        print(f"enviado: {r['id']} ({r['name']})")
    elif a.cmd == "delete":
        call(a.host, f"/delete?id={urllib.parse.quote(a.id)}")
        print("apagado:", a.id)
    elif a.cmd == "play":
        print(call(a.host, f"/play?id={urllib.parse.quote(a.id)}"))
    elif a.cmd == "stop":
        print(call(a.host, "/stop"))


if __name__ == "__main__":
    main()
