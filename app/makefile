all: custom.lib sample.file notepad.file game_bubble.file game_hexa.file #HangulViewer OneLineMemo

custom.lib:
	@echo 
	@echo =========== Build Custom Library =============
	@echo 

	make -C custom_lib 

	@echo 
	@echo =============== Build Complete ===============
	@echo 

sample.file:
	@echo 
	@echo ============= Build Sample File ==============
	@echo 

	make -C sample

	@echo 
	@echo =============== Build Complete ===============
	@echo 

notepad.file:
	@echo 
	@echo ============== Build Notepad =================
	@echo 

	make -C notepad

	@echo 
	@echo =============== Build Complete ===============
	@echo 

game_bubble.file:
	@echo 
	@echo ============= Build Bubble Pang ==============
	@echo 

	make -C game_bubble

	@echo 
	@echo =============== Build Complete ===============
	@echo 

game_hexa.file:
	@echo 
	@echo ============= Build Hexa Bomb ================
	@echo 

	make -C game_hexa

	@echo 
	@echo =============== Build Complete ===============
	@echo 

HangulViewer:
	@echo 
	@echo ============= Build HangulViewer ===============
	@echo 

	make -C 04.HangulViewer

	@echo 
	@echo =============== Build Complete ===============
	@echo 

OneLineMemo:
	@echo 
	@echo ============= Build OneLineMemo ===============
	@echo 

	make -C 05.OneLineMemo

	@echo 
	@echo =============== Build Complete ===============
	@echo 

clean:
	make -C custom_lib clean
	make -C sample clean
	make -C notepad clean
	make -C game_bubble clean
	make -C game_hexa clean
#	make -C 04.HangulViewer clean
#	make -C 05.OneLineMemo clean
