RM			=	rm -rf

SRCS_DIR	=	srcs/
INCS_DIR	=	srcs/
BUILD_DIR	=	build/

SRCS		=	$(wildcard $(SRCS_DIR)*.cpp)
HEADERS		=	$(wildcard $(INCS_DIR)*.hpp)

test:
	@ cmake -S . -B build -DTEST_BY_GTEST=1	# ビルド時にTEST_BY_GTESTが定義される.実行プロセスがGoogleTestによるものであることを確認するために定義することにした
	@ cmake --build build
	@ cd build && ctest --output-on-failure

fclean:
	$(RM) $(BUILD_DIR)

fmt:
	$(if $(SRCS), clang-format --style=Google -i $(SRCS))
	$(if $(HEADERS), clang-format --style=Google -i $(HEADERS))

.PHONY:	test fclean fmt
