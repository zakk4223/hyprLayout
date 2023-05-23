# compile with HYPRLAND_HEADERS=<path_to_hl> make all
# make sure that the path above is to the root hl repo directory, NOT src/
# and that you have ran `make protocols` in the hl dir.

WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

CXXFLAGS=-shared -Wall -fPIC --no-gnu-unique -I "${HYPRLAND_HEADERS}" -I "/usr/include/pixman-1" -I "/usr/include/libdrm" -std=c++23 

OBJS=hyprlandLayout.o HyprlandLayoutProtocolManager.o hyprland-layout-v1-protocol.o main.o

hyprland-layout-v1-protocol.h: protocol/hyprland-layout-v1.xml
	$(WAYLAND_SCANNER) server-header protocol/hyprland-layout-v1.xml $@

hyprland-layout-v1-protocol.c: protocol/hyprland-layout-v1.xml
	$(WAYLAND_SCANNER) public-code protocol/hyprland-layout-v1.xml $@


hyprland-layout-v1-protocol.o: hyprland-layout-v1-protocol.h

protocol: hyprland-layout-v1-protocol.o


all: protocol $(OBJS) 
	g++ -shared -fPIC -o hyprlandLayoutPlugin.so -g $(OBJS) 

clean:
	rm ./hyprlandLayoutPlugin.so
	rm $(OBJS)
	rm hyprland-layout-v1-protocol.h
	rm hyprland-layout-v1-protocol.c
