NAME			:= taskmaster

SRC_DIR			:=	src
SRC_FILES		:=	main.cpp

SRCS			:= $(addprefix $(SRC_DIR)/, $(SRC_FILES))

BUILD_DIR		:=	build
OBJS			:=	$(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)
DEPS			:=  $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.d)
CXX_DEFS		:=	NAME=\"$(NAME)\"

CXX				:=	g++

CXX_FLAGS		:= -Wextra -Werror -Wall -std=c++17 -O2 -g3
CXX_LINKS		:=
CXX_LIBS		:=

CXX_HEADERS		:=

CXX_DEPS_FLAGS	:=	-MP -MMD
CXX_DEFS_FLAGS	:=	$(foreach def,$(CXX_DEFS),-D $(def))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(CXX_LINKS) $(OBJS) $(CXX_LIBS) -o $@

-include $(DEPS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(LIB_DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
