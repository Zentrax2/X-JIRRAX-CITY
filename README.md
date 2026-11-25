# X-JIRRAX CITY — PSP Homebrew (prototipe v0.2)

Prototipe game open-world PSP homebrew (C + PSPSDK, target PPSSPP Android).
Versi 0.3: kota fiktif 8×8 blok dengan arketipe Indonesia (kampung, pasar, ruko, masjid), jalan rusak, dan 64 NPC dengan jadwal harian. Ini masih **prototipe**, bukan game jadi. Status lengkap ada di `STATUS_PROJECT.md`.

> **Penting:** sesi ini belum pernah membangun atau menjalankan EBOOT.PBP dengan PSPSDK asli,
> dan belum diuji di PPSSPP. Lihat bagian "Belum diverifikasi" di STATUS_PROJECT.md.

## Cara build tanpa PC (GitHub Actions)
1. Buat repository GitHub baru.
2. Upload isi folder proyek ini (Makefile, src/, tests/, .github/) ke root repository.
   Pastikan folder `.github` ikut ter-upload, karena itu berisi workflow build.
3. Buka tab **Actions**. Workflow "Build EBOOT.PBP" berjalan otomatis; atau pilih **Run workflow**.
4. Setelah hijau, buka run tersebut lalu unduh artifact **XJIRRAX_CITY** (berisi EBOOT.PBP).
5. Salin `EBOOT.PBP` ke folder `PSP/GAME/XJIRRAX_CITY/` di Memory Stick PPSSPP
   (lokasi Memory Stick bisa dicek di pengaturan PPSSPP), lalu buka dari menu Game di PPSSPP.

## Cara build dengan PC (opsional)
Pasang PSPSDK lalu jalankan `make` di folder proyek. Hasilnya `EBOOT.PBP`.

## Cek tanpa PSPSDK
`sh tests/check.sh`
- Cek sintaks C terhadap header PSP palsu (`tests/pspstub/`). Ini bukan build PSPSDK.
- Test logika host untuk mobil dan dunia (`tests/test_logic.c`).

## Kontrol
Lihat CONTROLS.txt.

## Struktur kode
- `src/main.c` — loop utama (input → update → render)
- `src/callbacks.*` — exit callback sistem
- `src/input.*` — D-pad, stik analog, tombol aksi
- `src/render.*` — setup GU, palet material, `renderCube`
- `src/camera.*` — kamera chase di belakang mobil
- `src/world.*` — kota 8×8 blok: distrik, trotoar, zebra, jalan rusak
- `src/npc.*` — 64 NPC dengan jadwal harian dan rute di trotoar
- `src/vehicle.*` — fisika mobil sederhana dan model mobil

## Fitur prototipe saat ini
- Kamera chase orbit mengikuti mobil
- Mobil yang bisa dikendalikan (gas, rem, setir)
- Kota blok 9×9 prosedural, tanpa aset eksternal
- Jalan, marka putus-putus, gedung dengan atap, pohon palem

## Rencana berikutnya
1. Ganti kubus prototipe dengan mesh low-poly yang dioptimasi dan tekstur.
2. District streaming supaya kota yang lebih besar dimuat per area.
3. Lalu lintas dan pejalan kaki dengan batas entitas yang ketat.
4. Misi, penyimpanan progres, audio, dan menu pause/pengaturan.
5. Profil di PPSSPP dan tuning draw call, visibilitas, dan frame pacing.

## Catatan
Proyek ini original dan tidak memakai aset, peta, musik, atau file game GTA maupun game berhak cipta lain.
