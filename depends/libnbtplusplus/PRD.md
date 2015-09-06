# libnbt++2 Product Requirements Document

### Purpose
Provide a C++ interface for working with NBT data, particularly originating
from Minecraft.

### Scope
External Minecraft utilities that read or manipulate parts of savefiles,
such as:
- (Graphical) NBT editors
- Inventory editors
- Tools for reading or changing world metadata
- Map editors and visualizers

### Definitions, Acronyms and Abbreviations
- **libnbt++1:** The predecessor of libnbt++2.
- **Minecraft:** A sandbox voxel world game written in Java, developed by
  Mojang.
- **Mojang's implementation:** The NBT library written in Java that Mojang
  uses in Minecraft.
- **NBT:** Named Binary Tag. A binary serialization format used by Minecraft.
- **Tag:** A data unit in NBT. Can be a number, string, array, list or
  compound.

### Product Functions
- /RF10/ Reading and writing NBT data files/streams with or without
  compression.
- /RF20/ Representing NBT data in memory and allowing programs to read or
  manipulate it in all the ways that Mojang's implementation and libnbt++1
  provide.
- /RF30/ A shorter syntax than in libnbt++1 and preferrably also Mojang's
  implementation.
- /RF35/ Typesafe operations (no possibly unwanted implicit casts), in case
  of incompatible types exceptions should be thrown.
- /RF40/ The need for insecure operations and manual memory management should
  be minimized; references and `std::unique_ptr` should be preferred before
  raw pointers.
- /RF55/ A wrapper for tags that provides syntactic sugar is preferred
  before raw `std::unique_ptr` values.
- /RF50/ Move semantics are preferred before copy semantics.
- /RF55/ Copying tags should be possible, but only in an explicit manner.
- /RF60/ Checked conversions are preferred, unchecked conversions may be
  possible but discouraged.

### Product Performance
- /RP10/ All operations on (not too large) NBT data should not be slower
  than their counterparts in Mojang's implementation.
- /RP20/ The library must be able to handle all possible NBT data that
  Mojang's implementation can create and handle.
- /RP30/ Often used operations on large Lists, Compounds and Arrays must
  be of at most O(log n) time complexity if reasonable. Other operations
  should be at most O(n).

### Quality Requirements
- Functionality: good
- Reliability: normal
- Usability: very good
- Efficiency: good
- Changeability: normal
- Transferability: normal
