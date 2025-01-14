
CC=gcc

all: bench

thp_move_pages: move_thp.c 
	$(CC) -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

non_thp_move_pages: move_base_page.c 
	$(CC) -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

bench: thp_move_pages non_thp_move_pages
	@echo "THP Migration"
	@./thp_move_pages 0 1 1 2>/dev/null | grep -A 1 "Total\|Test"
	@echo "-------------------"
	@echo "Base Page Migration"
	@./non_thp_move_pages 0 1 1 2>/dev/null | grep -A 1 "Total\|Test"

clean:
	rm thp_move_pages non_thp_move_pages
