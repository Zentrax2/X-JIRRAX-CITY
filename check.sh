#!/bin/sh
# Pemeriksaan tanpa PSPSDK:
#  1. Sintaks C semua src/*.c terhadap header PSP PALSU di tests/pspstub.
#  2. Test logika host (kota, jalan, mobil, NPC) dengan renderCube palsu.
# BUKAN pengganti build EBOOT.PBP dengan PSPSDK.
set -e
cd "$(dirname "$0")/.."
echo "== [1/2] Sintaks C (stub headers) =="
gcc -std=gnu11 -Wall -Wextra -fsyntax-only -Itests/pspstub -Isrc src/*.c
echo "syntax OK (stub headers)"
echo "== [2/2] Test logika host =="
gcc -std=gnu11 -O2 -Wall -Wextra -Isrc -o /tmp/xjirrax_test tests/test_logic.c \
    src/vehicle.c src/world.c src/npc.c -lm
/tmp/xjirrax_test
