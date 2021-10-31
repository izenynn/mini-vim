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
	- move with `h`, `j`, `k`, `l` in normal mode
	- commands `:w`, `:q`, `:wq`, `:q!`
	- command `:saveas [NAME]` to save files
	- search with `/[SEARCH]`, `n` for next occurrence and `N` for previous

##
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/you-didnt-ask-for-this.svg)](https://forthebadge.com)
