#!/usr/bin/env python3
"""Compare imported official Vulkan Profiles requirements with a capability capture."""
import argparse, json, pathlib

def flatten(value,prefix=""):
    out={}
    if isinstance(value,dict):
        for k,v in value.items(): out.update(flatten(v,f"{prefix}.{k}" if prefix else k))
    elif isinstance(value,list):
        for i,v in enumerate(value): out.update(flatten(v,f"{prefix}[{i}]"))
    else: out[prefix]=value
    return out

def actual_for(path,caps):
    parts=path.split("."); key=parts[-1]; features=caps.get("features",{}); props=caps.get("properties",{}); exts={x["name"]:x.get("specVersion",0) for x in caps.get("extensions",[])}
    if len(parts)>=3 and parts[-2] in caps.get("featureStructures",{}):return caps["featureStructures"][parts[-2]].get(key)
    if len(parts)>=3 and parts[-2] in caps.get("propertyStructures",{}):return caps["propertyStructures"][parts[-2]].get(key)
    if len(parts)>=3 and parts[-2] in ("VkPhysicalDeviceProperties","VkPhysicalDeviceLimits","VkPhysicalDeviceSparseProperties"):
        source=caps.get("coreProperties",{}); source=source.get("limits",{}) if parts[-2]=="VkPhysicalDeviceLimits" else source.get("sparseProperties",{}) if parts[-2]=="VkPhysicalDeviceSparseProperties" else source
        value=source.get(key,props.get(key))
        if isinstance(value,str):
            try:return int.from_bytes(bytes.fromhex(value),"little")
            except ValueError:pass
        return value
    if key in features:return features[key]
    if key in props:return props[key]
    if key in exts:return exts[key]
    return None

def satisfies(actual,required):
    if actual is None:return False
    if isinstance(required,bool):return actual is required
    if isinstance(required,(int,float)):return actual>=required
    return actual==required

def item_satisfies(item):
    return item["satisfied"] if "satisfied" in item else satisfies(item["actual"],item["required"])

def api_satisfies(actual, required):
    actual_version=((actual >> 22) & 0x7f, (actual >> 12) & 0x3ff, actual & 0xfff)
    required_version=tuple(int(x) for x in str(required).split("."))
    return actual_version >= required_version

def evaluate(caps, imported):
    doc=imported["document"]; results={}
    for pname,profile in doc["profiles"].items():
        api_required=profile.get("api-version"); api_actual=caps.get("device",{}).get("apiVersion",0)
        required=[{"capability":"profile","path":"api-version","required":api_required,"actual":api_actual,"satisfied":api_satisfies(api_actual,api_required)}]; optional=[]; feature_levels={}
        for cname in profile.get("capabilities",[]):
            target=optional if "optional" in cname else required
            capability_requirements=[]
            for path,want in flatten(doc["capabilities"][cname]).items():
                item={"capability":cname,"path":path,"required":want,"actual":actual_for(path,caps)}; target.append(item); capability_requirements.append(item)
            if cname.startswith("fl_") or "level" in cname: feature_levels[cname]="PASS" if all(satisfies(x["actual"],x["required"]) for x in capability_requirements) else "FAIL"
        passed=[x for x in required if item_satisfies(x)]; failed=[x for x in required if not item_satisfies(x)]
        optional_result=[dict(x,status="PASS" if satisfies(x["actual"],x["required"]) else "FAIL") for x in optional]
        results[pname]={"status":"PASS" if not failed else "FAIL","pass":passed,"fail":failed,"optional":optional_result,"featureLevel":{"label":profile.get("label",pname),"capabilities":feature_levels}}
    return {"schemaVersion":1,"profileSource":imported["provenance"],"profiles":results}

def main():
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument("--capabilities",required=True); ap.add_argument("--profile",required=True); ap.add_argument("--output"); a=ap.parse_args()
    profile_path=pathlib.Path(a.profile); imported=json.loads(profile_path.read_text()); result=evaluate(json.loads(pathlib.Path(a.capabilities).read_text()),imported); text=json.dumps(result,indent=2)+"\n"
    if a.output:
        output=pathlib.Path(a.output)
        if output.resolve()==profile_path.resolve(): imported["evaluation"]=result; text=json.dumps(imported,indent=2)+"\n"
        output.write_text(text)
    else:print(text,end="")
if __name__=="__main__":main()
