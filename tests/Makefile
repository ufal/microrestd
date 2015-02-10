include ../src/Makefile.include

OBJECTS = $(addsuffix .o,$(notdir $(MICRORESTD_OBJECTS) $(MICRORESTD_PUGIXML_OBJECTS)))

.PHONY: all compile
all: compile

compile: $(OBJECTS)

compile = $(CXX) $< -std=c++11 -W -Wall -Werror -c -o $@
%.o: ../src/%.cpp; $(compile)
%.o: ../src/pugixml/%.cpp; $(compile)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
