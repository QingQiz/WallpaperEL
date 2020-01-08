include config.mk

default : $(EXEC_NAME)

$(EXEC_NAME): 
	@make -C src
	@cp src/now .

clean :
	@rm -f $(EXEC_NAME)
	@make -C src clean

.PHONY : default
