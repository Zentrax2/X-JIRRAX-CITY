# CHANGELOG — X-JIRRAX CITY

## v0.3.0 — Sesi 2: kota berblok ala Indonesia + NPC jadwal harian

### Ditambahkan
- `src/world.c` (ditulis ulang): kota 8×8 blok (320×320 unit), graf trotoar 324 sudut dengan zebra cross.
- Enam tipe blok: kampung (rumah rapat, gang, warung, pos ronda), ruko (dua deret 3–4 lantai, tangki air, motor parkir), pasar (atap besar di tiang, meja dagang), perumahan (3×3 rumah seragam dengan pagar), masjid (kubah dan dua menara), lapangan (pohon, rumput).
- Jalan rusak: lubang dan tambalan aspal di tiap ruas, deterministik. Lubang memperlambat mobil.
- Tiang listrik dengan palang, lajur putus-putus, zebra cross di setiap persimpangan.
- `src/npc.c`: 64 NPC dengan 5 pekerjaan (pedagang warung, pekerja ruko, ibu rumah tangga ke pasar, pensiunan di pos ronda, tukang ojek). Jadwal harian dalam jam: subuh di masjid, sarapan dan makan siang di warung, kerja, ngobrol sore, pulang malam, tidur di rumah (tidak tergambar).
- Rute NPC: BFS di graf trotoar. NPC berjalan di trotoar dan menyeberang lewat zebra.
- Jam dalam game: 60 frame = 1 menit game (sehari = 24 menit nyata di 60 FPS). Di FPS lebih rendah jam berjalan lebih lambat.
- Kabut jarak (`sceGuFog`) untuk menyembunyikan kemunculan blok di tepi area gambar.
- Mobil hanya bisa di badan jalan (`worldClampToRoad`).
- Test: `tests/test_logic.c` menguji graf, peran POI, bangunan di dalam blok, lubang jalan, mobil di jalan, dan simulasi NPC satu hari penuh (19 cek).

### Diubah
- `render.h/.c`: daftar material diperluas menjadi 49 material dengan palet designated initializer.
- `vehicle.c`: mulai di pusat kota, dibatasi jalan, melambat di lubang.
- `main.c`: loop dengan NPC dan kota baru.
- `Makefile`: `OBJS` ditambah `src/npc.o`.
- `tests/check.sh`: cek sintaks dan test host, keluar dengan kode galat yang benar.

### Belum ada (sengaja, bukan lupa)
- Peta seluruh Indonesia yang nyata: tidak muat di PSP. Kota ini fiktif, dengan arketipe Indonesia.
- Masuk ke gedung: belum ada interior.
- Suara: belum ada audio.
- Lalu lintas: belum ada mobil lain; hanya mobil pemain.
- Siang dan malam secara visual: baru jam dalam game yang dipakai jadwal NPC.
- Tekstur dan detail bentuk: semua masih kubus.

### Belum diuji
- Build EBOOT.PBP dengan PSPSDK asli.
- Tampilan, FPS, dan kebutuhan RAM di PPSSPP. Perkiraan 622 kubus untuk kota per frame, ditambah NPC yang tergambar.


## v0.2.0 — Sesi 1 (Claude): audit + rapikan Phase 1

### Ditambahkan
- Modul terpisah: `callbacks`, `input`, `render`, `camera`, `world`, `vehicle` (sebelumnya semua di `main.c`).
- Exit callback sistem lewat `src/callbacks.c` (menu HOME → keluar).
- Input stik analog kiri untuk setir, dengan deadzone 0.15. D-pad tetap berfungsi.
- Palet material (11 material). Vertex color kubus dibake sekali saat startup.
- Test logika host: `tests/test_logic.c` (tanpa PSP SDK).
- Cek sintaks dengan header PSP palsu: `tests/check.sh` + `tests/pspstub/`.
- Workflow GitHub Actions: `.github/workflows/build.yml`. Build EBOOT.PBP di cloud untuk pengguna tanpa PC.
- `CHANGELOG.md` dan `STATUS_PROJECT.md`.

### Diubah
- `src/main.c`: ditulis ulang menjadi loop utama yang ringkas. Fungsi yang sama dipertahankan.
- `Makefile`: `OBJS` mencakup semua modul baru. Flag dan lib tidak diubah.
- Display list diperkecil dari 1 MB (262144 word) menjadi 512 KB (131072 word).
- Warna material ditulis sebagai ARGB (sesuai niat penulis awal) lalu dikonversi ke format ABGR yang dipakai GU.
- Bumper mobil kini bergeser di sumbu X dan Z sesuai arah hadap. Sebelumnya hanya Z.
- Gedung, pohon, dan mobil diposisikan dari permukaan tanah/jalan. Sebelumnya melayang sekitar 0.1–0.45 unit.
- Yaw mobil dibungkus ke ±π agar tidak membesar tanpa batas.
- `README.md` dan `CONTROLS.txt` diperbarui.

### Diperbaiki
- Parameter `tint` di `drawCube` diabaikan (`(void)tint`), sehingga semua objek tampil abu-abu. Sekarang warna material benar-benar dipakai.
- Warning `-Wall`: variabel `indices` tidak dipakai. Dihapus.
- Kode dummy dihapus: `frameCount` (tidak pernah dibaca), `#include <pspdebug.h>` (tidak dipakai), dan `sceGuTerm()`/`sceKernelExitGame()` yang tidak terjangkau setelah `while(1)`.
- Tidak ada exit callback. Ditambahkan.

### Belum diuji
- Build EBOOT.PBP dengan PSPSDK asli belum pernah dijalankan. Lingkungan sesi ini tidak punya PSPSDK dan tidak punya akses jaringan.
- Belum diuji di PPSSPP.
- Workflow GitHub Actions belum pernah dijalankan.
