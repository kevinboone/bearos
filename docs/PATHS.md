+++ Paths in BearOS

BearOS uses drive letters, like CP/M and MSDOS, to indicate devices. 
Unlike MSDOS, however, BearOS does not have a notion of per-drive
working directories. This means that X:foo and X:/foo refer to the
same file. 

In general, the symbols ".." and "." have the same meaning in BearOS
that they do in Linux. So "cd .." changes directory upwards. 

"cd X:/foo" changes the working directory to "X:/foo".
This is different to MSDOS, where this command would just change the working
directory of drive X, not necessarily the current directory.

At present, since only one "real" drive is supported -- the
SD card -- these considerations are unimportant; paths can be
considered the same in BearOS as they are on Linux/Unix. Drive
letters need not be used at all, as the SD card is the default drive.

Of course, if you want to use a different drive, you'll have to
specify the drive letter.


