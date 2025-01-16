# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: yoshimurahiro <yoshimurahiro@student.42    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/09/12 09:34:52 by tobeshota         #+#    #+#              #
#    Updated: 2025/01/17 07:52:30 by yoshimurahi      ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			=	OSInit
CC				=	c++
FLAGS			=	-Wall -Wextra -Werror -std=c++98 -pedantic-errors
RM				=	rm -rf

OBJS_DIR		=	objs/

SRCS			=	$(wildcard *.cpp)
HEADERS			=	$(wildcard *.hpp)
SHS				=	$(wildcard *.sh)
OBJS			=	$(patsubst %.cpp, $(OBJS_DIR)%.o, $(SRCS))

ifeq ($(MAKECMDGOALS), debug)
	FLAGS += -g -DDEBUG
endif

ifeq ($(MAKECMDGOALS), address)
	FLAGS += -g3 -fsanitize=address
endif

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS)   $(OBJS) -o $@

$(OBJS_DIR)%.o : %.cpp
	mkdir -p $(@D)
	$(CC) $(FLAGS)   -c $< -o $@

clean:
	$(RM) $(OBJS_DIR)

fclean: clean
	# make fclean -C test/
	# make fclean -C ./docs -f doc.mk
	$(RM) $(NAME)

re:		fclean all

fmt:
	make fmt -C test/
	$(if $(SRCS), clang-format --style=Google -i $(SRCS))
	$(if $(HEADERS), clang-format --style=Google -i $(HEADERS))
	$(if $(SHS), shfmt -w $(SHS))

debug: re

address: re

test:
	@ make test -C test/

coverage:
	@ make coverage -C test/

doc:
	make doc -C docs/ -f doc.mk

.PHONY:	all clean fclean re fmt debug address test doc
