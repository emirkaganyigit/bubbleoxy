# Kontrol algoritması

## Durumlar

| Durum | Pompa | Kompresör | Geçiş |
| --- | --- | --- | --- |
| IDLE | Kapalı | Kapalı | Oksijen ≤ alt eşik ve duruş süresi dolunca PRIMING |
| PRIMING | Açık | Kapalı | Kesintisiz akış doğrulaması sonrası AERATING; süre aşımında FAULT |
| AERATING | Açık | Açık | Oksijen ≥ üst eşik ve minimum süre dolunca IDLE |
| LOW_BATTERY | Kapalı | Kapalı | Batarya ≥ toparlanma eşiği olunca IDLE |
| FAULT | Kapalı | Kapalı | Geçerli girişler ve operatör onayı ile IDLE |

Her çağrıda öncelik: geçersiz yapılandırma → acil durum → geçersiz/eski ölçüm → kilitli hata → düşük batarya → normal çalışma. İlk hata nedeni korunur. Arıza onayı bir sonraki çalışma için izin verir; aynı çağrıda ekipmanı açmaz. Onay anında minimum kapalı kalma süresi yeniden başlar.

Oksijenin iki eşiği arasındaki aralık histerezistir: kapalı sistem bu aralıkta başlamaz; çalışan sistem çalışmaya devam eder. Batarya için de ayrı durma/toparlanma eşikleri kullanılır. Minimum çalışma süresi, kompresör devreye girdiğinde başlar. Güvenlik duruşları bu süreyi beklemez.

## Giriş sözleşmesi

- Oksijen: mg/L, sonlu ve negatif olmayan ölçüm; fiziksel üst limit ve ölçüm aralığı sensör sürücüsünde doğrulanmalı.
- Batarya: 0–100 aralığında SOC. Ham voltaj yüzde değildir. SOC üretimi batarya kimyası ve BMS arayüzü seçildikten sonra uygulanmalı.
- `valid`: bütün zorunlu ölçümler geçerli ve kalibrasyon koşulları sağlanmışsa true.
- `sampledAtMs`: bütün zorunlu ölçümlerin güncel olduğu tutarlı örneğin zamanı. Eski sensör değerine yeni zaman etiketi vermeyin. Ayrı örnekleme hızlarında her sensörün yaşını sürücü katmanında izleyin.
- `flow`: pompa akışının doğrulandığı dijital sonuç. Eşik, fiziksel debi sensörüne göre belirlenmeli.
- `emergency`: kaçak, aşırı akım, aşırı sıcaklık veya acil durdurma gibi donanım arayüzünün birleştirdiği hata.

Çekirdek gecikme (`delay`) kullanmaz; `tick` düzenli çağrılmalıdır. Örnek hedef periyot 100 ms'dir, nihai tepki süresi saha risklerine ve donanıma göre belirlenmelidir. PRIMING sırasında akış her çağrıda gözlenir; çağrılar arasındaki fiziksel süreklilik bu yazılım tarafından kanıtlanamaz. Sürücü gerekiyorsa kesme/latch veya donanımsal izleme kullanmalıdır.

32 bit milisaniye farkları unsigned çıkarma ile hesaplanır. Süre ayarları yarım sayaç aralığından küçüktür; döngü uzun süre bloke olmamalı ve ölçüm zamanları aynı saatten gelmelidir. Program yeniden başlarsa kapalı durum ve açılış beklemesi uygulanır. Hata geçmişi yeniden başlatmalar arasında saklanmaz; kalıcı kayıt sonraki iştir.

## Prototipte doğrulanacak varsayımlar

Pompanın kompresörden önce başlatılması ve hata durumunda ikisinin birlikte kapatılması başlangıç varsayımıdır. Gerçek ekipman üreticisinin gerektirdiği sıralama, basınç boşaltma ve duruş gecikmeleriyle güncellenmelidir. Pompanın kuru çalışmasına izin verilen süre belirlenmeden örnek priming süresi kullanılmamalıdır.

Sensör arızasında sistem kapalı kalır. Oksijenlendirme sürekliliği kritikse yedek sensör/ekipman ve ayrı alarm gereksinimi ayrıca tasarlanmalıdır. Ağdan uzaktan komut kabulü uygulanmamıştır; eklendiğinde yerel güvenlik koşullarını atlamamalıdır.

## İlk sürümün kapsamı dışında

- Sensör kalibrasyonu, sıcaklık/tuzluluk/basınç telafisi ve sensör kirlenmesi tespiti.
- Basınç/debiye göre hız kontrolü, PID ve pompa performans modeli.
- MPPT, BMS ve batarya şarj kontrolü.
- Telemetri, kimlik doğrulama, OTA, kalıcı kayıt ve çoklu ünite yönetimi.
- Gerçek nano kabarcık boyutu, oksijen transferi veya kesintisiz güneş enerjili işletmenin doğrulanması.
