# 1. Technologie
## Hardware
  1. Mikrokontroler -- ESP32
  2. Akcelerometr -- 3-osiowy akcelerometr cyfrowy MMA8452 I2C - HW-616
  3. Moduł ładowania -- Lipo Charger - moduł ładujący do akumulatorów Li-Pol poprzez USB C - DFRobot DFR0668
  4. Akumulator -- Akumulator Li-Pol Akyga 2200mAh 1S 3,7V - przewody 50mm - 59x37x9mm
## Software
1. Mikrokontroler - C/C++
2. Serwer AWS (lub inny tego typu)
3. Baza Danych SqLite (lub inna podobna)
4. Backend - Node.js
5. Frontend - Aplikacja webowa Html, JS (jeśli starczy czas z innym frameworkiem)
# 2. Kamienie Milowe

### 04.04.2024 - Układ mikrokontrolera
- [X]  Połączenie wszystkich elementów
- [X]  Działające ładowanie z baterii
- [X]  Akcelerometr wykrywający zmiany w położeniu
### 18.04.2024 - Program kostki
- [ ]  Prototyp obudowy
- [ ]  Wykrywanie wszystkich ścianek kostki (5 z projektami i 1 na przerwę/ładowanie)
- [ ]  Zabezpieczenie przed "bawieniem się" kostką
- [ ]  Zliczanie czasu nad każdą ścianką
- [ ]  Konfiguracja wifi
### 02.05.2024 - Wysyłanie/odbieranie danych z kostki na serwer
- [ ]  Działający serwer AWS (lub inny)
- [ ]  Wysyłanie danych z kostki na serwer
- [ ]  Odbieranie danych na serwerze
- [ ]  Zapis danych do bazy danych
### 23.05.2024 - Przetwarzanie i wyświetlanie danych
- [ ]  Wyświetlanie zapisanych danych
- [ ]  Logowanie Google
- [ ]  Wizualizacja pracy nad projektami
### 31.05.2024 - Oddanie Projektu
- [ ]  Skończona obudowa
- [ ]  Drugi działający prototyp
- [ ]  Poprawki pozostałych elementów
- [ ]  Potencjalne rozszerzenie funkcjonalności
