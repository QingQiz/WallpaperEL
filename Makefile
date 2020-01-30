include config.mk

default : release

release : 
	@make -C src release
	@cp src/$(EXEC_NAME) .

debug :
	@make -C src
	@cp src/$(EXEC_NAME) .

test : clean debug
	@cp $(EXEC_NAME) ./test

install :
	cp ./$(EXEC_NAME) $(INSTALL_DIR)

clean :
	@rm -f $(EXEC_NAME) test/$(EXEC_NAME)
	@make -C src clean

.PHONY : default $(EXEC_NAME) release debug
