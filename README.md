Converts the first sheet of an .ods file to .sc and spits it out on stdout.  

Pros:
- The only one of its kind if my google skills aren't too bad
- Fast asf
- Code is basically public domain due to wtfpl
Cons:
- Converts only the first sheet of a file
- buggy, probably. If you wanna know the current state of the conversion algorithm, look at the comments of the "cell\_to\_sc" function at the end of src/xml\_to\_sc.c

Originally I wanted to integrate this into [andmarti1424's sc-im](https://github.com/andmarti1424/sc-im), but idk if I'll ever finish doing that, so I'm just putting it out there I guess.
