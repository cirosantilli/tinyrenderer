SYSCONF_LINK = g++
CPPFLAGS     = -ggdb3 -Wall -Wextra -Weffc++ -pedantic -std=c++98
LDFLAGS      = -O3
LIBS         = -lm -lSDL2 -lSDL2_image

DESTDIR = ./
TARGET  = main

OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(DESTDIR)$(TARGET)

$(DESTDIR)$(TARGET): $(OBJECTS)
	$(SYSCONF_LINK) -Wall $(LDFLAGS) -o $(DESTDIR)$(TARGET) $(OBJECTS) $(LIBS)

$(OBJECTS): %.o: %.cpp
	$(SYSCONF_LINK) -Wall $(CPPFLAGS) -c $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
	-rm -f *.tga

