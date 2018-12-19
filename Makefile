all: libsystemuiplugin_modechange.so

clean:
	$(RM) libsystemuiplugin_modechange.so

install: libsystemuiplugin_modechange.so
	install -d $(DESTDIR)/usr/lib/systemui
	install -m 644 libsystemuiplugin_modechange.so $(DESTDIR)/usr/lib/systemui

libsystemuiplugin_modechange.so: osso-systemui-modechange.c
	$(CC) $^ -o $@ -shared -fPIC $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs --cflags osso-systemui hildon-1 gconf-2.0 gtk+-2.0 dbus-1 glib-2.0) -Wl,-soname -Wl,$@ -Wl,-rpath -Wl,/usr/lib/hildon-desktop

.PHONY: all clean install
