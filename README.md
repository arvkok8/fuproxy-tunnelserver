# fuproxy-tunnelserver

## Gerekli kütüphanelerin indirilmesi
Ubuntu için:
```
sudo apt update
sudo apt install libboost-all-dev
```
Arch için:
```
sudo pacman -Sy boost boost-libs
```

## Repository'nin hazırlanması
Proje üstünde çalışmak istediğiniz klasöre girip:
```
git clone https://github.com/arvkok8/fuproxy-tunnelserver.git
cd fuproxy-tunnelserver
git submodule init
git submodule update
```

## Kullanıcı adı ve Personel Acces Token(PAT)'i hatırlamak için
Giriş bilgilerini sadece bu proje için hatırlasın istiyorsanız proje klasörüne girip:
```
git config credential.helper store
```
Sistem genelinde saklamak istiyorsanız:
```
git config --global credential.helper store
```
