# Donanım entegrasyonu

Kontrol çekirdeği GPIO veya Arduino kütüphanesi kullanmaz. Aynı başlık dosyası masaüstünde test edilebilir ve kart projesine taşınabilir. Henüz Arduino/ESP32 araç zincirinde derlenip fiziksel kartta denenmemiştir.

## Bağlantı katmanı

Kartın ana döngüsü şu sırayı izlemeli:

```cpp
// Entegrasyon taslağıdır; aşağıdaki donanım fonksiyonları henüz yoktur.
const uint32_t now = millis();
bubbleoxy::Input input = hardware.readSnapshot(now);
const bool acknowledge = hardware.consumeFaultAcknowledgement();
bubbleoxy::Output command = controller.tick(now, input, acknowledge);
hardware.applyOutputs(command.pump, command.compressor);
telemetry.enqueue(now, input, command); // kontrol döngüsünü bloke etmemeli
```

Başlatma sırasında fiziksel çıkışlar sensör başlatılmadan önce kapalı konuma alınmalı. Aktif-low röleler için kapalı seviye sürücü katmanında açıkça tanımlanmalı. Kart reseti, bağlantı kaybı ve watchdog durumlarında çıkışların güvenli kalması yalnızca yazılıma bırakılmamalı.

## Seçilecek bileşenler

| Bileşen | Gerekli bilgiler |
| --- | --- |
| Çözünmüş oksijen sensörü | Model, ölçüm aralığı, kalibrasyon, arayüz, deniz suyu uygunluğu, telafi gereksinimleri |
| Akış sensörü | Çalışma aralığı, çıkış türü, akış doğrulama eşiği |
| Pompa ve kompresör | Besleme, nominal/kalkış akımı, sürücü, hız kontrolü, açma/kapama sırası |
| Batarya/BMS | Kimya, kapasite, SOC arayüzü, kesme ve toparlanma koşulları |
| Güneş şarj kontrolcüsü | Batarya uyumu, telemetri arayüzü |
| Koruma girişleri | Kaçak, sıcaklık, akım, basınç ve acil durdurma ihtiyacı |
| Haberleşme | Mesafe, kapsama, anten, güç bütçesi ve veri sıklığı |

## Entegrasyon sırası

1. Masaüstü testlerini çalıştır ve eşikleri simülasyonda incele.
2. Kartta yalnızca seri port/LED çıkışlarıyla aynı durum geçişlerini doğrula.
3. Gerçek sensör sürücülerini ekle; kopuk kablo, eski veri, kalibrasyon ve aralık hatalarını dene.
4. Güç sürücülerini ekipman değerlerine göre seçip yük kontrolünü doğrula.
5. Kontrollü tank deneyinde başlatma, duruş, akış kaybı ve enerji kaybı senaryolarını ölç.
6. Ölçülen debi, basınç, oksijen ve enerji verileriyle ayarları güncelle.
