# C++ Derleyici Kurulum Rehberi (Windows)

## Sorun
`g++` komutu sisteminizde bulunamıyor. C++ kodlarınızı derlemek için bir derleyici kurmanız gerekiyor.

## Çözüm Seçenekleri

### ✅ Seçenek 1: WinLibs MinGW-w64 (Önerilen - En Hızlı)

1. **İndirme:**
   - https://winlibs.com/ adresine gidin
   - "UCRT runtime" + "Release" + "64-bit" seçeneğini indirin
   - Örnek: `mingw-w64-ucrt-x86_64-13.2.0-16.0.6-11.0.0-ucrt-r1.zip`

2. **Kurulum:**
   - Zip dosyasını açın
   - İçeriği `C:\mingw64` klasörüne çıkarın

3. **PATH'e Ekleme:**
   - Windows tuşu + R → `sysdm.cpl` yazın → Enter
   - "Gelişmiş" sekmesi → "Ortam Değişkenleri"
   - "Sistem değişkenleri" altında `Path`'i seçin → "Düzenle"
   - "Yeni" → `C:\mingw64\bin` yazın → Tamam
   - Tüm pencereleri kapatın

4. **Doğrulama:**
   - Yeni bir PowerShell/CMD penceresi açın
   - `g++ --version` komutunu çalıştırın
   - Versiyon bilgisi görünmeli

### ✅ Seçenek 2: MSYS2 (Paket Yöneticili)

1. **İndirme ve Kurulum:**
   - https://www.msys2.org/ adresinden MSYS2'yi indirin
   - Kurulum sihirbazını çalıştırın (varsayılan: `C:\msys64`)

2. **Derleyici Kurulumu:**
   - MSYS2 UCRT64 terminalini açın
   - Şu komutu çalıştırın:
     ```bash
     pacman -S mingw-w64-ucrt-x86_64-gcc
     ```
   - Onay için `Y` yazın

3. **PATH'e Ekleme:**
   - `C:\msys64\ucrt64\bin` klasörünü PATH'e ekleyin (yukarıdaki adımları takip edin)

### ⚠️ Önemli Notlar

- PATH değişikliklerinin etkili olması için **VS Code'u tamamen kapatıp yeniden açın**
- Yeni terminal pencereleri açın (eski pencereler eski PATH'i kullanır)
- Kurulumdan sonra `g++ --version` ile test edin

## Kurulum Sonrası

1. VS Code'u kapatıp yeniden açın
2. `helloworld.cpp` dosyasını açın
3. `Ctrl+Shift+B` ile derleyin (veya Terminal → Run Build Task)
4. `F5` ile debug edin

## Sorun Giderme

**"g++ hala bulunamıyor" hatası:**
- PATH'e doğru klasörü eklediğinizden emin olun (`bin` klasörü içinde `g++.exe` olmalı)
- VS Code'u tamamen kapatıp yeniden açın
- Yeni bir terminal penceresi açın
- `where g++` komutu ile konumunu kontrol edin

**Alternatif: Visual Studio Build Tools**
- Visual Studio Installer'ı indirin
- "Desktop development with C++" iş yükünü seçin
- `task.json` ve `launch.json` dosyalarını `cl.exe` kullanacak şekilde güncelleyin

