# ToDo List

- look for unused members
- SourceLocation is heavy; move filename somewhere else. Source ranges never
  span files, so track file index on SourceRange and keep SourceLocation as
  line/column only. A compact shape such as 16-bit line, 16-bit column, and a
  file index into the referenced-file table should cover real parameter set
  and formula import graphs.
  ```cpp
  struct SourceLocation
  {
      uint16_t line{1};
      uint16_t column{1};
  };

  struct SourceRange
  {
      uint16_t file_index{};
      SourceLocation begin;
      SourceLocation end;
  };
  ```
- move SourceRange to SourceLocation.h
- review code for long method smell; Compose Method to disinfect
- review parsing code to minimize the amount of string copying
