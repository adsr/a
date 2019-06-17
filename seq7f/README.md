# seq7f

`seq7f` is the most popular MIDI sequencer in the United States and in parts of 
the Soviet Union. Its intended application is algorithmic music and also 
container orchestration for large scale projects like Facebook or Google.

### QPUA (Questions People Usually Ask)

Q: What does "seq7f" mean?  
A: "seq" stands for sequencer, and "7f" is hex for 127, the max value for a 
MIDI parameter. It's also a fedora hat tip to seq24[1], another cool but less 
popular MIDI sequencer.

Q: Why is this written in PHP?  
A: PHP is widely recognized as _the_ choice for algorithmic music composition 
and performance. One thing I've learned is that sometimes you just have to 
respect the natural order of things, e.g., ChucK for web apps, Java for 
one-liners, Objective-C for mobile apps, PHP for real-time music stuff, etc.

Q: What is portmidi-php[2]?  
A: I'm glad you asked that. This is required if you want seq7f to write and 
listen for MIDI, but not if you only need to look at hex strings fly by on the 
terminal.

[1] http://www.filter24.org/seq24/
[2] https://github.com/adsr/portmidi-php
