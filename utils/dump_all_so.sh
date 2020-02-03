#!/bin/bash
for filename in *.so; do
  echo "$filename"
  ../bin/dumper "$filename" > "$filename.json"
done

