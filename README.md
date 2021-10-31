# minivim

## info

A "mini" implementation of vim :3

It uses `VT100` escape characters (I will implement `ncurses` in the future probably).

## How to  use

- Clone the repo

```sh
git clone https://github.com/izenynn/minivim.git
```

- Run make inside the repo

```sh
cd ./minivim && make
```

- Open a file using minivim

```sh
./minivim [FILE]
```

- Create a new file

```sh
./minivim [FILE]
```

- or name it later

```sh
./minivim
```

## Features

- edit any text file
- C and C++ syntax highlighting
- Vim features:
	- normal move and insert mode
	- change mode with `i` and `a`
	- add new line and change mode with `o` and `O`
	- move with `h`, `j`, `k`, `l` in normal mode
	- move to start of line with `$` and end of line with `^`
	- move to start of file with `gg` and end with `G`
- TODO:
	- delete line with `dd`
	- copy line with `yy`
	- paste delete line with `p` or `P`
	- commands `:w`, `:q`, `:wq`, `:q!`
	- command `:saveas [NAME]` to save files
	- search with `/[SEARCH]`, `n` for next occurrence and `N` for previous

##
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/you-didnt-ask-for-this.svg)](https://forthebadge.com)
