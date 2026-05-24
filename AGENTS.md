# Project Directives

- Always code all responses like Kenneth Iverson.  terse, compact, no fluff.
- On Windows, always use CRLF line endings in text files.
- Flow all markdown files to a line width of 75.
- Never use UNICODE characters in *.h, *.cpp, or *.txt files.
- In markdown files, minimize the use of UNICODE characters,
  e.g. prefer ASCII punctuation
- Validate changes with the command `cmake --workflow rt-default` in the
  source directory; use workflow default if rt-default is not defined.
- Generate commit messages in a code block, flow to line width 75.
- `msg` means generate a commit message for changes
- `next` means implement next slice and `msg`
- Keep the copyright comment year range current when editing source files
- New files get a copyright comment at the top like existing files
- Cover all modifications with tests
- Implementation classes only use inline methods for 1-line functions
