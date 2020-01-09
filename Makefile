include config.mk

default : $(EXEC_NAME)

$(EXEC_NAME): 
	@make -C src
	@cp src/$(EXEC_NAME) .

clean :
	@rm -f $(EXEC_NAME)
	@make -C src clean

.PHONY : default $(EXEC_NAME)
