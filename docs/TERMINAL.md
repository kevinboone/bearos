## Terminal set-up for BearOS

BearOS is designed to use a dumb terminal as its user interface. Of course, in
practice, you can use a terminal emulator, but BearOS sets out to be compatible
with a VT100 in its default setup.

The terminal should have the following characteristics.

Size: at least 24 rows and _exactly_ 80 columns.

Line wrapping: text that overflows the line should wrap to the next line.  A
backspace at the start of a line should 'underflow' back to the previous ine.

Backspace: The terminal display should treat character 8 as a _non-destrutive_
backspace. BearOS does not normally send a desructive backspace character.  The
terminal should send character 8 when backspace is pressed, and character 127
when delete is pressed. 

Interrupt: The terminal should send character 3 when ctrl+C is pressed, or some
other suitable interrupt key.

Enter: The terminal should send CR (13) when enter is pressed.

These settings are the defaults for the Minicom terminal, except for line
wrapping. To enable line wrapping, hit ctrl+A followed by 'w'

When running CP/M programs under emulation on BearOS, it will probably be
necessary to set the terminal to emulate a VT52, unless the CP/M program allows
terminal selection (as WordStar does). There isn't really anything BearOS can
do to fix the fact that CP/M stems from a time before terminals were
standardized. 


