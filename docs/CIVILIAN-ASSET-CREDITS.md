# Źródła cywilnego bohatera

| Zasób | Autor i źródło | Licencja | Zastosowanie |
| --- | --- | --- | --- |
| Anatomia, UV, oczy, targety i skóra | MakeHuman Team; rewizja i poszczególne pliki w Data/hero_source_catalog.json | CC0-1.0 dla danych graficznych | Ciało, morphy i tekstury; źródła wersjonowane |
| male_casualsuit01, shoes01, short02, eyebrow001 | makehuman_system, oficjalna paczka systemowa MakeHuman | CC0-1.0 potwierdzone osobno na stronie paczki | Dopasowane ubranie, buty, włosy i brwi |
| Rig i źródłowe klipy ruchu | Quaternius, Universal Base Characters / Universal Animation Library 1 | CC0-1.0; istniejący rejestr docs/FREE_CHARACTER_ANIMATION_ASSETS.md | Wspólny rig i osobne pochodne klipów |
| Plecak i generator pochodnych | WroclawTheGame contributors | CC0-1.0 dla nowej geometrii dodatku | Panele, kieszeń, uchwyt, szwy i skinned paski |

Oficjalne źródła: [system assets](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html),
[licencja MakeHuman](https://static.makehumancommunity.org/about/license.html).
CC0 danych graficznych dopuszcza zarówno użycie w grze, jak i przechowywanie
źródeł w publicznym repozytorium. Kod aplikacji MakeHuman ma osobną licencję;
importer czyta jedynie dane graficzne, bez kopiowania kodu aplikacji.

Dokładne URL, autor, licencja, SHA256, rozmiar i decyzje o obu sposobach użycia
są zapisane dla 84 plików w Data/hero_source_catalog.json. Import odrzuca
zmieniony plik źródłowy i nieaktualny eksport FBX. `.gitattributes` wyłącza
konwersję końców linii danych MakeHuman, by checkout na Windows zachowywał hash.

Odtwarzanie: Blender uruchamia Pipeline/blender/build_asset.py z manifestem
Pipeline/assets/player/civilian.asset.json. Natywny etap to
Pipeline/unreal/import_asset.py z WTG_ASSET_MANIFEST wskazującym ten manifest.
Produkty runtime są rzeczywistymi zasobami UE w /Game/Wroclaw/Hero i są
wersjonowane; .blend oraz generator umożliwiają ich ponowne zbudowanie.
