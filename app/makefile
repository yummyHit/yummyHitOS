all: custom.lib sample.file #TextViewer Shooter Hexa HangulViewer OneLineMemo

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

TextViewer:
	@echo 
	@echo ============= Build TextViewer ===============
	@echo 

	make -C 01.TextViewer

	@echo 
	@echo =============== Build Complete ===============
	@echo 

Shooter:
	@echo 
	@echo ============= Build Shooter ===============
	@echo 

	make -C 02.Shooter

	@echo 
	@echo =============== Build Complete ===============
	@echo 

Hexa:
	@echo 
	@echo ============= Build Hexa ===============
	@echo 

	make -C 03.Hexa

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
#	make -C 01.TextViewer clean
#	make -C 02.Shooter clean
#	make -C 03.Hexa clean
#	make -C 04.HangulViewer clean
#	make -C 05.OneLineMemo clean
