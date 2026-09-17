// Generated from Data/city_gameplay.json; do not edit.
#pragma once
#include <string>
#include <vector>
namespace Wroclaw {
struct CityActionDef { std::string id,sector,title,body,kind,node,building; std::vector<std::string> prerequisites; double seconds; bool vehicle,crouch; };
inline const std::vector<CityActionDef>& CityActions(){static const std::vector<CityActionDef> data={
{"city.huby.clue1","sector.huby","Tabliczka przy bramie","Tabliczka przy bramie. Trop został zapisany w telefonie.","clue","2262621166","way/100942938:0",{},0,false,false},
{"city.huby.clue2","sector.huby","Skrzynka pocztowa","Skrzynka pocztowa. Trop został zapisany w telefonie.","clue","3075909778","way/100937260:0",{},0,false,false},
{"city.huby.clue3","sector.huby","Ślad przy opuszczonym lokalu","Ślad przy opuszczonym lokalu. Trop został zapisany w telefonie.","clue","2262495191","way/100943599:0",{},0,false,false},
{"city.huby.finish","sector.huby","Znikający lokator","Trzy ślady wskazują na magazyn poza centrum.","finish","6567420740","way/100930571:0",{"city.huby.clue1","city.huby.clue2","city.huby.clue3"},0,false,false},
{"city.huby.secret","sector.huby","Ukryta wiadomość — huby","Ukryta wiadomość — huby. Trop został zapisany w telefonie.","secret","3865087499","way/100920547:0",{},0,false,false},
{"city.huby.event1","sector.huby","Zgubiona torba","Zgubiona torba. Trop został zapisany w telefonie.","event","2262621164","way/100932990:0",{},0,false,false},
{"city.huby.event2","sector.huby","Sygnał ostrzegawczy","Sygnał ostrzegawczy. Trop został zapisany w telefonie.","event","4700650431","way/100922330:0",{},0,false,false},
{"city.huby.event3","sector.huby","Nieoczekiwana wiadomość","Nieoczekiwana wiadomość. Trop został zapisany w telefonie.","event","2262445188","way/100933986:0",{},0,false,false},
{"city.gaj.clue1","sector.gaj","Punkt obserwacyjny","Punkt obserwacyjny. Trop został zapisany w telefonie.","clue","2897661056","way/100834524:0",{},20,true,false},
{"city.gaj.clue2","sector.gaj","Notatka parkingowego","Notatka parkingowego. Trop został zapisany w telefonie.","clue","2897845751","way/100834524:0",{},0,false,false},
{"city.gaj.clue3","sector.gaj","Numer dostawy","Numer dostawy. Trop został zapisany w telefonie.","clue","12996407578","way/100834524:0",{},0,false,false},
{"city.gaj.finish","sector.gaj","Samochód bez kierowcy","Dostawa łączy osiedle z firmą na zachodzie.","finish","2897661063","way/100834524:0",{"city.gaj.clue1","city.gaj.clue2","city.gaj.clue3"},0,false,false},
{"city.gaj.secret","sector.gaj","Ukryta wiadomość — gaj","Ukryta wiadomość — gaj. Trop został zapisany w telefonie.","secret","6230933123","way/100830495:0",{},0,false,false},
{"city.gaj.event1","sector.gaj","Zgubiona torba","Zgubiona torba. Trop został zapisany w telefonie.","event","7934346545","way/100836261:0",{},0,false,false},
{"city.gaj.event2","sector.gaj","Sygnał ostrzegawczy","Sygnał ostrzegawczy. Trop został zapisany w telefonie.","event","469129007","way/100843378:0",{},0,false,false},
{"city.gaj.event3","sector.gaj","Nieoczekiwana wiadomość","Nieoczekiwana wiadomość. Trop został zapisany w telefonie.","event","8210836935","way/100834524:0",{},0,false,false},
{"city.borek.clue1","sector.borek","Ślad przy ogrodzeniu","Ślad przy ogrodzeniu. Trop został zapisany w telefonie.","clue","2245953664","way/100941611:0",{},0,false,false},
{"city.borek.clue2","sector.borek","List w skrytce","List w skrytce. Trop został zapisany w telefonie.","clue","2246920254","way/100920581:0",{},0,false,true},
{"city.borek.clue3","sector.borek","Kontakt mieszkańca","Kontakt mieszkańca. Trop został zapisany w telefonie.","clue","2245950554","way/100920581:0",{},0,false,false},
{"city.borek.finish","sector.borek","Cichy dom","Dom może służyć jako bezpieczny punkt spotkań.","finish","2246983940","way/100922877:0",{"city.borek.clue1","city.borek.clue2","city.borek.clue3"},0,false,false},
{"city.borek.secret","sector.borek","Ukryta wiadomość — borek","Ukryta wiadomość — borek. Trop został zapisany w telefonie.","secret","5551944424","way/100925704:0",{},0,false,false},
{"city.borek.event1","sector.borek","Zgubiona torba","Zgubiona torba. Trop został zapisany w telefonie.","event","2246843827","way/100914853:0",{},0,false,false},
{"city.borek.event2","sector.borek","Sygnał ostrzegawczy","Sygnał ostrzegawczy. Trop został zapisany w telefonie.","event","3748110588","way/100929023:0",{},0,false,false},
{"city.borek.event3","sector.borek","Nieoczekiwana wiadomość","Nieoczekiwana wiadomość. Trop został zapisany w telefonie.","event","3693691011","way/100932159:0",{},0,false,false},
{"city.krzyki.clue1","sector.krzyki","Kwit garażowy","Kwit garażowy. Trop został zapisany w telefonie.","clue","304074228","way/100834345:0",{},0,false,false},
{"city.krzyki.clue2","sector.krzyki","Oznaczenie skrzyni","Oznaczenie skrzyni. Trop został zapisany w telefonie.","clue","4789864137","way/657079949:0",{},0,false,false},
{"city.krzyki.clue3","sector.krzyki","Zapis trasy","Zapis trasy. Trop został zapisany w telefonie.","clue","304074234","way/100834345:0",{},0,false,false},
{"city.krzyki.finish","sector.krzyki","Ślad po dostawie","Zebrane dokumenty ujawniają kolejny transport.","finish","4789864156","way/100823733:0",{"city.krzyki.clue1","city.krzyki.clue2","city.krzyki.clue3"},0,false,false},
{"city.krzyki.secret","sector.krzyki","Ukryta wiadomość — krzyki","Ukryta wiadomość — krzyki. Trop został zapisany w telefonie.","secret","304074221","way/100823404:0",{},0,false,false},
{"city.krzyki.event1","sector.krzyki","Zgubiona torba","Zgubiona torba. Trop został zapisany w telefonie.","event","305758781","way/100829610:0",{},0,false,false},
{"city.krzyki.event2","sector.krzyki","Sygnał ostrzegawczy","Sygnał ostrzegawczy. Trop został zapisany w telefonie.","event","367050057","way/100834345:0",{},0,false,false},
{"city.krzyki.event3","sector.krzyki","Nieoczekiwana wiadomość","Nieoczekiwana wiadomość. Trop został zapisany w telefonie.","event","302375805","way/677519726:0",{},0,false,false},
};return data;}
}
