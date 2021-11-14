# minivim

## info

A "mini" implementation of vim :3

It uses `VT100` escape characters (I will implement `ncurses` in the future probably).

P.S. I know they are way too much comments, sorry for that. I do this project for learning purpouses, so I comment everything I do for that reason ":D

## How to use

Clone the repo

```sh
git clone https://github.com/izenynn/minivim.git
```

Run make inside the repo

```sh
cd ./minivim && make
```

Open a file using minivim (it will create the file if it does not exists)

```sh
./minivim [FILE]
```

- or create a file with no name, and name it later with vim command `:saveas`

```sh
./minivim
```

*NOTE: if cursor highlighting is not working, that is probably becouse your terminal is reversing the cursor position color too, so it goes back to normal, to fix this, compile again with the variable CURSOR_HL=0 (disabled).*

```sh
make re CURSOR_HL=0
```

## How to install

If you want to add minivim to your path and be able to use it in any directory like any other command, run `make install`

```sh
make install
```

In case it gives you permissions error, try running it with `sudo`

```sh
sudo make install
```

*NOTE: if you are having the issue I described before with the cursor highlighting, you will need to also install with CURSOR_HL=0.*

```sh
sudo make install CURSOR_HL=0
```

*NOTE: to change the directory in which the binary is installed, you can compile with BIN_DIR="/usr/local" (just an example).*

```sh
sudo make install BIN_DIR="/usr/local/bin"
```

## Features

Editor features:
- Open, edit and save any text files.
- C and C++ syntax highlighting.

Vim features:
- `normal` and `insert` mode.
- `i`, `a`: change to insert mode.
- `o`, `O`: insert new line.
- `h`, `j`, `k`, `l`: move around (also: arrows).
- `0`: move to first character in the line (also: home key).
- `^`: move to first non-blank character in the line.
- `$`: move to last character in the line (also: end key).
- `gg`: goto first line.
- `G`: goto last line.
- `:w`, `:q`, `:q!`, `:wq`: supported commands.
- `:saveas [NAME]`: supported command.
- `/[MATCH]`: supported command (`n` / `N`: move to next / previous occurrence).

##
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/you-didnt-ask-for-this.svg)](https://forthebadge.com)
