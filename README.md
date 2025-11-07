# Game Server - FPS-lite / Arena

Gerçek zamanlı oyun sunucusu projesi. C++20, ECS mimarisi, authoritative server, 128+ oyuncu desteği.

## Özellikler

- ✅ **ECS (Entity-Component-System)** mimarisi
- ✅ **Network katmanı** (UDP socket abstraction)
- ✅ **Oda tabanlı** (room-based) sunucu
- ✅ **60/120 tick** server loop
- ✅ **Physics** katmanı (BVH spatial partitioning)
- ✅ **Matchmaker** temel yapısı
- ✅ **Anti-cheat-lite** temel kontrolleri
- ✅ **Snapshot** yönetimi (delta compression hazır)
- ⏳ Lag compensation (TODO)
- ⏳ Rollback/rewind (TODO)
- ⏳ Deterministik simülasyon (TODO)

## Proje Yapısı

```
Projects/
├── include/
│   └── common/
│       └── types.hpp          # Temel type tanımları
├── ecs/
│   ├── Entity.hpp             # Entity sınıfı
│   ├── Component.hpp          # Component base ve registry
│   ├── Component.cpp
│   ├── System.hpp             # System base (template)
│   └── World.hpp/cpp          # ECS World
├── net/
│   ├── Socket.hpp/cpp         # UDP socket wrapper
│   ├── Packet.hpp             # Packet reader/writer
│   └── Snapshot.hpp/cpp       # Snapshot ve delta compression
├── physics/
│   ├── Physics.hpp            # Vec3, AABB, BVH
│   └── Physics.cpp
├── matchmaker/
│   ├── Matchmaker.hpp/cpp     # Oyun eşleştirme
└── anti-cheat-lite/
    ├── AntiCheat.hpp/cpp      # Temel anti-cheat kontrolleri
└── src/
    ├── Server.hpp/cpp         # Ana sunucu sınıfı
    └── main.cpp               # Entry point
```

## Derleme

### Windows (MinGW)

```bash
# CMake ile
mkdir build
cd build
cmake ..
cmake --build .

# Veya direkt g++ ile
g++ -std=c++20 -O3 -Wall -Wextra \
    src/*.cpp ecs/*.cpp net/*.cpp physics/*.cpp \
    matchmaker/*.cpp anti-cheat-lite/*.cpp \
    -o GameServer -lws2_32
```

### Linux

```bash
mkdir build && cd build
cmake ..
make
```

## Kullanım

```bash
# Varsayılan: port 7777, 60 tick
./GameServer

# Özel port ve tick rate
./GameServer 8888 120
```

## Teknik Detaylar

### ECS Mimarisi

- **Entity**: ID + component mask
- **Component**: Type-safe component registry
- **System**: Template-based system with component filtering
- **World**: Entity/component/system yönetimi

### Network

- UDP socket abstraction (Windows/Linux)
- Packet header: type, sequence, tick, playerID
- Snapshot history (64 snapshot)
- Delta compression hazır (implementation TODO)

### Physics

- BVH (Bounding Volume Hierarchy) spatial partitioning
- AABB collision detection
- Vec3 matematik

### Matchmaker

- Rating-based matching
- Configurable tolerance
- Team size support

### Anti-Cheat

- Packet rate limiting
- Movement speed validation
- Suspicious action tracking

## Geliştirme Durumu

**Tamamlanan:**
- ✅ Temel mimari
- ✅ ECS framework
- ✅ Network katmanı
- ✅ Sunucu çerçevesi
- ✅ Physics temel yapısı

**Devam Eden:**
- ⏳ Snapshot serialization
- ⏳ Delta compression implementation
- ⏳ Lag compensation
- ⏳ Rollback/rewind

**Planlanan:**
- 📋 Deterministik simülasyon
- 📋 Lua/AngelScript scripting
- 📋 Glicko-2 rating sistemi
- 📋 Profiling tools

## Notlar

⚠️ **Önemli:** Bu proje aktif geliştirme aşamasındadır. Production kullanımı için ek testler ve optimizasyonlar gereklidir.

## Lisans

Bu proje eğitim/öğrenme amaçlıdır.

