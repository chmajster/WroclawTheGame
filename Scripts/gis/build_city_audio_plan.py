#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_audio_profiles.json";CITY=ROOT/"Saved/CityData/city.json"

def count_any(counts,*keys):return sum(float(counts.get(k,0) or 0) for k in keys)

def detect_layers(cfg,sector):
 c=sector.get("counts",{});layers=[]
 def add(layer,activity=1.0):
  spec=cfg["layers"][layer];layers.append({"id":layer,"base_gain":spec["base_gain"],"activity":round(max(0.0,min(1.0,activity)),3),"priority":spec.get("priority",50),"emitter_role":spec.get("emitter_role",layer)})
 roads=count_any(c,"roads","road_edges")
 rails=count_any(c,"rail_lines","tram_lines","rails")
 green=count_any(c,"green","green_areas","parks")
 water=count_any(c,"water","water_areas","river_lines")
 industrial=count_any(c,"industrial","industrial_areas")
 if roads>0:add("traffic",min(1,0.25+roads/20))
 if rails>0:add("tram",min(1,0.4+rails/8))
 if green>0:add("park",min(1,0.35+green/8))
 if water>0:add("river",min(1,0.4+water/6))
 if industrial>0:add("industrial",min(1,0.35+industrial/8))
 if sector.get("density") in {"High","VeryHigh"}:add("crowd",1 if sector.get("density")=="VeryHigh" else .75)
 return layers

def mix_layer(layer,cfg,day_night=None,weather=None):
 day_night=day_night or {};weather=weather or {}
 phase=day_night.get("phase","day");phase_gain=cfg.get("day_phase_gain",{}).get(phase,1.0)
 weather_id=weather.get("weather","Clear");weather_gain=cfg["weather_gain"].get(weather_id,1.0)
 category_weather=cfg.get("weather_layer_gain",{}).get(weather_id,{}).get(layer["id"],1.0)
 gain=float(layer["base_gain"])*float(layer["activity"])*phase_gain*weather_gain*category_weather
 if layer["id"]=="crowd":gain*=float(day_night.get("crowd_multiplier",1.0))
 if layer["id"]=="traffic":gain*=float(day_night.get("traffic_multiplier",1.0))
 if layer["id"]=="night":gain*=float(day_night.get("night_alpha",1.0))
 return {**layer,"gain":round(max(0.0,min(1.5,gain)),4)}

def acoustic_profile(cfg,sector):
 density=sector.get("density","Medium")
 return {
  "reverb_profile":cfg["reverb_by_density"].get(density,cfg["reverb_by_density"]["default"]),
  "occlusion":cfg["occlusion_by_density"].get(density,cfg["occlusion_by_density"]["default"]),
  "voice_budget":int(cfg["voice_budget_by_density"].get(density,cfg["voice_budget_by_density"]["default"]))
 }

def generate(cfg,city,day_night=None,weather=None):
 out=[]
 for s in city.get("sectors",[]):
  layers=detect_layers(cfg,s)
  if (day_night or {}).get("night_alpha",0)>0.35:
   spec=cfg["layers"]["night"];layers.append({"id":"night","base_gain":spec["base_gain"],"activity":float((day_night or {}).get("night_alpha",1)),"priority":spec.get("priority",45),"emitter_role":spec.get("emitter_role","night")})
  mixed=sorted((mix_layer(x,cfg,day_night,weather) for x in layers),key=lambda x:(-x["priority"],x["id"]))
  out.append({"sector":s["id"],"district":s.get("district"),"layers":mixed,**acoustic_profile(cfg,s)})
 return {"schema_version":2,"sectors":out,"weather":(weather or {}).get("weather","Clear"),"phase":(day_night or {}).get("phase","day"),"global_voice_budget":cfg["global_voice_budget"],"streaming_prefetch_seconds":cfg["streaming_prefetch_seconds"]}

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--day-night-state",type=Path);p.add_argument("--weather-state",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 load=lambda x:json.loads(x.read_text()) if x else None;r=generate(load(a.config),load(a.city),load(a.day_night_state),load(a.weather_state));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["sectors"]))
if __name__=="__main__":main()
