# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tobeshota <tobeshota@student.42.fr>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/09/12 09:34:52 by tobeshota         #+#    #+#              #
#    Updated: 2024/11/13 16:27:32 by tobeshota        ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			=	webserv
CC				=	c++
FLAGS			=	-Wall -Wextra -Werror -std=c++98 -pedantic-errors
RM				=	rm -rf

SRCS_DIR		=	srcs/
INCS_DIR		=	srcs/
OBJS_DIR		=	objs/

SRCS			=	$(wildcard $(SRCS_DIR)*.cpp)
HEADERS			=	$(wildcard $(INCS_DIR)*.hpp)
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
	$(CC) $(FLAGS) -I $(INCS_DIR) $(OBJS) -o $@

$(OBJS_DIR)%.o : %.cpp
	mkdir -p $(@D)
	$(CC) $(FLAGS) -I $(INCS_DIR) -c $< -o $@

clean:
	$(RM) $(OBJS_DIR)

fclean: clean
	make fclean -C test/
	make fclean -C ./docs -f doc.mk
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

doc:
	make doc -C docs/ -f doc.mk

.PHONY:	all clean fclean re fmt debug address test doc
