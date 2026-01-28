# Parsing iTunes Library database using C

My wife has an iPod Gen4 where certain playlists live. And these playlists
live nowhere else. The point of this program is to load an 'iTunes Library' file
assumed to be in the `.itl` format (but it has no extension) and get the
playlist information out to save me the gruntwork of clicking through the Gen4
clickwheel myself to figure out which songs are on the playlist. Mainly her
Christmas Playlist.

The idea for this library leans heavily on the work of this 
[repo](https://github.com/josephw/titl)
and this 
[fork](https://github.com/cbfiddle/titl).

The description of the file used as a starting point is
[here](https://metacpan.org/release/BDFOY/Mac-iTunes-1.22/view/doc/file_format.pod).

## Requirements

The final binary is dynamically linked to the `-lcrypto` and `-lz` libs so you
must have OpenSSL and ZLib installed.

## To Debug

Change the Makefile to compile with debugging information.  
```Makefile
# To add debug flag
obj/%.o : src/%.c	
	gcc -c $< -o $@ -Iinclude -g
```

Check for memory leaks with valgrind.  
```bash
# On file open
valgrind --leak-check=full ./bin/itlview -f ./iTunes_Library.itl
```

Step through the program using gdb (with args).  
```bash
# Open a db and list the playlists
gdb --args ./bin/itlview -f ./iTunes_Library.itl -p
```

Once in GDB, switch to the TUI with `tui enable`.  
Cycle thgrough TUI layouts using `layout next`.  
Set breakpoints with `break main`.  
Run the program with `run`.  
Go to next line in src with `next`.  
Step into a line in src with `step`.  
See local vars with `info locals`.  
GDB cheat sheets [here](https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf) 
and [here](https://cs.brown.edu/courses/cs033/docs/guides/gdb.pdf).
