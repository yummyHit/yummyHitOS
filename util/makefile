all: ImageMaker.exe

ImageMaker.exe: ImageMaker/ImageMaker.c
	gcc -o $@ $<
	chmod +x $@
	mv $@ ../binfiles/
