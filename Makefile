
CC=gcc

thp_move_pages: move_thp.c 
	$(CC) -o $@ $^ -lnuma
	sudo setcap "all=ep" $@

non_thp_move_pages: move_base_page.c 
	$(CC) -o $@ $^ -lnuma
	sudo setcap "all=ep" $@