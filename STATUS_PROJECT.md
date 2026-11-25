# STATUS PROJECT — X-JIRRAX CITY

Versi: v0.3.0 (akhir sesi 2, Claude)

## 1. Ringkasan jujur
- Ini masih **prototipe**. Kota sudah berblok dan ada NPC dengan jadwal harian, tetapi belum ada tekstur, suara, interior, atau lalu lintas.
- Target "seluruh Indonesia, 100% seperti dunia nyata, semua gedung bisa dimasuki, suara, NPC seperti GTA 6" **belum tercapai** dan di PSP tidak realistis dalam satu tahap. Peta yang dibuat adalah kota fiktif dengan arketipe Indonesia.
- EBOOT.PBP **belum pernah dibangun** dengan PSPSDK asli. Sesi ini tidak punya PSPSDK dan tidak punya internet.
- Yang sudah diuji: sintaks C terhadap header PSP palsu, dan logika di host (19 cek lulus, lihat `tests/test_logic.c`).

## 2. File
**Dibuat sesi ini:** `src/npc.c`, `src/npc.h`, `tests/test_logic.c` (ditulis ulang), `tests/check.sh` (diperbarui).
**Ditulis ulang:** `src/world.c`, `src/world.h`, `src/render.c`, `src/render.h`, `src/vehicle.c`, `src/main.c`, `tests/pspstub/pspgu.h`, `Makefile`, `CHANGELOG.md`, `STATUS_PROJECT.md`, `README.md`.
**Tidak berubah:** `src/callbacks.*`, `src/input.*`, `src/camera.*`, `src/vehicle.h`, `.github/workflows/build.yml`, `CONTROLS.txt`.

## 3. Status per PHASE
| Phase | Status | Catatan |
|---|---|---|
| 1 Core engine | Sebagian | Render, kamera, input, exit callback ada. Belum diuji di PSP. |
| 2 Open world | Sebagian | Kota 8×8 blok dengan enam tipe distrik, trotoar, zebra, jalan rusak. Belum ada streaming per area yang sesungguhnya (hanya blok sekitar kamera yang digambar). |
| 3 Gameplay | Sebagian | Mobil di jalan dengan lubang. 64 NPC dengan jadwal dan rute. Belum ada lalu lintas, misi, interaksi bangunan. |
| 4 Polish | Belum | Tekstur, suara, UI, efek, pencahayaan siang-malam. |
| 5 Final build | Belum | Build EBOOT dan uji PPSSPP belum terverifikasi. |

## 4. Fitur yang ada
- Kota 8×8 blok, 320×320 unit, dengan 6 tipe blok (kampung, ruko, pasar, perumahan, masjid, lapangan).
- Graf trotoar dengan zebra cross di setiap persimpangan.
- Jalan rusak (lubang dan tambalan) yang memperlambat mobil.
- Mobil pemain hanya di badan jalan.
- 64 NPC dengan 5 pekerjaan dan jadwal harian dalam jam game (1 menit game = 1 detik nyata).
- NPC pulang dan tidur di rumah (tidak digambar saat di dalam).
- Kabut jarak untuk menyembunyikan kemunculan blok.

## 5. Belum ada
Peta Indonesia yang lebih luas, masuk ke gedung, suara, lalu lintas, siang-malam visual, tekstur, HUD, menu, simpan progres, misi, cuaca.

## 6. Error dan masalah yang diketahui
Belum diverifikasi (perlu build PSPSDK dan PPSSPP):
1. Build `make` dengan PSPSDK asli belum dijalankan.
2. Workflow GitHub Actions belum dijalankan.
3. Arah rotasi `sceGumRotateY` diasumsikan cocok dengan arah depan `(sin yaw, cos yaw)`.
4. Warna material diasumsikan ARGB.
5. Jumlah kubus per frame (sekitar 622 untuk kota, ditambah NPC) kemungkinan berat di PSP. FPS belum diukur. Langkah pertama bila lambat: kurangi `WORLD_VIEW_RADIUS` menjadi 0 atau sederhanakan rumah.
6. Ukuran display list 512 KB belum diukur.

Masalah dari desain:
7. Tidak ada collision antara NPC dan NPC, atau NPC dan mobil. NPC bisa berjalan melewati satu sama lain.
8. NPC menyeberang lewat zebra tanpa mengecek mobil.
9. Tidak ada lalu lintas. Hanya satu mobil.
10. Rumah dan gedung hanya kubus. Tidak ada pintu yang bisa dibuka.
11. Simulasi per frame, bukan delta time. Jam game dihitung 60 frame per menit game: di 60 FPS satu menit game = satu detik nyata, tetapi di 30 FPS jam game berjalan dua kali lebih lambat dan jadwal NPC ikut melambat.

## 7. Hasil pengujian sesi ini
- `sh tests/check.sh`: sintaks semua `src/*.c` lulus dengan `-Wall -Wextra` terhadap header palsu. Kode keluar 0.
- Test logika host lulus 19 cek. Termasuk: graf simetris dan terhubung, bangunan di dalam blok, dinding berdiri di tanah, lubang jalan tidak mendominasi, mobil selalu di jalan, dan simulasi satu hari dengan 64 NPC yang tetap di trotoar, bergerak mulus, dan 80% punya kegiatan.
- Kubus per frame untuk kota di tengah peta: 622.

## 8. Instruksi untuk AI berikutnya
1. Mulai dari ZIP v0.3.0. Jangan hapus modul yang ada.
2. Pastikan build hijau dulu: `make` dengan PSPSDK atau workflow GitHub Actions. Catat error di CHANGELOG. Jangan menambah fitur sebelum build hijau.
3. Setelah build, uji di PPSSPP dan ukur FPS. Catat angka di STATUS. Bila di bawah 30 FPS, turunkan `WORLD_VIEW_RADIUS` atau kurangi kubus pada rumah kampung.
4. Berikutnya yang paling berdampak: collision NPC dengan mobil dan sesama NPC, lalu lalu lintas sederhana di jalan.
5. Setiap sesi wajib menghasilkan ZIP terbaru, CHANGELOG.md, dan STATUS_PROJECT.md. Tandai jelas mana yang sudah diuji.
