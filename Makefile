all: clean
	make notsx=TSX -C demos

notsx: clean
	make notsx=NOTSX -C demos

clean:
	make -C demos clean
