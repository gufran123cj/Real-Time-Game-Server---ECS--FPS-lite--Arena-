# 🌐 Network Setup - Başka Bilgisayarlardan Bağlanma

## 📋 Gereksinimler

- Server bilgisayarı ve client bilgisayarları aynı ağda olmalı (LAN)
- Veya internet üzerinden bağlanmak için port forwarding gerekli

## 🔧 Adım 1: Server Bilgisayarında IP Adresini Bulma

### Windows'ta:
```powershell
# PowerShell'de çalıştır:
ipconfig

# "IPv4 Address" değerini bul (örnek: 192.168.1.100)
```

### Alternatif (Command Prompt):
```cmd
ipconfig | findstr IPv4
```

## 🖥️ Adım 2: Server'ı Başlatma

Server zaten `0.0.0.0` adresine bind oluyor (tüm network interface'lerini dinler).

```bash
# Varsayılan port (7777) ile:
GameServer.exe

# Veya özel port ile:
GameServer.exe 7777 60
```

Server başladığında şunu göreceksiniz:
```
Game Server initialized on 0.0.0.0:7777 (Tick Rate: 60)
```

## 🔥 Adım 3: Windows Firewall Ayarları

Server bilgisayarında Windows Firewall'u yapılandırın:

### Yöntem 1: PowerShell (Yönetici olarak)
```powershell
# UDP port 7777'yi aç
New-NetFirewallRule -DisplayName "Game Server UDP 7777" -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow
```

### Yöntem 2: Windows Firewall GUI
1. Windows Defender Firewall'u aç
2. "Advanced settings" → "Inbound Rules" → "New Rule"
3. Rule Type: Port → Next
4. Protocol: UDP → Specific local ports: 7777 → Next
5. Action: Allow the connection → Next
6. Profile: Tümünü seç → Next
7. Name: "Game Server UDP 7777" → Finish

## 💻 Adım 4: Client'tan Bağlanma

### Yöntem 1: Command Line'dan IP Belirtme
```bash
# Server IP'si 192.168.1.100 ise:
GameClient.exe 192.168.1.100 7777
```

### Yöntem 2: Kodda Değiştirme (Kalıcı)
`src/GameClient.cpp` dosyasında:
```cpp
std::string serverIP = "192.168.1.100";  // Server'ın IP'si
```

## 🌍 İnternet Üzerinden Bağlanma (Farklı Ağlar)

Eğer client ve server farklı ağlardaysa:

### 1. Router'da Port Forwarding
- Router admin paneline gir (genelde 192.168.1.1 veya 192.168.0.1)
- Port Forwarding / Virtual Server bölümüne git
- UDP port 7777'yi server bilgisayarının local IP'sine yönlendir

### 2. Public IP'yi Bul
```powershell
# Browser'da aç:
# https://whatismyipaddress.com
# veya
# https://www.whatismyip.com
```

### 3. Client'ta Public IP Kullan
```bash
GameClient.exe [PUBLIC_IP] 7777
```

## ⚠️ Güvenlik Notları

- Port forwarding yaparken sadece gerekli port'u açın
- Production'da authentication ekleyin
- Rate limiting aktif edin (anti-cheat)

## 🧪 Test

1. Server'ı başlat: `GameServer.exe`
2. Server bilgisayarında IP'yi bul: `ipconfig`
3. Client bilgisayarında bağlan: `GameClient.exe [SERVER_IP] 7777`
4. İki client aynı server'a bağlanabilmeli

## 📝 Örnek Senaryo

**Server Bilgisayarı:**
- Local IP: `192.168.1.100`
- Port: `7777`
- Komut: `GameServer.exe`

**Client 1 (Aynı Ağ):**
- Komut: `GameClient.exe 192.168.1.100 7777`

**Client 2 (Aynı Ağ):**
- Komut: `GameClient.exe 192.168.1.100 7777`

Her iki client aynı server'a bağlanır ve birlikte oynayabilir!

