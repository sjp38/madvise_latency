madv_lat: madv_lat.c
	gcc -o $@ $^

clean:
	rm -fr madv_lat
