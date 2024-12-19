RM				=	rm -rf

HTML_DIR		=	html/
LATEX_DIR		=	latex/

DOXYFILE_PATH	= Doxyfile
INDEX_HTML_PATH	= $(HTML_DIR)index.html

doc:
	@ doxygen $(DOXYFILE_PATH)
	@ echo "\nDocument:\n$(INDEX_HTML_PATH)\n"

fclean:
	$(RM) $(HTML_DIR) $(LATEX_DIR)

.PHONY:	doc fclean
