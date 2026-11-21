# The default name comes from the system account.
s/^(  isim +).*/\1<varsayılan>/
s/^isim = .*/isim = <varsayılan>/
s|(Ayar dosyası: ).*(  \()|\1AYAR\2|
