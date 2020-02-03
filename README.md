# Description

Dump the DWARF debug information into a JSON file. It's designed to support 
large files.


# Usage

```sh
$ dwarf_dumper/bin/dumper library.so > output.json
```

Example:

```sh
$ du -sh libchromium.so 
  4,5G	libchromium.so

$ /usr/bin/time -v dwarf_dumper/bin/dumper libchromium.so > dwarf.json 
	Command being timed: "dwarf_dumper/bin/dumper libchromium.so"
	User time (seconds): 16.36
	System time (seconds): 3.72
	Percent of CPU this job got: 95%
	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:21.11
	Maximum resident set size (kbytes): 8446220

$ du -sh dwarf.json 
  1,4G	dwarf.json
```

# Dealing with the output

The JSON generated can be big sometimes. For the basics things you can use a 
simple parser like the one in `utils/find_object_size.py`. However if you plan
to use it on huge projects like Chromium I recommend you to parse everything 
with a performance oriented parser like 
[RapidJSON](https://github.com/Tencent/rapidjson) to get an acceptable speed / 
memory usage.


# Code 

We are following the 
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).


# Chromium

In the chromium build, we need to enable all the symbols. Add the following
arguments in your `args.gn`:

```
symbol_level=2
ignore_elf32_limitations=true
```

