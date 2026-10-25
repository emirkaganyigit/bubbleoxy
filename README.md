# Bubbleoxy

Güneş enerjili, yüzer oksijenlendirme prototipi için donanımdan bağımsız C++11 kontrol algoritması, masaüstü simülasyonu ve testler.

Bu depo bir **prototip yazılım başlangıcıdır**. Gerçek sensör sürücüleri, kart pinleri, güç elektroniği ve uzaktan izleme henüz uygulanmamıştır. Varsayılan oksijen, batarya ve süre değerleri yalnızca simülasyon örneğidir; saha veya canlı ortam için önerilen değerler değildir. Nano kabarcık oluşumu ve oksijen transfer performansı ayrı fiziksel ölçümlerle doğrulanmalıdır.

## Yerelde çalıştırma

Gereken araçlar: C++11 derleyicisi, CMake 3.16 veya üzeri, bir derleme aracı (ör. Make).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/simulator
```

Simülatör CSV biçiminde zaman, çözünmüş oksijen, batarya, akış, durum, hata ve çıkış komutlarını yazdırır. İstersen `./build/simulator > simulation.csv` ile kaydet. Bu bir su kimyası veya batarya fizik modeli değildir; önceden tanımlı girdilerle kontrol geçişlerini gösterir.

## Dosyalar

- `src/bubbleoxy.h`: platform bağımsız kontrol çekirdeği.
- `examples/simulator.cpp`: sensör yerine örnek değerler kullanan senaryo.
- `tests/controller_tests.cpp`: geçişler, kilitli hatalar, histerezis, zaman taşması ve veri geçerliliği testleri.
- `docs/ALGORITHM.md`: karar sırası, varsayımlar ve sınırlar.
- `docs/HARDWARE.md`: prototip yapılırken tamamlanacak donanım arayüzleri.
- `.github/workflows/ci.yml`: her push ve pull request için derleme ve test.

## Algoritmanın özeti

1. Yapılandırmayı, acil durum girişini ve sensör verisinin geçerliliğini kontrol et.
2. Arıza varsa pompa ve kompresörü kapat, arızayı operatör onayına kadar kilitle.
3. Batarya düşükse çıkışları kapat; ayrı bir toparlanma eşiğini bekle.
4. Oksijen alt eşikte veya altındaysa ve bekleme süresi dolduysa pompayı çalıştır.
5. Akış belirli süre doğrulanırsa kompresörü devreye al; zamanında akış oluşmazsa arızaya geç.
6. Çalışırken akış kaybolursa hemen dur. Oksijen üst eşiğe ulaşır ve minimum çalışma süresi dolarsa normal duruş yap.

Bağlantı olmasa da kontrol yerel çalışacak şekilde tasarlanmıştır. İnternet/telemetri ileride ayrı bir katman olarak eklenmelidir.

## GitHub'a ilk yükleme

Bu komutları gerçek Git deposu olan çalışma klasöründe çalıştır:

```bash
gh auth login --hostname github.com --git-protocol https --web
git init -b main
git add .
git diff --cached --stat
git commit -m "Add Bubbleoxy controller, simulation and tests"
gh repo create bubbleoxy --public --source=. --remote=origin --push
```

Git kimliğin ayarlı değilse commit öncesinde kendi bilgilerinle `git config user.name` ve `git config user.email` ayarla. E-posta için GitHub hesabındaki noreply adresini kullanabilirsin. Depo adı zaten kullanılıyorsa `bubbleoxy-controller` gibi boş bir isim seç.

Tarifname veya başvuru belgeleri bu depoya dahil edilmemiştir. Lisans seçimi proje sahibine bırakılmıştır; public görünürlük tek başına açık kaynak kullanım lisansı vermez.
