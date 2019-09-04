
CC=gcc

all: move_4kb_pages move_2mb_pages move_4kb_pages_par move_2mb_pages_par

move_2mb_pages: move_page_breakdown.c 
	$(CC) -DUSE_2MB -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

move_4kb_pages: move_page_breakdown.c 
	$(CC) -DUSE_4KB -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

move_2mb_pages_par: move_page_breakdown.c 
	$(CC) -DUSE_2MB -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

move_4kb_pages_par: move_page_breakdown.c 
	$(CC) -DUSE_4KB -o $@ $^ -lnuma
	sudo setcap "all=ep" $@
