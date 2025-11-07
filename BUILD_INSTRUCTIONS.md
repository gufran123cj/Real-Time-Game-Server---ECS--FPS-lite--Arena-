# 🚀 Projeyi Çalıştırma Rehberi

## Hızlı Başlangıç

### Yöntem 1: Batch Script (Önerilen - En Kolay)

1. **Derleme:**
   ```bash
   build.bat
   ```

2. **Çalıştırma:**
   ```bash
   run.bat
   ```
   
   Veya özel port/tick rate ile:
   ```bash
   run.bat 8888 120
   ```

### Yöntem 2: VS Code

1. **Derleme:** `Ctrl+Shift+B` (veya Terminal → Run Build Task)
2. **Çalıştırma:** Terminal'de `run.bat` yazın

### Yöntem 3: Manuel Komut Satırı

PowerShell veya CMD'de:

```bash
# MinGW path'ini ayarla (gerekirse)
$env:PATH += ";D:\MinGW\bin"

# Derle
g++ -std=c++20 -O3 -Wall -Wextra -I. -Iinclude -Iecs -Inet -Iphysics -Imatchmaker -Ianti-cheat-lite src\main.cpp src\Server.cpp ecs\Component.cpp ecs\World.cpp net\Socket.cpp net\Snapshot.cpp physics\Physics.cpp matchmaker\Matchmaker.cpp anti-cheat-lite\AntiCheat.cpp -o GameServer.exe -lws2_32

# Çalıştır
.\GameServer.exe
```

## Gereksinimler

- ✅ **MinGW-w64** (g++ derleyicisi)
  - Path: `D:\MinGW\bin\g++.exe`
  - Eğer farklı bir yerdeyse, `build.bat` dosyasındaki `MINGW_PATH` değişkenini güncelleyin

- ✅ **C++20 desteği** (g++ 10+)

## Derleme Hataları

### "g++ bulunamadı" hatası
- MinGW kurulu mu kontrol edin
- `build.bat` içindeki `MINGW_PATH` değerini kontrol edin
- PATH environment variable'ına MinGW\bin ekleyin

### "C++20 standard not found" hatası
- g++ versiyonunu kontrol edin: `g++ --version`
- En az g++ 10.0 gereklidir
- WinLibs veya MSYS2'den güncel MinGW indirin

### Linker hataları (ws2_32)
- Windows'ta normal, `-lws2_32` flag'i eklenmiş olmalı
- `build.bat` içinde zaten var

## Çalıştırma

Sunucu başladığında:
```
=== Game Server (FPS-lite / Arena) ===
C++20 | ECS | Authoritative Server
=====================================
Game Server initialized on 0.0.0.0:7777 (Tick Rate: 60)
Server running. Press Ctrl+C to stop.
```

### Parametreler

```bash
GameServer.exe [PORT] [TICK_RATE]
```

- **PORT**: Sunucu portu (varsayılan: 7777)
- **TICK_RATE**: Tick rate (varsayılan: 60)

Örnekler:
```bash
GameServer.exe              # Port 7777, 60 tick
GameServer.exe 8888         # Port 8888, 60 tick
GameServer.exe 8888 120     # Port 8888, 120 tick
```

## Test Etme

Sunucu çalıştıktan sonra, başka bir terminal'de test edebilirsiniz:

```bash
# Windows'ta netcat veya PowerShell ile test
Test-NetConnection -ComputerName localhost -Port 7777
```

Veya basit bir UDP test client'ı yazabilirsiniz.

## Sorun Giderme

### Port zaten kullanılıyor
```bash
# Windows'ta port kullanımını kontrol et
netstat -ano | findstr :7777

# Process'i sonlandır (PID'yi bulun)
taskkill /PID <PID> /F
```

### Firewall uyarısı
- Windows Firewall sunucuya izin isteyebilir
- "Private networks" için izin verin

### Sunucu çöküyor
- Debug modda derleyin: `build.bat` içinde `-O3` yerine `-g -O0` kullanın
- Log'lara bakın (şu an sadece console output var)

## Sonraki Adımlar

1. ✅ Sunucuyu çalıştırın
2. ⏳ Client bağlantısı test edin
3. ⏳ Network paketlerini test edin
4. ⏳ ECS sistemlerini test edin

## Notlar

⚠️ **Önemli:**
- Sunucu şu an sadece UDP paketlerini dinliyor
- Gerçek bir client olmadan sadece bağlantı testleri yapabilirsiniz
- Production kullanımı için ek güvenlik ve optimizasyonlar gereklidir

