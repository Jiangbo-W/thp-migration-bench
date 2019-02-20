
CC=gcc

thp_move_pages: move_thp.c 
	$(CC) -o $@ $^ -lnuma

bench: thp_move_pages non_thp_move_pages
	@echo "THP Migration"
	@./thp_move_pages 1 2>/dev/null | grep -A 1 "Total\|Test"
	@echo "-------------------"
	@echo "Base Page Migration"
	@./non_thp_move_pages 512 2>/dev/null | grep -A 1 "Total\|Test"