include config.mk

default : release

release : 
	@make -C src release
	@cp src/$(EXEC_NAME) .

debug :
	@make -C src release
	@cp src/$(EXEC_NAME) .


clean :
	@rm -f $(EXEC_NAME)
	@make -C src clean

.PHONY : default $(EXEC_NAME) release debug
