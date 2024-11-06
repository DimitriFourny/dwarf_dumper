#!/usr/bin/env python
# pip install ujson
import ujson
import glob
import os 
import sys

def get_type_name(data, type_id):
  type_name =  "#" + str(type_id)
  if type_id not in data:
    return type_name

  if "name" in data[type_id]:
    return data[type_id]["name"]

  if ("type" in data[type_id]) and (data[type_id]["type"] == "pointer_type"):
    pointer_type_id = data[type_id]["type_id"]
    if pointer_type_id in data:
      if "name" in data[pointer_type_id]:
        return data[pointer_type_id]["name"] + "*"

  return type_name

def get_members(data, type_id):
  if type_id not in data:
    return None

  if "members" in data[type_id]:
    return data[type_id]["members"]

  return None

def dump_members(data, members, base_offset=0, deep_size=0):
  if deep_size >= 4: # no cyclic references
    return

  for member in members:
    member["type_name"] = "unk"
    submembers = None

    if "type_id" in member:
      member["type_name"] = get_type_name(data, member["type_id"])
      submembers = get_members(data, member["type_id"])

    member["offset"] += base_offset
    member["spaces"] = " "*4*deep_size
    print("    +0x{offset:<4x}{spaces} {type_name} {name}".format(**member))

    if submembers:
      dump_members(data, submembers, member["offset"], deep_size+1)


if len(sys.argv) < 3:
  print("Usage: %s <json_path> <class_size>")
  sys.exit(1)

filepath = sys.argv[1]
class_size = int(sys.argv[2], 0)

print("Reading the file %s" % filepath)
buffer = ""
with open(filepath, "r") as f:
  buffer = f.read()

print("Parsing JSON")
try:
  data = ujson.loads(buffer)
except Exception, e: 
  print("Error when parsing '%s': %s" % (filepath, e))
  sys.exit(2)
  
print("Searching the size 0x%x" % class_size)
for class_data in data.values():
  if "type" not in class_data:
    continue 
  if class_data["type"] not in ["class", "struct"]:
    continue

  if "size" not in class_data:
    continue
  if class_data["size"] != class_size:  
    continue

  if "name" not in class_data:
    continue

  print("\n%s" % class_data["name"])
  if "members" in class_data:         
    dump_members(data, class_data["members"])
