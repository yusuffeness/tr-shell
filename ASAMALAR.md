# tr-shell Aşamalar

Her aşama tek bir commit ile kapanır ve bir sürüm etiketi alır.
İşaretli adımlar tamamlanmış, ⏳ olan aşama üzerinde çalışılıyor demektir.

| Aşama | Konu | Sürüm | Durum |
| --- | --- | --- | --- |
| 1 | Altyapı | v0.1 | ✅ |
| 2 | Türkçe komutlar | v0.2 | ⏳ |
| 3 | Görünüm | v0.3 | |
| 4 | Kullanım rahatlığı | v0.4 | |
| 5 | Kendi komutlar | v0.5 | |
| 6 | Shell özellikleri | v0.6 | |
| 7 | Takma ad ve ayarlar | v0.7 | |
| 8 | Betik (opsiyonel) | v0.8 | |
| 9 | Yayın | v1.0 | |

---

## Aşama 1: Altyapı → v0.1 ✅

- [x] Repo ve ilk commit
- [x] Kodu `src/` altında modüllere bölme (ui, input, parser, builtins, executor)
- [x] Makefile (`make`, `run`, `test`, `debug`, `install`, `clean`)
- [x] Test altyapısı (`tests/run.sh`, 11 davranış testi) ve etkileşimsiz mod
- [x] README, LICENSE, ASAMALAR.md

## Aşama 2: Türkçe komutlar → v0.2

- [ ] Komut tablosu ve eşleme komutları: `listele`, `oku`, `kopyala`, `tasi`, `klasor`, `olustur`, `bul`, `ara`, `boyut`, `surecler`, `oldur`, `disk`, `bellek`, `kimim`
- [ ] Yerleşik komutlar: `gec`, `geri`, `ev`, `nerdeyim`, `cikis`
- [ ] `yardim`, `hakkinda`, `tarih`
- [ ] Yeni komutların testleri

## Aşama 3: Görünüm → v0.3

- [ ] Alternatif ekran (önceki terminal çıktısı gizlenir) ve açılış kutusu
- [ ] İki satırlı istem, ✓ / ✗ durum göstergesi, uzun komutlarda süre
- [ ] İstemde git dalı
- [ ] Temalar (`tema`) ve ekran genişliğine uyum

## Aşama 4: Kullanım rahatlığı → v0.4

- [ ] readline ile satır düzenleme ve kalıcı geçmiş (`gecmis`, `!N`)
- [ ] Tab tamamlama
- [ ] Ctrl+C shell'i kapatmasın
- [ ] "Bunu mu demek istedin?" önerisi ve `zamanla`

## Aşama 5: Kendi komutlar → v0.5

- [ ] `listele` (renkli, okunur boyutlar)
- [ ] `agac`
- [ ] `sistem` (CPU, RAM, açık kalma süresi)
- [ ] `sil` (onaylı, çöpe taşır) ve `copkutusu`
- [ ] `hesapla` ve `not` / `notlar`

## Aşama 6: Shell özellikleri → v0.6

- [ ] Yeni sözcükleyici: tırnak ve kaçış karakteri
- [ ] Ayrıştırıcı (sözdizimi ağacı)
- [ ] Yönlendirme: `>`, `>>`, `<`, `2>`
- [ ] Pipe: `|`
- [ ] `&&`, `||`, `;`
- [ ] Değişkenler: `$HOME`, `$?`, `~`, `degisken`
- [ ] Arka plan işleri: `&`, `isler`, `on`, `arka`

## Aşama 7: Takma ad ve ayarlar → v0.7

- [ ] `takmaad` ve `takmaadsil`
- [ ] Ayar dosyası (`~/.trsh/ayar`)

## Aşama 8: Betik (opsiyonel) → v0.8

- [ ] Betik modu (`trsh betik.trsh`) ve `kaynak`
- [ ] `eger` ve `dongu`
- [ ] `$(...)` komut yerine koyma

## Aşama 9: Yayın → v1.0

- [ ] Test paketini tamamlama ve GitHub Actions
- [ ] Son README (ekran görüntüleriyle) ve v1.0
