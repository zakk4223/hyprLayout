# compile with HYPRLAND_HEADERS=<path_to_hl> make all
# make sure that the path above is to the root hl repo directory, NOT src/
# and that you have ran `make protocols` in the hl dir.

WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

CXXFLAGS=-g -shared -Wall -fPIC --no-gnu-unique `pkg-config --cflags  pixman-1 libdrm hyprland` -std=c++2b 

OBJS=hyprlandLayout.o HyprlandLayoutProtocolManager.o hyprland-layout-v1-protocol.o main.o focusHooks.o

hyprland-layout-v1-protocol.h: protocol/hyprland-layout-v1.xml
	$(WAYLAND_SCANNER) server-header protocol/hyprland-layout-v1.xml $@

hyprland-layout-v1-protocol.c: protocol/hyprland-layout-v1.xml
	$(WAYLAND_SCANNER) public-code protocol/hyprland-layout-v1.xml $@


hyprland-layout-v1-protocol.o: hyprland-layout-v1-protocol.h

protocol: hyprland-layout-v1-protocol.o

python_protocol: 
	python3 -m pywayland.scanner -i /usr/share/wayland/wayland.xml protocol/hyprland-layout-v1.xml -o ./pyprotocol



all: protocol $(OBJS) 
	g++ -shared -fPIC -o hyprlandLayoutPlugin.so -g $(OBJS) 

clean:
	rm ./hyprlandLayoutPlugin.so
	rm $(OBJS)
	rm hyprland-layout-v1-protocol.h
	rm hyprland-layout-v1-protocol.c
