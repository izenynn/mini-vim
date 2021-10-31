######################################################################
#                                VARS                                #
######################################################################

TAB_SIZE = 4

######################################################################
#                                NAME                                #
######################################################################

NAME = minivim
#NAME = mvm

######################################################################
#                              COMPILER                              #
######################################################################

CC = gcc

# flags
CFLAGS += -D TAB_SIZE=$(TAB_SIZE)

######################################################################
#                                LIBS                                #
######################################################################

CFLAGS += -I ./inc

#LDFLAGS = -L ./inc

# ncurses will be probably implemented in the future, right now only supports VT100 escapes
#LDLIBS = -lncurses

######################################################################
#                                 RM                                 #
######################################################################

RM = rm

RMFLAGS = -rf

######################################################################
#                               PATHS                                #
######################################################################

SRC_PATH = src

OBJ_PATH = obj

######################################################################
#                                SRC                                 #
######################################################################

SRC_FILES =		main.c			init.c			input.c			\
				output.c		append_buff.c	find.c			\
				file_io.c		editor_ops.c	row_ops.c		\
				syntax_hl.c		terminal.c

OBJ_FILES = $(SRC_FILES:%.c=%.o)

SRC = $(addprefix $(SRC_PATH)/, $(SRC_FILES))
OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_FILES))

######################################################################
#                                IF                                  #
######################################################################

DEV=0;
ifeq ($(DEV), 1)
	# common flags
	CFLAGS += -Wall -Werror -Wextra
	CFLAGS += -Wno-unknown-pragmas
	# check OS
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CFLAGS += -pedantic
		CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=bounds -fsanitize=null
		CFLAGS += -g3
	endif
	ifeq ($(UNAME_S),Darwin)
		CFLAGS += -pedantic
		CFLAGS += -fsanitize=address
		CFLAGS += -g3
	endif
endif

######################################################################
#                               RULES                                #
######################################################################

.PHONY: all dev clean fclean re

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c | $(OBJ_PATH)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_PATH):
	mkdir -p $(OBJ_PATH) 2> /dev/null

clean:
	$(RM) $(RMFLAGS) $(OBJ_PATH)

fclean: clean
	$(RM) $(RMFLAGS) $(NAME)

re: fclean all
